cmake_minimum_required(VERSION 3.20)

# Project configuration
project(LFIXE VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(include)

# Automatically find all .cpp files in src directory
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Set target properties for the main executable
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Enable testing
enable_testing()

# Set policy for FetchContent timestamps
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Download and configure GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Find all test files
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

# Create test executable
if(TEST_SOURCES)
    add_executable(tests ${TEST_SOURCES})
    
    # Link GoogleTest libraries
    target_link_libraries(tests
        GTest::gtest_main
        GTest::gmock_main
    )
    
    # Include directories for tests
    target_include_directories(tests PRIVATE include)
    target_compile_features(tests PRIVATE cxx_std_23)
    
    # Discover and register tests
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    if(TARGET tests)
        target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    if(TARGET tests)
        target_compile_options(tests PRIVATE /W4)
    endif()
endif()

# Build type specific options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    if(TARGET tests)
        target_compile_definitions(tests PRIVATE DEBUG)
    endif()
endif()

# Print configuration summary
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source files found: ${SOURCES}")
if(TEST_SOURCES)
    message(STATUS "Test files found: ${TEST_SOURCES}")
endif()