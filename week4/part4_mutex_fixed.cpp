#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
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
// GLOBAL shared variable (now PROTECTED by mutex!)
// ============================================================
double sharedTotal = 0.0;
std::mutex totalMutex;  // Protects sharedTotal

// ============================================================
// Worker function with SAFE reduction using mutex
// ============================================================
void workerSafeReduce(const WeatherDataset& dataset, 
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
    
    // REDUCE: SAFE! Using std::lock_guard for mutex protection
    {
        std::lock_guard<std::mutex> lock(totalMutex);
        sharedTotal += partial;  // CRITICAL SECTION - only one thread at a time
    }  // lock automatically released here (RAII!)
}

// ============================================================
// MAIN: Demonstrate SAFE synchronization with mutex
// ============================================================
int main() {
    constexpr std::size_t DATA_SIZE = 10'000'000;
    constexpr std::size_t CHUNK_SIZE = 250'000;
    constexpr int NUM_THREADS = 4;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=== Part 4: SAFE with Mutex + RAII ===\n\n";
    std::cout << "This version uses std::lock_guard for automatic mutex protection\n\n";

    // Reset shared variable
    sharedTotal = 0.0;

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

    // Prepare parallel computation with MUTEX PROTECTION
    std::cout << "=== Parallel with MUTEX (" << NUM_THREADS << " threads) ===\n\n";
    
    std::vector<std::thread> threads;
    std::size_t numChunks = (DATA_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;
    std::size_t chunksPerThread = numChunks / NUM_THREADS;
    
    auto computeStart = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    std::cout << "Launching threads...\n";
    for (int t = 0; t < NUM_THREADS; ++t) {
        std::size_t begin = t * chunksPerThread * CHUNK_SIZE;
        std::size_t end = (t == NUM_THREADS - 1) ? DATA_SIZE : (t + 1) * chunksPerThread * CHUNK_SIZE;
        
        threads.emplace_back(workerSafeReduce, 
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
    std::cout << "Actual total:   " << sharedTotal << "\n";
    std::cout << "Difference:     " << (expected - sharedTotal) << "\n";
    std::cout << "Compute time:   " << computeMs << " ms\n\n";

    if (std::abs(expected - sharedTotal) < 0.01) {
        std::cout << "Result: CORRECT! Mutex ensured thread safety.\n";
        std::cout << "        Run this multiple times - always correct!\n";
    } else {
        std::cout << "Result: INCORRECT - This should not happen with mutex!\n";
    }

    std::cout << "\nTeaching notes:\n";
    std::cout << " - MAP phase: safe (each thread uses local variable)\n";
    std::cout << " - REDUCE phase: SAFE (mutex protects sharedTotal)\n";
    std::cout << " - std::lock_guard: RAII wrapper for mutex\n";
    std::cout << " - Lock acquired when lock_guard constructed\n";
    std::cout << " - Lock released when lock_guard destroyed (automatic!)\n";
    std::cout << " - Critical section: only one thread executes sharedTotal += partial\n";
    std::cout << " - No lost updates: all partial results properly accumulated\n";

    return 0;
}
