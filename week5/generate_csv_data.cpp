#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>

// ============================================================
// CSV Data Generator for MPI Lab
// ============================================================
// Generates weather data CSV files:
//   - Multiple partitioned files (for Parts 3-5)
//   - Single large file (for Part 6)
// ============================================================

void generateWeatherRow(std::ofstream& file, std::size_t rowIndex) {
    int day = static_cast<int>(rowIndex / 24);
    int hour = static_cast<int>(rowIndex % 24);

    // Simple LCG random number generator
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

    // CSV format: day,hour,temperature,humidity
    file << day << "," << hour << "," 
         << std::fixed << std::setprecision(2) 
         << temperature << "," << humidity << "\n";
}

void generatePartitionedFiles(std::size_t totalRows, int numPartitions) {
    std::cout << "Generating " << numPartitions << " partitioned CSV files...\n";
    std::cout << "Total rows: " << totalRows << "\n";
    std::cout << "Rows per partition: " << (totalRows / numPartitions) << "\n\n";

    std::size_t rowsPerPartition = totalRows / numPartitions;
    
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int p = 0; p < numPartitions; ++p) {
        std::string filename = "weather_data_part" + std::to_string(p) + ".csv";
        std::ofstream file(filename);
        
        if (!file) {
            std::cerr << "Error: Cannot create " << filename << "\n";
            return;
        }

        // Write CSV header
        file << "day,hour,temperature,humidity\n";

        std::size_t startRow = p * rowsPerPartition;
        std::size_t endRow = (p == numPartitions - 1) ? totalRows : (p + 1) * rowsPerPartition;
        
        std::cout << "Writing " << filename << " (rows " << startRow << " - " << (endRow - 1) << ")...\n";

        for (std::size_t i = startRow; i < endRow; ++i) {
            generateWeatherRow(file, i);
        }

        file.close();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "\nPartitioned files generated in " << durationMs << " ms\n\n";
}

void generateSingleFile(int numPartitions) {
    std::string filename = "weather_data_full.csv";
    std::cout << "Combining partitioned files into: " << filename << "\n\n";

    std::ofstream outFile(filename);
    
    if (!outFile) {
        std::cerr << "Error: Cannot create " << filename << "\n";
        return;
    }

    // Write CSV header
    outFile << "day,hour,temperature,humidity\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    // Combine all partition files
    for (int p = 0; p < numPartitions; ++p) {
        std::string partFilename = "weather_data_part" + std::to_string(p) + ".csv";
        std::ifstream inFile(partFilename);
        
        if (!inFile) {
            std::cerr << "Error: Cannot open " << partFilename << "\n";
            return;
        }

        std::cout << "  Reading " << partFilename << "...\n";

        std::string line;
        // Skip header from partition file
        std::getline(inFile, line);
        
        // Copy all data lines
        while (std::getline(inFile, line)) {
            outFile << line << "\n";
        }

        inFile.close();
    }

    outFile.close();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "\nCombined file created in " << durationMs << " ms\n\n";
}

int main(int argc, char** argv) {
    std::cout << "=== Weather Data CSV Generator ===\n\n";

    // Configuration
    constexpr std::size_t TOTAL_ROWS = 10'000'000;  // 10 million rows
    constexpr int NUM_PARTITIONS = 4;                // For 4 MPI processes

    std::cout << "This will generate:\n";
    std::cout << "  - 4 partitioned CSV files (for Parts 3-5)\n";
    std::cout << "  - 1 combined CSV file from partitions (for Part 6)\n\n";
    std::cout << "Total data: " << TOTAL_ROWS << " weather samples\n";
    std::cout << "Estimated disk space: ~500 MB\n\n";

    std::cout << "Continue? (y/n): ";
    char response;
    std::cin >> response;

    if (response != 'y' && response != 'Y') {
        std::cout << "Cancelled.\n";
        return 0;
    }

    std::cout << "\n";

    // Generate partitioned files (for Parts 3-5)
    generatePartitionedFiles(TOTAL_ROWS, NUM_PARTITIONS);

    // Combine partitioned files into single file (for Part 6)
    generateSingleFile(NUM_PARTITIONS);

    std::cout << "=== Generation Complete ===\n\n";
    return 0;
}
