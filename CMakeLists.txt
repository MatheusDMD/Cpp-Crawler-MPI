project(crawler)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(O2_FLAG "-O2")

# Point to our own cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

# Add global definitions
add_definitions("-Wall")

#Add MPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(opt)

#create exe mpi
add_executable(crawler crawlerMPI.cpp)
target_link_libraries(crawler ${CPR_LIBRARIES} ${MPI_LIBRARIES}  "-lcurl -lboost_regex -lboost_mpi -lboost_serialization")
set_target_properties(crawlerDIS PROPERTIES COMPILE_FLAGS ${O2_FLAG} LINK_FLAGS ${O2_FLAG})
include_directories(${CPR_INCLUDE_DIRS})