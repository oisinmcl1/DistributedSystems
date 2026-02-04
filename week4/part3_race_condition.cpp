#include <iostream>
#include <memory>
#include <vector>
#include <thread>
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
        // Deterministic pseudo-random values
        int day = static_cast<int>(rowIndex / 24); // Days start at 0 
        int hour = static_cast<int>(rowIndex % 24); // Hours 0-23

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
        this-> data = std::make_unique<WeatherSample[]>(count);

        /*
        // RAII OPTION 2 (Also correct, more explicit for learning)
        // We explicitly use 'new[]' here, but ownership is still transferred
        // to unique_ptr immediately, so this is still RAII and safe.
        this-> data = std::unique_ptr<WeatherSample[]>(new WeatherSample[count]
        );
        */
    }

    // Destructor: unique_ptr automatically deletes heap array
    ~WeatherDataset() = default;

    std::size_t size() const { return count; }
    
    // We provide two versions of at():
    // 1. Non-const version - allows modification of the WeatherSample (used when loading data)
    // 2. Const version - for read-only access when the dataset is const (used in worker threads)
    // This enables the compiler to enforce const-correctness and prevents accidental modifications
    WeatherSample& at(std::size_t i) {
        return data[i];
    }
    const WeatherSample& at(std::size_t i) const {
        return data[i];
    }

    void loadDataset(WeatherLoader& loader) {
        for (std::size_t i = 0; i < count; ++i) {
            loader.loadOneRow(data[i], i);
            // alternatively: loader.loadOneRow(this->at(i), i);
        }
    }

private:
    std::unique_ptr<WeatherSample[]> data;
    std::size_t count;
};

// ============================================================
// GLOBAL shared variable (RACE CONDITION!)
// ============================================================
double sharedTotal = 0.0;

// ============================================================
// Worker function with UNSAFE reduction
// ============================================================
void workerUnsafeReduce(const WeatherDataset& dataset, 
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
    
    // REDUCE: UNSAFE! Multiple threads updating sharedTotal
    // To make race more visible, update in small chunks
    const double CHUNK = 1000.0;
    for (double remaining = partial; remaining > 0.0; remaining -= CHUNK) {
        double amount = (remaining > CHUNK) ? CHUNK : remaining;
      
        sharedTotal += amount;  // RACE CONDITION HERE!
    }
    
    // sharedTotal += partial;  // RACE CONDITION HERE!
}

// ============================================================
// MAIN: Demonstrate RACE CONDITION
// ============================================================
int main() {
    constexpr std::size_t DATA_SIZE = 10'000'000;
    constexpr std::size_t CHUNK_SIZE = 250'000;
    constexpr int NUM_THREADS = 4;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "=== Part 3: UNSAFE Race Condition ===\n\n";
    std::cout << "WARNING: This code has an intentional race condition!\n";
    std::cout << "Run it multiple times and observe different (wrong) results!\n\n";

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

    // Prepare parallel computation with RACE CONDITION
    std::cout << "=== Parallel with RACE CONDITION (" << NUM_THREADS << " threads) ===\n\n";
    
    std::vector<std::thread> threads;
    std::size_t numChunks = (DATA_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;
    std::size_t chunksPerThread = numChunks / NUM_THREADS;
    
    auto computeStart = std::chrono::high_resolution_clock::now();
    
    // Launch threads
    std::cout << "Launching threads...\n";
    for (int t = 0; t < NUM_THREADS; ++t) {
        std::size_t begin = t * chunksPerThread * CHUNK_SIZE;
        std::size_t end = (t == NUM_THREADS - 1) ? DATA_SIZE : (t + 1) * chunksPerThread * CHUNK_SIZE;
        
        threads.emplace_back(workerUnsafeReduce, 
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
    std::cout << "Difference:     " << (expected - sharedTotal) << " (LOST UPDATES!)\n";
    std::cout << "Compute time:   " << computeMs << " ms\n\n";

    if (std::abs(expected - sharedTotal) < 0.01) {
        std::cout << "Result: CORRECT (but this is just LUCK!)\n";
        std::cout << "        Compile with -O0 and run again to see the race!\n";
    } else {
        std::cout << "Result: INCORRECT - Race condition caused lost updates!\n";
    }

    std::cout << "\nTeaching notes:\n";
    std::cout << " - MAP phase: safe (each thread uses local variable)\n";
    std::cout << " - REDUCE phase: UNSAFE (all threads update sharedTotal)\n";
    std::cout << " - += is NOT atomic: read-modify-write can interleave\n";
    std::cout << " - Compile with: g++ -std=c++17 -pthread -O0 part3_race_condition.cpp\n";
    std::cout << " - The -O0 flag disables optimizations to make race more visible\n";

    return 0;
}
