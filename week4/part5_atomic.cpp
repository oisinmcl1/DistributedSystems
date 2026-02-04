#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>

// ============================================================
// WeatherSample: simple data structure for hourly weather data
// ============================================================
class WeatherSample {
public:
    WeatherSample() = default;
    ~WeatherSample() = default;
    
    void setTemperature(float t) { temperature = t; }
    void setHumidity(float h) { humidity = h; }
    void setDayIndex(int d) { dayIndex = d; }
    void setHourOfDay(int h) { hourOfDay = h; }

    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    int getDayIndex() const { return dayIndex; }
    int getHourOfDay() const { return hourOfDay; }

private:
    float temperature = 0.0f;
    float humidity = 0.0f;
    int dayIndex = 0;
    int hourOfDay = 0;
};

// ============================================================
// WeatherLoader: simulates reading weather data from Excel/CSV
// ============================================================
class WeatherLoader {
public:
    WeatherLoader() = default;
    ~WeatherLoader() = default;
    
    void loadOneRow(WeatherSample& sample, std::size_t rowIndex) {
        int day = static_cast<int>(rowIndex / 24);
        int hour = static_cast<int>(rowIndex % 24);

        std::uint32_t x = static_cast<std::uint32_t>(rowIndex);
        x = x * 1664525u + 1013904223u;
        float r1 = (x & 0xFFFFu) / 65535.0f;

        x = x * 1664525u + 1013904223u;
        float r2 = (x & 0xFFFFu) / 65535.0f;

        float dailyOffset = 0.0f;
        if (hour >= 6 && hour <= 16) {
            dailyOffset = 8.0f;
        } else if (hour >= 17 && hour <= 20) {
            dailyOffset = 4.0f;
        }

        int dayOfYear = day % 365;
        float seasonal = (dayOfYear - 182) / 182.0f;
        float baseTemp = 10.0f + 8.0f * seasonal;

        float temperature = baseTemp + dailyOffset + 4.0f * r1;
        float humidity = 30.0f + 60.0f * r2;

        sample.setDayIndex(day);
        sample.setHourOfDay(hour);
        sample.setTemperature(temperature);
        sample.setHumidity(humidity);
    }
};

// ============================================================
// WeatherDataset: RAII owner of large weather dataset
// ============================================================
class WeatherDataset {
public:
    // Constructor: allocates heap memory (RAII)
    explicit WeatherDataset(std::size_t count) {
        this->count = count;

        // RAII OPTION 1 (Recommended, modern C++)
        // We don't write 'new', but the program still allocates on the heap —
        // because make_unique does it for us safely and returns a unique_ptr
        // that owns the memory.
        data = std::make_unique<WeatherSample[]>(count);

        /*
        // RAII OPTION 2 (Also correct, more explicit for learning)
        // We explicitly use 'new[]' here, but ownership is still transferred
        // to unique_ptr immediately, so this is still RAII and safe.
        data = std::unique_ptr<WeatherSample[]>(
            new WeatherSample[count]
        );
        */
    }

    // Destructor: unique_ptr automatically deletes heap array
    ~WeatherDataset() = default;

    std::size_t size() const { return count; }

    WeatherSample& at(std::size_t i) {
        return data[i];
    }

    const WeatherSample& at(std::size_t i) const {
        return data[i];
    }

    void loadDataset(WeatherLoader& loader) {
        for (std::size_t i = 0; i < count; ++i) {
            loader.loadOneRow(data[i], i);
        }
    }

private:
    std::unique_ptr<WeatherSample[]> data;
    std::size_t count;
};

// ============================================================
// GLOBAL shared variable (now ATOMIC - lock-free!)
// ============================================================
// Note: std::atomic<double> uses special CPU instructions or internal locking
// to ensure atomic read-modify-write operations
std::atomic<double> atomicTotal{0.0};

// ============================================================
// Worker function with SAFE reduction using atomic operations
// ============================================================
void workerAtomicReduce(const WeatherDataset& dataset, 
                        std::size_t begin, 
                        std::size_t end,
                        int threadId) {
    double partial = 0.0;
    
    // MAP: compute metric for this chunk (SAFE - local variable)
    for (std::size_t i = begin; i < end; ++i) {
        partial += dataset.at(i).getTemperature() + 0.01 * dataset.at(i).getHumidity();
    }
    
    std::cout << "Thread " << threadId << " computed partial: " 
              << std::fixed << std::setprecision(2) << partial << "\n";
    
    // REDUCE: SAFE! Using atomic fetch_add (C++20 feature for floating-point)
    // fetch_add is an atomic operation that:
    // 1. Reads the current value
    // 2. Adds the partial to it
    // 3. Writes back the result
    // All as ONE indivisible operation!
    atomicTotal.fetch_add(partial, std::memory_order_relaxed);

    // Note: We use memory_order_relaxed because we only care about
    // the atomicity of the addition, not ordering with other operations
}

