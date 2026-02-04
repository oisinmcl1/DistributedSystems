cmake_minimum_required(VERSION 4.1)
project(week4)

set(CMAKE_CXX_STANDARD 20)

# Build each part as a separate executable
add_executable(part1_sequential part1_sequential.cpp)
add_executable(part2_parallel part2_parallel.cpp)
add_executable(part3_race_condition part3_race_condition.cpp)
add_executable(part4_mutex_fixed part4_mutex_fixed.cpp)
add_executable(part5_atomic part5_atomic.cpp)

# Link pthread library (needed for std::thread)
target_link_libraries(part2_parallel pthread)
target_link_libraries(part3_race_condition pthread)
target_link_libraries(part4_mutex_fixed pthread)
target_link_libraries(part5_atomic pthread)