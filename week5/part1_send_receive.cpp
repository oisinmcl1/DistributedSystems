#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <mpi.h>

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
// Generates deterministic synthetic weather data from rowIndex
class WeatherLoader {
public:
    WeatherLoader() = default;
    ~WeatherLoader() = default;
    
    // Generate one synthetic weather row from rowIndex
    // rowIndex is the global row number (0 to 10,000,000)
    // Uses deterministic algorithm: extract day/hour, apply LCG for randomness
    void loadOneRow(WeatherSample& sample, std::size_t rowIndex) {
        // STEP 1: Extract day and hour from global rowIndex
        // 24 rows = 1 day (hourly data), so rowIndex/24 = which day
        int day = static_cast<int>(rowIndex / 24);
        int hour = static_cast<int>(rowIndex % 24);

        // STEP 2: Generate random values using LCG (Linear Congruential Generator)
        // Deterministic: same rowIndex seed → same random values
        // LCG formula: x_next = (1664525 * x + 1013904223) mod 2^32
        std::uint32_t x = static_cast<std::uint32_t>(rowIndex);
        // First random: for temperature variation
        x = x * 1664525u + 1013904223u;
        float r1 = (x & 0xFFFFu) / 65535.0f;  // Extract lower 16 bits, scale to [0,1)

        // Second random: for humidity variation using new x value
        x = x * 1664525u + 1013904223u;
        float r2 = (x & 0xFFFFu) / 65535.0f;

        // STEP 3: Daily temperature pattern (warmer day, cold night)
        float dailyOffset = 0.0f;
        if (hour >= 6 && hour <= 16) {
            dailyOffset = 8.0f;    // Daytime (6am-4pm): +8C above baseline
        } else if (hour >= 17 && hour <= 20) {
            dailyOffset = 4.0f;    // Evening (5pm-8pm): +4C transition
        }
        // Nighttime (9pm-5am): 0C offset (cold)

        // STEP 4: Seasonal temperature pattern (winter vs summer)
        // Day 0 = coldest (winter), Day 182 = hottest (summer), repeat
        int dayOfYear = day % 365;
        // seasonal ranges from -1.0 (day 0) to +1.0 (day 182)
        float seasonal = (dayOfYear - 182) / 182.0f;
        // baseTemp ranges from 2C (winter) to 18C (summer)
        float baseTemp = 10.0f + 8.0f * seasonal;

        // STEP 5: Combine all factors for final temperature and humidity
        // temperature = seasonal_base + daily_offset + random_noise
        // Example: Summer daytime = 18C + 8C + 0-4C random = 26-30C
        //          Winter night = 2C + 0C + 0-4C random = 2-6C
        float temperature = baseTemp + dailyOffset + 4.0f * r1;
        // humidity ranges from 30% (very dry) to 90% (very humid)
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
    explicit WeatherDataset(std::size_t count) {
        this->count = count;
        data = std::make_unique<WeatherSample[]>(count);
    }

    ~WeatherDataset() = default;

    std::size_t size() const { return count; }

    WeatherSample& at(std::size_t i) {
        return data[i];
    }

    const WeatherSample& at(std::size_t i) const {
        return data[i];
    }

    // Generate all data samples for this rank's partition
    // globalOffset = starting rowIndex for this rank (e.g., Rank 0: 0, Rank 1: 2,500,000)
    // Example: Rank 0 generates rows 0-2,499,999, Rank 1 generates rows 2,500,000-4,999,999
    void loadDataset(WeatherLoader& loader, std::size_t globalOffset) {
        for (std::size_t i = 0; i < count; ++i) {
            // globalOffset + i = true global row index for this sample
            loader.loadOneRow(data[i], globalOffset + i);
        }
    }

private:
    std::unique_ptr<WeatherSample[]> data;
    std::size_t count;
};

// ============================================================
// MPI Error Handling Helper
// ============================================================
// All MPI functions return error codes: MPI_SUCCESS (0) or error value
// This helper checks for errors and provides diagnostic output
// PARAMETER: errorCode = return value from MPI call (e.g., from MPI_Init)
// PARAMETER: functionName = name of MPI function for error reporting (e.g., "MPI_Init")
// ON ERROR: Prints human-readable message and calls MPI_Abort to terminate all ranks
static void check_mpi_error(int errorCode, const char* functionName) {
    if (errorCode != MPI_SUCCESS) {
        char errorString[MPI_MAX_ERROR_STRING];
        int errorStringLen = 0;
        // Convert MPI error code to readable error description
        MPI_Error_string(errorCode, errorString, &errorStringLen);
        // Print to stderr with function name context
        std::cerr << "MPI error in " << functionName << ": "
                  << std::string(errorString, errorStringLen) << std::endl;
        // MPI_Abort: Terminate all processes in MPI_COMM_WORLD cleanly
        // Better than exit() because it notifies all ranks of the failure
        MPI_Abort(MPI_COMM_WORLD, errorCode);
    }
}

int main(int argc, char** argv) {
    // MPI_Init: Initialize MPI environment - MUST be called before any other MPI functions
    // Takes command line arguments so MPI can process its own flags (e.g., --oversubscribe)
    int err = MPI_Init(&argc, &argv);
    check_mpi_error(err, "MPI_Init");

    int rank, size;
    // MPI_Comm_rank: Get my unique process ID (rank) within MPI_COMM_WORLD communicator
    // If you run "mpirun -n 4", this sets rank to 0, 1, 2, or 3 for each process
    err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    check_mpi_error(err, "MPI_Comm_rank");
    // MPI_Comm_size: Get total number of processes in MPI_COMM_WORLD communicator
    // If you run "mpirun -n 4", this sets size = 4 on all processes
    err = MPI_Comm_size(MPI_COMM_WORLD, &size);
    check_mpi_error(err, "MPI_Comm_size");

    // GLOBAL START TIME: used for absolute timing from program launch
    auto globalStart = std::chrono::high_resolution_clock::now();

    // Total dataset: 10 million samples (synthetic data, no CSV files)
    constexpr std::size_t TOTAL_DATA_SIZE = 10'000'000;

    if (rank == 0) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "=== Part 1: BASELINE - Synthetic Data + Send/Receive ===\n\n";
        std::cout << "Total dataset size: " << TOTAL_DATA_SIZE << " samples\n";
        std::cout << "Number of MPI processes: " << size << "\n\n";
    }

