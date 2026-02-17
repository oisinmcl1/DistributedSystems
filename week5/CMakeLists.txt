cmake_minimum_required(VERSION 4.1)
project(week5)

set(CMAKE_CXX_STANDARD 20)

# Find MPI
find_package(MPI REQUIRED)

add_executable(in_class in_class.cpp)
target_link_libraries(in_class MPI::MPI_CXX)

add_executable(part1_send_receive part1_send_receive.cpp)
target_link_libraries(part1_send_receive MPI::MPI_CXX)

add_executable(part2_shared_sendrecv part2_shared_sendrecv.cpp)
target_link_libraries(part2_shared_sendrecv MPI::MPI_CXX)

add_executable(part3_shared_broadcast part3_shared_broadcast.cpp)
target_link_libraries(part3_shared_broadcast MPI::MPI_CXX)

add_executable(part4_scatter_gather part4_scatter_gather.cpp)
target_link_libraries(part4_scatter_gather MPI::MPI_CXX)

add_executable(part5_reduce part5_reduce.cpp)
target_link_libraries(part5_reduce MPI::MPI_CXX)
