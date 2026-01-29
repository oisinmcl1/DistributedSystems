cmake_minimum_required(VERSION 4.1)
project(week3)

set(CMAKE_CXX_STANDARD 23)

# Create separate executables for each part
add_executable(message_system message_system.cpp)
add_executable(part1_manual part1_manual.cpp)
add_executable(part2_5_copy_problem part2.5_copy_problem.cpp)
add_executable(part2_destructors part2_destructors.cpp)
add_executable(part3_unique_ptr part3_unique_ptr.cpp)
add_executable(part4_shared_ptr part4_shared_ptr.cpp)