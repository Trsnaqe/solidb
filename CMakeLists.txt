cmake_minimum_required(VERSION 3.10)
project(SolidDB VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(include)

# Source files by component
file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB PARSER_SOURCES "src/parser/*.cpp")
file(GLOB UTIL_SOURCES "src/util/*.cpp")
file(GLOB MAIN_SOURCES "src/*.cpp")

# All source files
set(SOURCES
    ${CORE_SOURCES}
    ${PARSER_SOURCES}
    ${UTIL_SOURCES}
    ${MAIN_SOURCES}
)

# Add executable
add_executable(soliddb ${SOURCES})

# Add testing if needed
# enable_testing()
# add_subdirectory(tests) 