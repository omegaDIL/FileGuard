// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <benchmark/benchmark.h>

module FileGuard;
import :Streams;

import std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File creation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void fileCreationSTLNoFile(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench0.txt" };

    std::size_t i{ 0 };
    for (auto _ : state)
    {
        const std::filesystem::path filename{ std::to_string(i++) + s_path.string() };

        std::ofstream stream{ filename, std::ios::app };
    }

    for (size_t j{ 0 }; j < i; ++j)
        std::filesystem::remove(std::to_string(j) + s_path.string());
}

static void fileCreationFileGuardNoFile(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench1.txt" };

    size_t i{ 0 };
    for (auto _ : state)
    {
        const std::filesystem::path filename{ std::to_string(i++) + s_path.string() };

        auto result{ FileGuard::createFile(filename) };
        benchmark::DoNotOptimize(result);
    }

    for (size_t j{ 0 }; j < i; ++j)
        std::filesystem::remove(std::to_string(j) + s_path.string());
}

static void fileCreationSTLWithFile(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench2.txt" };
    FileGuard::createFile(s_path);

    for (auto _ : state)
    {
        std::ofstream stream{ s_path, std::ios::app };
    }

    std::filesystem::remove(s_path);
}

static void fileCreationFileGuardWithFile(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench3.txt" };
    FileGuard::createFile(s_path);

    for (auto _ : state)
    {
        auto result{ FileGuard::createFile(s_path) };
        benchmark::DoNotOptimize(result);
    }

    std::filesystem::remove(s_path);
}

BENCHMARK(fileCreationSTLNoFile);
BENCHMARK(fileCreationFileGuardNoFile);
BENCHMARK(fileCreationSTLWithFile);
BENCHMARK(fileCreationFileGuardWithFile);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File creation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open reading file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void openReadingFileSTL(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench4.txt" };
    FileGuard::createFile(s_path);
    
    for (auto _ : state)
    {
        std::ifstream stream{ s_path, std::ios::in };
    }

    std::filesystem::remove(s_path);
}

static void openReadingFileFileGuardINTERN(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench5.txt" };
    FileGuard::createFile(s_path);

    for (auto _ : state)
    {
        auto result{ FileGuard::openReadingFile(s_path) };
        benchmark::DoNotOptimize(result);
    }
   
    std::filesystem::remove(s_path);
}

BENCHMARK(openReadingFileSTL);
BENCHMARK(openReadingFileFileGuardINTERN);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open reading file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open writing file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void openWritingFileSTL(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench6.txt" };
    FileGuard::createFile(s_path);

    for (auto _ : state)
    {
        std::ofstream stream{ s_path, std::ios::out | std::ios::in }; // Has side effect: no deletion by compiler
    }

    std::filesystem::remove(s_path);
}

static void openWritingFileFileGuardINTERN(benchmark::State& state)
{
    const std::filesystem::path s_path{ "bench7.txt" };
    FileGuard::createFile(s_path);

    for (auto _ : state)
    {
        auto result{ FileGuard::openWritingFile(s_path) };
        benchmark::DoNotOptimize(result);
    }

    std::filesystem::remove(s_path);
}

BENCHMARK(openWritingFileSTL);
BENCHMARK(openWritingFileFileGuardINTERN);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open writing file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading in file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void readingInFileSTL(benchmark::State& state)
{
    const std::int64_t bytes_to_read{ state.range(0) };
    std::string source_data(bytes_to_read, 'A');

    const std::filesystem::path s_path{ "bench8.txt" };
    FileGuard::createFile(s_path);
    auto file{ *FileGuard::openIOFile(s_path) };
    FileGuard::overwriteStream(file, source_data);
    file.flush();
    file.clear();
    file.seekg(0, std::ios::beg);

    for (auto _ : state)
    {
        std::string result(bytes_to_read, ' ');
        file.read(result.data(), bytes_to_read);
        benchmark::DoNotOptimize(result);
        file.seekg(0, std::ios::beg);
    }

    state.SetBytesProcessed(state.iterations() * bytes_to_read);

    file.close();
    std::filesystem::remove(s_path);
}

static void readingInFileFileGuardINTERN(benchmark::State& state)
{
    const std::int64_t bytes_to_read{ state.range(0) };
    std::string source_data(bytes_to_read, 'A');

    const std::filesystem::path s_path{ "bench9.txt" };
    FileGuard::createFile(s_path);
    auto file{ *FileGuard::openIOFile(s_path) };
    FileGuard::overwriteStream(file, source_data);
    file.flush();
    file.clear();
    file.seekg(0, std::ios::beg);

    for (auto _ : state)
    {
        auto result = FileGuard::readStream(file, bytes_to_read);
        benchmark::DoNotOptimize(result);
        file.seekg(0, std::ios::beg);
    }

    state.SetBytesProcessed(state.iterations() * bytes_to_read);

    file.close();
    std::filesystem::remove(s_path);
}

