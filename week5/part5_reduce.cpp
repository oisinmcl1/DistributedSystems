#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
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
// CSVReader: reads weather data from CSV file
// ============================================================
class CSVReader {
public:
    CSVReader() = default;
    ~CSVReader() = default;
    
    std::size_t loadFromCSV(const std::string& filename, std::vector<WeatherSample>& samples) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error: Cannot open " << filename << "\n";
            return 0;
        }

        std::string line;
        std::getline(file, line);
        
        std::size_t count = 0;
        while (std::getline(file, line)) {
            WeatherSample sample;
            if (parseLine(line, sample)) {
                samples.push_back(sample);
                count++;
            }
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
// MAIN: Part 5 - PARTITIONED FILES + MPI_Reduce (OPTIMAL)
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
        std::cout << "=== Part 5: PARTITIONED FILES + MPI_Reduce ===\n\n";
        std::cout << "Number of MPI processes: " << size << "\n\n";
    }

    // ========================================
    // Data loading from PARTITIONED CSV files
    // ========================================
    if (rank == 0) {
        std::cout << "=== Loading from Partitioned CSV Files ===\n";
        std::cout << "File: weather_data_part[0-3].csv (each ~125 MB)\n";
        std::cout << "Strategy: Each process reads its OWN file (parallel I/O)\n\n";
    }

    std::string filename = "weather_data_part" + std::to_string(rank) + ".csv";
    
    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::cout << "Rank " << r << " reading " << filename << " (~125 MB)\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n";
    }

    // Load data
    std::vector<WeatherSample> localData;
    CSVReader reader;

    auto loadStart = std::chrono::high_resolution_clock::now();
    std::size_t localSize = reader.loadFromCSV(filename, localData);
    auto loadEnd = std::chrono::high_resolution_clock::now();

    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();

    // Compute
    auto computeStart = std::chrono::high_resolution_clock::now();
    double localTotal = 0.0;
    for (const auto& sample : localData) {
        localTotal += sample.getTemperature() + 0.01 * sample.getHumidity();
    }
    auto computeEnd = std::chrono::high_resolution_clock::now();
    auto computeMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - computeStart).count();
    auto computeAbsoluteMs = std::chrono::duration_cast<std::chrono::milliseconds>(computeEnd - globalStart).count();

    for (int r = 0; r < size; ++r) {
        if (rank == r) {
            std::cout << "[t=" << computeAbsoluteMs << "ms] Rank " << rank << ": local_total = " 
                      << localTotal << " (load: " << loadMs << " ms, compute: " << computeMs << " ms)\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (rank == 0) {
        std::cout << "\n=== REDUCE Phase: MPI_Reduce (O(log N) tree-based) ===\n\n";
    }

    auto reduceStart = std::chrono::high_resolution_clock::now();

    double globalTotal = 0.0;
    // MPI_Reduce: Tree-based reduction (MOST EFFICIENT communication)
    MPI_Reduce(
        &localTotal,
        &globalTotal,
        1,
        MPI_DOUBLE,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    auto reduceEnd = std::chrono::high_resolution_clock::now();
    auto reduceMs = std::chrono::duration_cast<std::chrono::milliseconds>(reduceEnd - reduceStart).count();

    if (rank == 0) {
        std::cout << "MPI_Reduce completed in " << reduceMs << " ms\n\n";
        
        std::cout << "=== Final Results ===\n";
        std::cout << "Global total: " << globalTotal << "\n";
    }

    // MPI_Finalize: Clean up MPI environment
    MPI_Finalize();
    return 0;
}
