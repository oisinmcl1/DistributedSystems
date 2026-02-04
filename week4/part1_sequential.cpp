#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <algorithm>  // for std::min

// ============================================================
// WeatherSample: simple data structure for hourly weather data
// ============================================================
class WeatherSample {
public:
    WeatherSample() = default;  // Default constructor (C++11)
    ~WeatherSample() = default;  // Default destructor (C++11)
    
    void setTemperature(float t) { temperature = t; }
    void setHumidity(float h) { humidity = h; }
    void setDayIndex(int d) { dayIndex = d; }
    void setHourOfDay(int h) { hourOfDay = h; }

    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    int getDayIndex() const { return dayIndex; }
    int getHourOfDay() const { return hourOfDay; }

private:
    // These members are stored wherever the WeatherSample object is allocated:
    // - If object is on stack → members on stack
    // - If object is on heap → members on heap
    // In this program: allocated in heap array, so these are on HEAP
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
    WeatherLoader() = default;  // Default constructor
    ~WeatherLoader() = default;  // Default destructor
    
    // Simulate reading one row from "Excel"
    void loadOneRow(WeatherSample& sample, std::size_t rowIndex) {
        int day = static_cast<int>(rowIndex / 24);
        int hour = static_cast<int>(rowIndex % 24);

        // Deterministic pseudo-random values
        std::uint32_t x = static_cast<std::uint32_t>(rowIndex);
        x = x * 1664525u + 1013904223u;
        float r1 = (x & 0xFFFFu) / 65535.0f;

        x = x * 1664525u + 1013904223u;
        float r2 = (x & 0xFFFFu) / 65535.0f;

        // Daily pattern: warmer during daytime
        float dailyOffset = 0.0f;
        if (hour >= 6 && hour <= 16) {
            dailyOffset = 8.0f;
        } else if (hour >= 17 && hour <= 20) {
            dailyOffset = 4.0f;
        }

        // Seasonal pattern
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

    // Accessor method: provides controlled access to heap array elements
    // Returns reference (&) so caller can read or modify the element
    // Note: No bounds checking in this teaching example (production code should validate i < count)
    WeatherSample& at(std::size_t i) {
        return data[i];
    }

    // Load all rows
    void loadDataset(WeatherLoader& loader) {
        for (std::size_t i = 0; i < count; ++i) {
            loader.loadOneRow(data[i], i);
        }
    }

private:
    std::unique_ptr<WeatherSample[]> data;  // RAII: owns heap array
    std::size_t count;
};

// ============================================================
// MAIN: Sequential MAP + REDUCE
// ============================================================
int main() {
    constexpr std::size_t DATA_SIZE = 10'000'000;
    constexpr std::size_t CHUNK_SIZE = 250'000;

    std::cout << std::fixed << std::setprecision(2);

    // Create dataset (allocates heap memory)
    WeatherDataset dataset(DATA_SIZE);
    WeatherLoader loader;

    std::cout << "Loading dataset...\n";
    auto loadStart = std::chrono::high_resolution_clock::now();
    dataset.loadDataset(loader);
    auto loadEnd = std::chrono::high_resolution_clock::now();
    
    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();
    std::cout << "Load time: " << loadMs << " ms\n\n";

    // Show sample rows
    std::cout << "Row[0]  -> day " << dataset.at(0).getDayIndex() 
              << ", hour " << dataset.at(0).getHourOfDay()
              << ", temp " << dataset.at(0).getTemperature() 
              << "°C, humidity " << dataset.at(0).getHumidity() << "%\n";
    std::cout << "Row[1]  -> day " << dataset.at(1).getDayIndex() 
              << ", hour " << dataset.at(1).getHourOfDay()
              << ", temp " << dataset.at(1).getTemperature() 
              << "°C, humidity " << dataset.at(1).getHumidity() << "%\n";
    std::cout << "Row[24] -> day " << dataset.at(24).getDayIndex() 
              << ", hour " << dataset.at(24).getHourOfDay()
              << ", temp " << dataset.at(24).getTemperature() 
              << "°C, humidity " << dataset.at(24).getHumidity() << "%\n\n";

    // MAP PHASE: compute partial sums per chunk
    std::cout << "Computing metric using Map + Reduce...\n";
    auto computeStart = std::chrono::high_resolution_clock::now();

    std::size_t numChunks = (DATA_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;
    std::vector<double> partialSums(numChunks, 0.0);

    // MAP: process each chunk
    for (std::size_t c = 0; c < numChunks; ++c) {
        std::size_t begin = c * CHUNK_SIZE;
        std::size_t end = std::min(begin + CHUNK_SIZE, DATA_SIZE);

        double partial = 0.0;
        for (std::size_t i = begin; i < end; ++i) {
            // MAP: compute metric for each row
            partial += dataset.at(i).getTemperature() + 0.01 * dataset.at(i).getHumidity();
        }
        partialSums[c] = partial;
    }

    // REDUCE PHASE: sum all partial values
    double total = 0.0;
    for (std::size_t c = 0; c < partialSums.size(); ++c) {
        total += partialSums[c];
    }

    auto computeEnd = std::chrono::high_resolution_clock::now();
    auto computeMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - computeStart).count();

    std::cout << "Total metric sum: " << total << "\n";
    std::cout << "Compute time: " << computeMs << " ms\n\n";
    std::cout << "SUMMARY:\n";
    std::cout << " - Dataset size: " << DATA_SIZE << " samples\n";
    std::cout << " - RAII: WeatherDataset allocates heap memory in constructor\n";
    std::cout << " - Smart pointer: unique_ptr automatically frees memory\n";
    std::cout << " - Destructor: = default lets compiler handle RAII cleanup\n";
    std::cout << " - MAP: compute partial sums for each chunk\n";
    std::cout << " - REDUCE: sum all partial sums to get final total\n";

    return 0;
}

/*
============================================================
CLASS RELATIONSHIP DIAGRAM
============================================================

                        STACK                                           HEAP
                    ┌─────────────┐                          ┌────────────────────────────────┐
                    │   main()    │                          │                                │
                    ├─────────────┤                          │  WeatherSample[10,000,000]     │
                    │             │                          │  ┌──────┬──────┬──────┬─────┐  │
  WeatherDataset    │  dataset    │───────────────────────▶  │  │ [0]  │ [1]  │ [2]  │ ... │  │
  (RAII owner)      │  ┌────────┐ │   unique_ptr owns        │  ├──────┼──────┼──────┼─────┤  │
                    │  │ count  │ │   heap array             │  │temp  │temp  │temp  │     │  │
                    │  │ data ──┼─┼──────────────────────────│──│humid │humid │humid │     │  │
                    │  └────────┘ │                          │  │day   │day   │day   │     │  │
                    │             │                          │  │hour  │hour  │hour  │     │  │
                    ├─────────────┤                          │  └──────┴──────┴──────┴─────┘  │
  WeatherLoader     │   loader    │                          │                                │
  (stateless)       │  ┌────────┐ │                          └────────────────────────────────┘
                    │  │ (empty)│ │
                    │  └────────┘ │
                    └─────────────┘

                            │
                            ▼

              ┌─────────────────────────────────────┐
              │         Object Interactions         │
              └─────────────────────────────────────┘

    1. WeatherDataset allocates heap array in constructor (RAII)
    2. WeatherLoader.loadOneRow() fills each WeatherSample with data
    3. main() accesses samples via dataset.at(i) for MAP/REDUCE
    4. unique_ptr automatically frees heap when dataset goes out of scope

              ┌──────────────────┐
              │  WeatherLoader   │
              │   (helper)       │
              └────────┬─────────┘
                       │ loadOneRow(sample, index)
                       │ writes to each sample
                       ▼
              ┌──────────────────┐         owns          ┌──────────────────┐
              │  WeatherDataset  │ ─────────────────────▶ │  WeatherSample[] │
              │   (RAII owner)   │    unique_ptr          │  (heap array)    │
              └──────────────────┘                        └──────────────────┘
                       │
                       │ at(i) returns reference
                       ▼
              ┌──────────────────┐
              │     main()       │
              │  MAP + REDUCE    │
              └──────────────────┘

============================================================
MAP + REDUCE CHUNKING DIAGRAM
============================================================

    ORIGINAL DATA: 10,000,000 WeatherSamples
    ┌────────────────────────────────────────────────────────────────────┐
    │ [0]  [1]  [2]  ...  [249,999] │ [250,000] ... │ ... │ [9,999,999]  │
    └────────────────────────────────────────────────────────────────────┘
                              │
                              ▼ SPLIT into 40 chunks of 250,000 each

    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       ┌─────────────┐
    │   Chunk 0   │  │   Chunk 1   │  │   Chunk 2   │  ...  │  Chunk 39   │
    │ [0..249999] │  │[250k..499k] │  │[500k..749k] │       │[9.75M..10M] │
    └──────┬──────┘  └──────┬──────┘  └──────┬──────┘       └──────┬──────┘
           │                │                │                     │
           ▼                ▼                ▼                     ▼
    ╔═════════════════════════════════════════════════════════════════════╗
    ║                        MAP PHASE                                    ║
    ║  For each chunk: sum(temperature + 0.01 × humidity)                 ║
    ╚═════════════════════════════════════════════════════════════════════╝
           │                │                │                     │
           ▼                ▼                ▼                     ▼
    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       ┌─────────────┐
    │ partial[0]  │  │ partial[1]  │  │ partial[2]  │  ...  │ partial[39] │
    │ 4,232,898.9 │  │ 4,232,898.9 │  │ 4,232,898.9 │       │ 4,232,898.9 │
    └──────┬──────┘  └──────┬──────┘  └──────┬──────┘       └──────┬──────┘
           │                │                │                     │
           └────────────────┴────────────────┴──────────┬──────────┘
                                                        │
                                                        ▼
                   ╔═════════════════════════════════════════════╗
                   ║              REDUCE PHASE                   ║
                   ║  total = sum(partial[0] + partial[1] + ...) ║
                   ╚═════════════════════════════════════════════╝
                                                        │
                                                        ▼
                                          ┌─────────────────────────┐
                                          │    FINAL RESULT         │
                                          │    169,315,954.90       │
                                          └─────────────────────────┘

    KEY INSIGHT:
    • Sequential: One loop processes all 40 chunks one after another
    • Parallel: Multiple threads process different chunks SIMULTANEOUSLY
    • Same pattern, different execution model!

*/