// ============================================================
// MAIN: Demonstrate SAFE synchronization with std::atomic
// ============================================================
int main() {
    constexpr std::size_t DATA_SIZE = 10'000'000;
    constexpr std::size_t CHUNK_SIZE = 250'000;
    constexpr int NUM_THREADS = 4;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=== Part 5: SAFE with std::atomic (Lock-Free) ===\n\n";
    std::cout << "This version uses std::atomic<double> for lock-free synchronization\n\n";

    // Reset atomic variable
    atomicTotal.store(0.0);

    // Create and load dataset
    WeatherDataset dataset(DATA_SIZE);
    WeatherLoader loader;

    std::cout << "Loading dataset...\n";
    auto loadStart = std::chrono::high_resolution_clock::now();
    dataset.loadDataset(loader);
    auto loadEnd = std::chrono::high_resolution_clock::now();
    
    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();
    std::cout << "Load time: " << loadMs << " ms\n\n";

    // Compute expected value first (sequential)
    double expected = 0.0;
    for (std::size_t i = 0; i < DATA_SIZE; ++i) {
        expected += dataset.at(i).getTemperature() + 0.01 * dataset.at(i).getHumidity();
    }
    
    std::cout << "Expected total (computed sequentially): " << expected << "\n\n";

    // Prepare parallel computation with ATOMIC OPERATIONS
    std::cout << "=== Parallel with std::atomic (" << NUM_THREADS << " threads) ===\n\n";
    
    std::vector<std::thread> threads;
    std::size_t numChunks = (DATA_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;
    std::size_t chunksPerThread = numChunks / NUM_THREADS;
    
    auto computeStart = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    std::cout << "Launching threads...\n";
    for (int t = 0; t < NUM_THREADS; ++t) {
        std::size_t begin = t * chunksPerThread * CHUNK_SIZE;
        std::size_t end = (t == NUM_THREADS - 1) ? DATA_SIZE : (t + 1) * chunksPerThread * CHUNK_SIZE;
        
        threads.emplace_back(workerAtomicReduce, 
                            std::ref(dataset), 
                            begin, 
                            end, 
                            t);
    }
    
    // Wait for all threads
    std::cout << "\nWaiting for threads to complete...\n";
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto computeEnd = std::chrono::high_resolution_clock::now();
    auto computeMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - computeStart).count();

    std::cout << "\n=== Results ===\n";
    std::cout << "Expected total: " << expected << "\n";
    std::cout << "Actual total:   " << atomicTotal.load() << "\n";
    std::cout << "Difference:     " << (expected - atomicTotal.load()) << "\n";
    std::cout << "Compute time:   " << computeMs << " ms\n\n";

    if (std::abs(expected - atomicTotal.load()) < 0.01) {
        std::cout << "Result: CORRECT! Atomic operations ensured thread safety.\n";
        std::cout << "        Run this multiple times - always correct!\n";
    } else {
        std::cout << "Result: INCORRECT - This should not happen with atomics!\n";
    }

    std::cout << " - MAP phase: safe (each thread uses local variable)\n";
    std::cout << " - REDUCE phase: SAFE (atomic fetch_add ensures atomicity)\n";
    std::cout << " - std::atomic<double>: uses hardware or internal synchronization\n";
    std::cout << " - fetch_add: atomic read-modify-write operation (C++20 for double)\n";
    std::cout << " - Lock-free: potentially faster than mutex (no blocking)\n";
    std::cout << " - No critical section: operations are inherently atomic\n";
    std::cout << " - No lost updates: each addition happens atomically\n\n";
    
    std::cout << "Comparison: std::atomic vs std::mutex\n";
    std::cout << " - atomic: Lock-free, faster for simple operations (single variable)\n";
    std::cout << " - mutex: More flexible, can protect complex multi-statement logic\n";
    std::cout << " - Use atomic for: counters, flags, single variable updates\n";
    std::cout << " - Use mutex for: multiple variables, complex critical sections\n\n";
    
    std::cout << "Compilation: g++ -std=c++20 -pthread -O2 part5_atomic.cpp\n";

    return 0;
}
