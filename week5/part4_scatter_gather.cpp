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
            if (lineIndex >= endRow) break;
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
// MAIN: Part 4 - SHARED FILE + SCATTER/GATHER
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
        std::cout << "=== Part 4: SHARED FILE + Scatter/Gather ===\n\n";
        std::cout << "Number of MPI processes: " << size << "\n\n";
    }

    // ========================================
    // Data loading from SHARED CSV file
    // ========================================
    // Data loading from SHARED CSV file (rank 0 reads, then scatters)
    // ========================================
    const std::size_t TOTAL_ROWS = 10'000'000;
    std::size_t rowsPerRank = TOTAL_ROWS / size;
    std::size_t remainder = TOTAL_ROWS % size;
    std::size_t localRows = rowsPerRank + ((rank == size - 1) ? remainder : 0);

    if (rank == 0) {
        std::cout << "=== Loading from Shared CSV File ===\n";
        std::cout << "File: weather_data_full.csv (500 MB, 10M rows)\n";
        std::cout << "Rank 0 reads full file, then scatters rows to all ranks\n\n";
    }

    std::vector<WeatherSample> allData;
    std::vector<WeatherSample> localData(localRows);
    CSVReader reader;

    auto loadStart = std::chrono::high_resolution_clock::now();
    if (rank == 0) {
        reader.loadFromCSV("weather_data_full.csv", allData, 0, TOTAL_ROWS);
    }

    int bytesPerSample = static_cast<int>(sizeof(WeatherSample));
    std::vector<int> sendcounts;
    std::vector<int> displs;
    if (rank == 0) {
        sendcounts.resize(size);
        displs.resize(size);
        int offset = 0;
        for (int r = 0; r < size; ++r) {
            std::size_t count = rowsPerRank + ((r == size - 1) ? remainder : 0);
            sendcounts[r] = static_cast<int>(count * bytesPerSample);
            displs[r] = offset;
            offset += sendcounts[r];
        }
    }

    // MPI_Scatterv: root scatters variable-sized chunks to all ranks
    // sendcounts/displs are in bytes because we use MPI_BYTE
    // Each rank receives localRows samples into localData
    MPI_Scatterv(
        rank == 0 ? allData.data() : nullptr,
        rank == 0 ? sendcounts.data() : nullptr,
        rank == 0 ? displs.data() : nullptr,
        MPI_BYTE,
        localData.data(),
        static_cast<int>(localRows * bytesPerSample),
        MPI_BYTE,
        0,
        MPI_COMM_WORLD
    );

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
            std::cout << "Rank " << r << ": loaded " << localRows 
                      << " samples, local_total = " << localTotal 
                      << " (load: " << loadMs << " ms, compute: " << computeMs << " ms)\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n=== Gather Phase (O(log N) tree-based collection) ===\n\n";
    }

    // Gather results
    std::vector<double> allTotals;
    if (rank == 0) {
        allTotals.resize(size);
    }

    // MPI_Gather: Collect values from all processes (O(log N) tree-based)
    MPI_Gather(
        &localTotal,
        1,
        MPI_DOUBLE,
        allTotals.data(),
        1,
        MPI_DOUBLE,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        double globalTotal = 0.0;
        for (int r = 0; r < size; ++r) {
            globalTotal += allTotals[r];
        }
        
        std::cout << "=== Final Results ===\n";
        std::cout << "Global total: " << globalTotal << "\n";
        
    }

    // MPI_Finalize: Clean up MPI environment
    MPI_Finalize();
    return 0;
}