    // Compute how much data this process should handle
    std::size_t localSize = TOTAL_DATA_SIZE / size;
    std::size_t globalOffset = rank * localSize;
    
    // Last process handles any remainder
    if (rank == size - 1) {
        localSize += TOTAL_DATA_SIZE % size;
    }

    if (rank == 0) {
        std::cout << "=== Data Partitioning (BLOCK strategy) ===\n";
        std::cout << "Each rank generates its portion independently\n\n";
    }
    
    // Each process prints its partition
    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::size_t myLocalSize = (r == size - 1) ? 
                (TOTAL_DATA_SIZE / size + TOTAL_DATA_SIZE % size) : 
                (TOTAL_DATA_SIZE / size);
            std::size_t myOffset = r * (TOTAL_DATA_SIZE / size);
            std::cout << "  Rank " << r << ": generates synthetic data for indices [" << myOffset << " - " 
                      << (myOffset + myLocalSize - 1) << "] (" 
                      << myLocalSize << " samples, ~" << (myLocalSize * sizeof(WeatherSample) / 1'000'000) 
                      << " MB in memory)\n";
        }
        // MPI_Barrier: Synchronization point - all processes wait here until everyone arrives
        // Ensures clean ordered output (rank 0 prints, then rank 1, etc.)
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    if (rank == 0) {
        std::cout << "\n";
    }

    // Each process creates and loads its own local dataset
    WeatherDataset localDataset(localSize);
    WeatherLoader loader;

    auto loadStart = std::chrono::high_resolution_clock::now();
    localDataset.loadDataset(loader, globalOffset);
    auto loadEnd = std::chrono::high_resolution_clock::now();
    
    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();
    auto loadAbsoluteMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - globalStart).count();

    // Each process computes its local partial sum
    auto computeStart = std::chrono::high_resolution_clock::now();
    
    double localTotal = 0.0;
    for (std::size_t i = 0; i < localSize; ++i) {
        localTotal += localDataset.at(i).getTemperature() + 0.01 * localDataset.at(i).getHumidity();
    }
    
    auto computeEnd = std::chrono::high_resolution_clock::now();
    auto computeMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - computeStart).count();
    auto computeAbsoluteMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - globalStart).count();

    // Print local results
    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::cout << "[t=" << computeAbsoluteMs << "ms] Rank " << rank << " computed local total: " 
                      << std::fixed << std::setprecision(2) << localTotal 
                      << " (load: " << loadMs << " ms, compute: " << computeMs << " ms)\n";
        }
        // MPI_Barrier: Wait for all processes before starting communication phase
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n=== Communication Phase: Send/Receive ===\n\n";
    }

    // ========================================
    // COMMUNICATION PATTERN: Point-to-Point Send/Receive
    // ========================================
    // Rank 0 does sequential receives from all workers (O(N) complexity)
    // Each MPI_Recv blocks until matching MPI_Send arrives
    double globalTotal = 0.0;  // Combine results here (only rank 0 uses this)
    
    if (rank == 0) {
        // ROOT PROCESS: Collect and combine results from all workers
        globalTotal = localTotal;  // Start with my own result
        std::cout << "Rank 0: starting with own partial = " << localTotal << "\n";
        
        // Send/Recv is BLOCKING:
        // - Sender waits until data is sent
        // - Receiver waits until data arrives
        // Must Recv from each worker in order: rank 1, then 2, then 3...
        for (int r = 1; r < size; ++r) {
            double receivedPartial;
            // MPI_Recv: BLOCKING receive - waits until data arrives from sender
            // Parameters: buffer (&receivedPartial), count (1), datatype (MPI_DOUBLE),
            //             source rank (r), message tag (0), communicator, status
            // BLOCKS until rank r sends matching message with MPI_Send
            MPI_Recv(&receivedPartial, 1, MPI_DOUBLE, r, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            std::cout << "Rank 0: received partial from Rank " << r << " = " << receivedPartial << "\n";
            globalTotal += receivedPartial;  // Accumulate result
        }
        
        std::cout << "\n=== Final Results ===\n";
        std::cout << "Global total: " << globalTotal << "\n";
    } else {
        // WORKER PROCESSES: Send partial result to rank 0
        // MPI_Send: BLOCKING send - waits until data is safely sent to receiver
        // Parameters: buffer (&localTotal), count (1), datatype (MPI_DOUBLE),
        //             destination rank (0), message tag (0), communicator
        // BLOCKS until rank 0 calls matching MPI_Recv with same message tag
        MPI_Send(&localTotal, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    // MPI_Finalize: Clean up MPI environment - MUST be called before program exits
    // After this, no more MPI functions can be called
    MPI_Finalize();
    return 0;
}