BENCHMARK(readingInFileSTL)->RangeMultiplier(4)->Range(64, 4 * 1024 * 1024);
BENCHMARK(readingInFileFileGuardINTERN)->RangeMultiplier(4)->Range(64, 4 * 1024 * 1024);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading in file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writing in file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void writingInFileSTL(benchmark::State& state)
{
    const std::int64_t bytes_to_write{ state.range(0) };
    const std::string data_to_write(bytes_to_write, 'B');
    const std::string_view view_to_write(data_to_write);

    const std::filesystem::path s_path{ "bench10.txt" };
    FileGuard::createFile(s_path);
    auto file{ *FileGuard::openWritingFile(s_path) };

    for (auto _ : state)
    {
        file << view_to_write;
        file.seekp(0, std::ios::beg);
    }

    state.SetBytesProcessed(state.iterations() * bytes_to_write);

    file.close();
    std::filesystem::remove(s_path);
}

static void writingInFileFileGuardINTERN(benchmark::State& state)
{
    const std::int64_t bytes_to_write{ state.range(0) };
    const std::string data_to_write(bytes_to_write, 'B');
    const std::string_view view_to_write(data_to_write);

    const std::filesystem::path s_path{ "bench11.txt" };
    FileGuard::createFile(s_path);
    auto file{ *FileGuard::openWritingFile(s_path) };

    for (auto _ : state) 
    {
        auto err{ FileGuard::overwriteStream(file, view_to_write, bytes_to_write) };
        benchmark::DoNotOptimize(err);
        file.seekp(0, std::ios::beg);
    }

    state.SetBytesProcessed(state.iterations() * bytes_to_write);

    file.close();
    std::filesystem::remove(s_path);
}

BENCHMARK(writingInFileSTL)->RangeMultiplier(4)->Range(64, 4 * 1024 * 1024);
BENCHMARK(writingInFileFileGuardINTERN)->RangeMultiplier(4)->Range(64, 4 * 1024 * 1024);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writing in file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Closing writing file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void flushingFileSTL(benchmark::State& state)
{
    const std::string data_to_write(65536, 'A');
    const std::string_view view_to_write(data_to_write);

    const std::filesystem::path s_path{ "bench12.txt" };
    FileGuard::createFile(s_path);
    auto file{ *FileGuard::openWritingFile(s_path) };

    for (auto _ : state)
    {
        file << view_to_write;
        file.flush();
    }

    file.close();
    std::filesystem::remove(s_path);
}

static void flushingFileFileGuardINTERN(benchmark::State& state)
{
    const std::string data_to_write(65536, 'B');
    const std::string_view view_to_write(data_to_write);

    const std::filesystem::path s_path{ "bench13.txt" };
    FileGuard::createFile(s_path);
    auto file{ *FileGuard::openWritingFile(s_path) };

    for (auto _ : state)
    {
        file << view_to_write;
        FileGuard::flushWritingFile(file);
    }

    file.close();
    std::filesystem::remove(s_path);
}

BENCHMARK(flushingFileSTL);
BENCHMARK(flushingFileFileGuardINTERN);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Closing writing file.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zeroize files.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void zeroizeFileFileGuard(benchmark::State& state)
{
    const std::int64_t bytes_to_write{ state.range(0) };
    const std::string data_to_write(bytes_to_write, 'B');
    const std::string_view view_to_write(data_to_write);

    const std::filesystem::path s_path{ "bench14.txt" };
    FileGuard::createFile(s_path);
    std::filesystem::resize_file(s_path, 0);
    
    {
        auto file{ *FileGuard::openWritingFile(s_path) };
        FileGuard::overwriteStream(file, view_to_write);
    }

    for (auto _ : state)
    {
        // Opens and delete a stream to file which lowers performance. Not comparable with writingInFile but with
        // closingAfterWritingFile().
        auto err{ FileGuard::basicFileZeroing(s_path) };
        benchmark::DoNotOptimize(err); 
    }   

    state.SetBytesProcessed(state.iterations() * bytes_to_write);

    std::filesystem::remove(s_path);
}

BENCHMARK(zeroizeFileFileGuard)->RangeMultiplier(4)->Range(64, 4 * 1024 * 1024);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Zeroize files.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// - openIOFile() benchmarks would be similar to what openWritingFile() and openReadingFile() give
// - closeWritingFile() benchmarks would be similar to what flushWritingFile() give, except it has to close the I/O
//   interface. This overhead is similar to the STL function. closeWritingFile() simply wraps another function the
//   same.