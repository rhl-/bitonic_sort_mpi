project(bitonic_sort CXX)
cmake_minimum_required(VERSION 2.6.2)
find_package(MPI REQUIRED)
add_definitions("-std=c++1y")
include_directories("$ENV{HOME}/code/diy2/include")
add_executable(bitonic bitonic_sort.cpp)
target_link_libraries(bitonic ${MPI_LIBRARIES})


