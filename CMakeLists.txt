cmake_minimum_required(VERSION 3.10)
project(CSVTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set UTF-8 encoding for MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/csv_processor.cpp
    src/command_parser.cpp
)

set(HEADERS
    src/csv_processor.h
    src/command_parser.h
)

# Add executable
add_executable(csvtool ${SOURCES} ${HEADERS})

# Installation
install(TARGETS csvtool DESTINATION bin)
