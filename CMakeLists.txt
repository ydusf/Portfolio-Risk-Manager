cmake_minimum_required(VERSION 3.20)

project(PortfolioRiskManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE_DIR})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -fno-omit-frame-pointer>
    $<$<CONFIG:Release>:
        -O3
        -march=native
        -flto
        -ffast-math
        -funroll-loops
        -fomit-frame-pointer
        -finline-functions
        -fdata-sections
        -ffunction-sections
        
        $<$<CXX_COMPILER_ID:GNU>:
            -mtune=native
            -fprefetch-loop-arrays
            -ftracer
            -floop-parallelize-all
            -ftree-vectorize
            -fvect-cost-model=dynamic
            -fmerge-all-constants
            -fgcse-after-reload
            -fipa-cp-clone
            -floop-interchange
            -floop-unroll-and-jam
            -fpeel-loops
            -fpredictive-commoning
            -fsplit-loops
            -ftree-loop-distribution
            -ftree-loop-distribute-patterns
            -ftree-partial-pre
            -funswitch-loops
            -fversion-loops-for-strides
            -fweb
        >
        
        $<$<CXX_COMPILER_ID:Clang>:
            -fvectorize
            -fslp-vectorize
            -fmerge-all-constants
        >
    >
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:
        -flto
        $<$<PLATFORM_ID:Linux>:
            -Wl,--gc-sections
            -Wl,-O3
            -Wl,--as-needed
            -Wl,--sort-common
        >
        $<$<PLATFORM_ID:Darwin>:
            -Wl,-dead_strip
            -Wl,-O3
        >
    >
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    $<$<CONFIG:Release>:
        NDEBUG
        _FORTIFY_SOURCE=2
    >
)

option(USE_PGO "Enable Profile-Guided Optimization" OFF)
if(USE_PGO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -fprofile-generate)
    target_link_options(${PROJECT_NAME} PRIVATE -fprofile-generate)
endif()

enable_testing()
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
)
FetchContent_MakeAvailable(eigen)

find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})


file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

if(TEST_SOURCES)
    add_library(portfolio STATIC ${SOURCES})
    target_include_directories(portfolio PUBLIC ${PROJECT_INCLUDE_DIR})
    target_link_libraries(portfolio PUBLIC Eigen3::Eigen)
    target_compile_definitions(portfolio PUBLIC PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

    add_executable(tests ${TEST_SOURCES})
    target_include_directories(tests PRIVATE ${PROJECT_INCLUDE_DIR})
    target_compile_features(tests PRIVATE cxx_std_23)
    
    target_link_libraries(tests PRIVATE
        GTest::gmock_main
        portfolio
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -O2)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(tests PRIVATE /W4 /O2)
    endif()

    include(GoogleTest)
    gtest_discover_tests(tests)
endif()


set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:MinSizeRel>:-Os -march=native -flto -fdata-sections -ffunction-sections>
)
target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:MinSizeRel>:
        -flto
        $<$<PLATFORM_ID:Linux>:-Wl,--gc-sections -s>
        $<$<PLATFORM_ID:Darwin>:-Wl,-dead_strip -Wl,-S>
    >
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:RelWithDebInfo>:-O2 -g -march=native -fno-omit-frame-pointer>
)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Source files found: ${SOURCES}")
if(TEST_SOURCES)
    message(STATUS "Test files found: ${TEST_SOURCES}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "")
    message(STATUS "=== PERFORMANCE OPTIMIZATIONS ENABLED ===")
    message(STATUS "- Link-time optimization (LTO)")
    message(STATUS "- Native CPU architecture targeting")
    message(STATUS "- Aggressive loop optimizations")
    message(STATUS "- Fast math operations")
    message(STATUS "- Dead code elimination")
    message(STATUS "- Function inlining")
    message(STATUS "")
    message(STATUS "For maximum performance:")
    message(STATUS "1. Use 'make -j$(nproc)' or 'ninja' for parallel builds")
    message(STATUS "2. Consider enabling PGO with -DUSE_PGO=ON")
    message(STATUS "3. Ensure your CPU supports the generated instructions")
    message(STATUS "")
endif()