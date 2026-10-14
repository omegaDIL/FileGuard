# ---------------------------------------------------------------------------------------------------------------------
# The project
# ---------------------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# Used only if the project is top level. If not, this is silently ignored.
if (CMAKE_GENERATOR MATCHES "Ninja")
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "4.4.0")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
    elseif (CMAKE_VERSION VERSION_GREATER_EQUAL "4.3.0")
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
    endif ()
endif ()

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,ProgramDatabase,Embedded>")

project(FileGuard VERSION 1.0.0 LANGUAGES CXX)

option(fileGuardTopLevel "is FileGuard used as a library or as a standalone project?" ${PROJECT_IS_TOP_LEVEL})

if (fileGuardTopLevel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    if (CMAKE_GENERATOR MATCHES "Ninja" AND CMAKE_VERSION VERSION_LESS "4.3.0")
        message(FATAL_ERROR "Please use CMake version 4.3.0 or newer if you use Ninja. You are using the version '${CMAKE_VERSION}'")
    endif()

    # MSVC needs those flags
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# ---------------------------------------------------------------------------------------------------------------------
# The project
# ---------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------
# Toolchain infos
# ---------------------------------------------------------------------------------------------------------------------

if (fileGuardTopLevel)
    message(STATUS "The compiler found is '${CMAKE_CXX_COMPILER_ID}'")
    message(STATUS "The compiler version is '${CMAKE_CXX_COMPILER_VERSION}'")
    message(STATUS "The standard library found is '${CMAKE_CXX_STANDARD_LIBRARY}'")
    message(STATUS "The CMake version found is '${CMAKE_VERSION}'")
    message(STATUS "The generator found is '${CMAKE_GENERATOR}'")
endif()

# Toolchain checks
if (NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
    message(WARNING "Only Ninja and Visual Studio have been tested with FileGuard and modules/`import std;`")
endif ()

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|MSVC")
    message(WARNING "Only Clang, GCC and MSVC have been tested with FileGuard")
endif ()

if (CMAKE_GENERATOR MATCHES "Ninja")
    if (NOT "23" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
        message(WARNING "Ninja does not report `import std;` support for C++23 ")
    endif ()
endif ()

# ---------------------------------------------------------------------------------------------------------------------
# Toolchain infos
# ---------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------
# dependency management
# ---------------------------------------------------------------------------------------------------------------------

if (fileGuardTopLevel)
    include(FetchContent)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.15.0
    )
    set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "" FORCE)
    set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
            google_benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG        v1.9.5
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(Catch2 google_benchmark)

    if (TARGET benchmark)
        set_target_properties(benchmark PROPERTIES FOLDER "Dependencies/GoogleBenchmark")
    endif ()
    if (TARGET benchmark_main)
        set_target_properties(benchmark_main PROPERTIES FOLDER "Dependencies/GoogleBenchmark")
    endif ()
    if (TARGET Catch2)
        set_target_properties(Catch2 PROPERTIES FOLDER "Dependencies/Catch2")
    endif ()
    if(TARGET Catch2WithMain)
        set_target_properties(Catch2WithMain PROPERTIES FOLDER "Dependencies/Catch2")
    endif ()
endif ()

# ---------------------------------------------------------------------------------------------------------------------
# dependency management
# ---------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------
# The library target
# ---------------------------------------------------------------------------------------------------------------------

# Additional steps to support modules/`import std` with Ninja.
if (CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    set(CMAKE_CXX_MODULE_STD ON)
endif()

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

set(MODULE_INTERFACES
        modules/FileGuard/Streams.cppm
        modules/FileGuard/Encryption.cppm
        modules/FileGuard/AllOrNothing.cppm
        modules/FileGuard/AppendGuideline.cppm
        modules/FileGuard/IndexedGuideline.cppm
        modules/FileGuard/FileGuard.cppm
)

set(MODULE_IMPLEMENTATIONS
    modules/FileGuard/StreamsImpl.cpp
    modules/FileGuard/EncryptionImpl.cpp
    modules/FileGuard/AllOrNothingImpl.cpp
    modules/FileGuard/AppendGuidelineImpl.cpp
    modules/FileGuard/IndexedGuidelineImpl.cpp
)

# Define the Public Module Interfaces
target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS modules
        FILES ${MODULE_INTERFACES}
        PRIVATE
        ${MODULE_IMPLEMENTATIONS}
)

# ---------------------------------------------------------------------------------------------------------------------
# The library target
# ---------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------
# The test target
# ---------------------------------------------------------------------------------------------------------------------

if (fileGuardTopLevel)
    enable_testing()
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)

    set(TEST_SOURCES
        tests/EncryptionTests.cpp
        tests/StreamsTests.cpp
        tests/AllOrNothingTests.cpp
        tests/AppendTests.cpp
        tests/IndexedTests.cpp
    )

    add_executable(AllTests ${TEST_SOURCES})
    target_link_libraries(AllTests
        PRIVATE
            ${PROJECT_NAME}::${PROJECT_NAME}
            Catch2::Catch2WithMain
    )

    catch_discover_tests(AllTests)
endif()

# ---------------------------------------------------------------------------------------------------------------------
# The test target
# ---------------------------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------------------------
# The benchmark target
# ---------------------------------------------------------------------------------------------------------------------

if (fileGuardTopLevel)
    set(BENCH_SOURCES
        benchmarks/StreamsBenchs.cpp
        benchmarks/GuidelinesBenchs.cpp
        benchmarks/mainBenchs.cpp
    )

    add_executable(AllBenchmarks ${BENCH_SOURCES})
    target_link_libraries(AllBenchmarks
        PRIVATE
            ${PROJECT_NAME}::${PROJECT_NAME}
            benchmark::benchmark
            benchmark::benchmark_main
    )
endif()

# ---------------------------------------------------------------------------------------------------------------------
# The benchmark target
# ---------------------------------------------------------------------------------------------------------------------
