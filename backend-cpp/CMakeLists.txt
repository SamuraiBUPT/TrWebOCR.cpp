# CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Set the project name
project(TrWebOCRcpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the current directory for header files
include_directories(${CMAKE_SOURCE_DIR})

# Add the executable target (main.cc)
add_executable(main 
               csrc/main.cc
               csrc/tr.h)

# Link the shared library (libtr.so)
target_link_libraries(main ${CMAKE_SOURCE_DIR}/tr/libtr.so)