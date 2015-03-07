project(bitonic_sort CXX)
cmake_minimum_required(VERSION 2.6.2)
find_package(MPI REQUIRED)

set(CMAKE_BUILD_TYPE Debug)
add_definitions("-std=c++1y")
include_directories("$ENV{HOME}/code/diy2/include")
set(Boost_INCLUDE_DIR /home/rhl/code/ctl/dependencies/boost/include)
set(Boost_LIBRARY_DIR /home/rhl/code/ctl/dependencies/boost/lib)
set(Boost_LIBRARIES ${Boost_LIBRARY_DIR}/libboost_program_options.so
${Boost_LIBRARY_DIR}/libboost_serialization.so) 
include_directories(${Boost_INCLUDE_DIR})
add_executable(bitonic bitonic_sort.cpp)
target_link_libraries(bitonic ${MPI_LIBRARIES} ${Boost_LIBRARIES})


