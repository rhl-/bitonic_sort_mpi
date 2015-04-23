project(bitonic_sort CXX)
cmake_minimum_required(VERSION 2.6.2)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
add_definitions("-std=c++11 -g -Wl,-eh-frame-hdr -fno-omit-frame-pointer")
find_package(MPI REQUIRED)
set(BOOST_ROOT $ENV{HOME}) #We assume boost is installed in your homedirectory
find_package(Boost COMPONENTS mpi serialization REQUIRED)
include_directories(${MPI_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
add_executable(bitonic bitonic_sort.cpp)
target_link_libraries(bitonic ${MPI_LIBRARIES} ${Boost_LIBRARIES})

if(MPI_COMPILE_FLAGS)
  set_target_properties(bitonic PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
  set_target_properties(bitonic PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

