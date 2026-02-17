#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <limits>
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
// CSVReader: reads weather data from CSV file
// ============================================================
class CSVReader {
public:
    CSVReader() = default;
    ~CSVReader() = default;
    
    std::size_t loadFromCSV(const std::string& filename, std::vector<WeatherSample>& samples,
                           std::size_t startRow = 0, std::size_t endRow = std::numeric_limits<std::size_t>::max()) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error: Cannot open " << filename << "\n";
            return 0;
        }

        std::string line;
        // Skip header line
        std::getline(file, line);
        
        std::size_t count = 0;
        std::size_t lineIndex = 0;
        while (std::getline(file, line)) {
            if (lineIndex >= startRow && lineIndex < endRow) {
                WeatherSample sample;
                if (parseLine(line, sample)) {
                    samples.push_back(sample);
                    count++;
                }
            }
            lineIndex++;
            if (lineIndex >= endRow) break;  // Stop reading after endRow
        }
        
        return count;
    }

private:
    bool parseLine(const std::string& line, WeatherSample& sample) {
        std::istringstream ss(line);
        std::string token;
        
        int day, hour;
        float temp, humidity;
        
        if (std::getline(ss, token, ',')) day = std::stoi(token);
        else return false;
        if (std::getline(ss, token, ',')) hour = std::stoi(token);
        else return false;
        if (std::getline(ss, token, ',')) temp = std::stof(token);
        else return false;
        if (std::getline(ss, token, ',')) humidity = std::stof(token);
        else return false;
        
        sample.setDayIndex(day);
        sample.setHourOfDay(hour);
        sample.setTemperature(temp);
        sample.setHumidity(humidity);
        
        return true;
    }
};

// ============================================================
// MAIN: Part 3 - SHARED FILE + BROADCAST + SEND/RECV
// ============================================================
int main(int argc, char** argv) {
    // MPI_Init: Initialize MPI environment
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    auto globalStart = std::chrono::high_resolution_clock::now();

    if (rank == 0) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "=== Part 3: SHARED FILE + Broadcast ===\n\n";
        std::cout << "Number of MPI processes: " << size << "\n\n";
    }

    // Configuration parameters
    char csvFilename[100] = "weather_data_full.csv";
    
    if (rank == 0) {
        std::cout << "Broadcasting configuration to all processes...\n\n";
    }

    // MPI_Bcast: Tree-based broadcast from rank 0 to all processes
    MPI_Bcast(csvFilename, 100, MPI_CHAR, 0, MPI_COMM_WORLD);

    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::cout << "Rank " << r << " received: file = \"" << csvFilename << "\"\n";
        }
        // MPI_Barrier: Synchronization point
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n";
    }

    // ========================================
    // Data loading from SHARED CSV file
    // ========================================
    const std::size_t TOTAL_ROWS = 10'000'000;
    std::size_t rowsPerRank = TOTAL_ROWS / size;
    std::size_t startRow = rank * rowsPerRank;
    std::size_t endRow = (rank == size - 1) ? TOTAL_ROWS : (rank + 1) * rowsPerRank;

    if (rank == 0) {
        std::cout << "=== Loading from Shared CSV File ===\n";
        std::cout << "File: weather_data_full.csv (500 MB)\n\n";
    }

    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::cout << "Rank " << r << " reading rows [" << startRow << " - " << (endRow - 1) << "]\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n";
    }

    std::vector<WeatherSample> localData;
    CSVReader reader;

    auto loadStart = std::chrono::high_resolution_clock::now();
    std::size_t localSize = reader.loadFromCSV(csvFilename, localData, startRow, endRow);
    auto loadEnd = std::chrono::high_resolution_clock::now();

    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();
    auto loadAbsoluteMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - globalStart).count();

    // Compute local total
    auto computeStart = std::chrono::high_resolution_clock::now();
    double localTotal = 0.0;
    for (const auto& sample : localData) {
        localTotal += sample.getTemperature() + 0.01 * sample.getHumidity();
    }
    auto computeEnd = std::chrono::high_resolution_clock::now();
    auto computeMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - computeStart).count();

    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::cout << "[t=" << loadAbsoluteMs << "ms] Rank " << r << " loaded " << localSize 
                      << " samples, local_total = " << localTotal 
                      << " (load: " << loadMs << " ms, compute: " << computeMs << " ms)\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n=== Reduction Phase (Send/Receive) ===\n\n";
    }

    // Gather results
    double globalTotal = 0.0;
    if (rank == 0) {
        globalTotal = localTotal;
        for (int r = 1; r < size; ++r) {
            double receivedPartial;
            // MPI_Recv: BLOCKING receive
            MPI_Recv(&receivedPartial, 1, MPI_DOUBLE, r, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            globalTotal += receivedPartial;
        }
        
        std::cout << "=== Final Results ===\n";
        std::cout << "Global total: " << globalTotal << "\n";
        
    } else {
        // MPI_Send: BLOCKING send
        MPI_Send(&localTotal, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    // MPI_Finalize: Clean up MPI environment
    MPI_Finalize();
    return 0;
}
