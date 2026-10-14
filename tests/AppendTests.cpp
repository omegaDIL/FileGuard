// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <catch2/catch_test_macros.hpp>

module FileGuard;
import :Append;

import std;

static constexpr std::string_view kData{ "Some really critical data that needs to be saved at all cost even if it launches nuclear missiles in the process!!!" };

TEST_CASE("creating an instance append", "[guideline][Append]")
{
    SECTION("opening constructor")
    {
        const std::filesystem::path path{ "append0.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::AppendGuideline append{ path };
        CHECK_FALSE(append.append("thing")); // Is valid.
        CHECK_FALSE(append.close());

        CHECK(std::filesystem::remove(path));

#ifdef NDEBUG
        CHECK_THROWS_AS(FileGuard::AppendGuideline(path), std::ios_base::failure);
#endif //NDEBUG
    }

    SECTION("writing constructor")
    {
        const std::filesystem::path path{ "append1.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::AppendGuideline append{ path, kData };
        CHECK_FALSE(append.append("thing")); // Is valid.
        CHECK_FALSE(append.close());

        CHECK(std::filesystem::remove(path));

#ifdef NDEBUG
        CHECK_THROWS_AS(FileGuard::AppendGuideline(path, kData), std::ios_base::failure);
#endif //NDEBUG
    }
        
    SECTION("default constructor")
    {
        const std::filesystem::path path{ "append2.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::AppendGuideline append{};
        CHECK_FALSE(append.open(path));
        CHECK_FALSE(append.append("thing")); // Is valid.
        CHECK_FALSE(append.close());

        CHECK(std::filesystem::remove(path));
    }

    SECTION("reopening")
    {
        const std::filesystem::path path{ "append3.txt" };
        const std::filesystem::path path2{ "append(v2)3.txt" };
        std::filesystem::remove(path);
        std::filesystem::remove(path2);
        REQUIRE_FALSE(FileGuard::createFile(path));
        REQUIRE_FALSE(FileGuard::createFile(path2));

        FileGuard::AppendGuideline append{ path, kData };
        CHECK_FALSE(append.open(path));
        CHECK_FALSE(append.append("thing"));
        CHECK_FALSE(append.openAndAppend(path, "thing"));
        CHECK_FALSE(append.append("thing"));
        CHECK_FALSE(append.ensureFlushed());
        CHECK_FALSE(append.close());

        FileGuard::AppendGuideline append1{ path };
        CHECK_FALSE(append1.openAndAppend(path, "thing"));
        CHECK_FALSE(append1.append("thing"));
        CHECK_FALSE(append1.close());
        CHECK_FALSE(append1.open(path));
        CHECK_FALSE(append1.append("thing"));
        CHECK_FALSE(append1.open(path2)); // other file.
        CHECK_FALSE(append1.append("some"));
        CHECK_FALSE(append1.close());

        CHECK(std::filesystem::remove(path));
        CHECK(std::filesystem::remove(path2));
    }
}

TEST_CASE("writing, flushing and closing append", "[guideline][Append][writing]")
{
    const std::filesystem::path path{ "append4.txt" };
    std::filesystem::remove(path);
    REQUIRE_FALSE(FileGuard::createFile(path));

    FileGuard::AppendGuideline append{};
    CHECK_FALSE(append.openAndAppend(path, "data"));
    CHECK_FALSE(append.append("data"));
    CHECK_FALSE(append.append("data2"));
    CHECK_FALSE(append.append(kData));
    CHECK_FALSE(append.append(""));
    CHECK_FALSE(append.ensureFlushed());
    CHECK_FALSE(append.append("hello world"));
    CHECK_FALSE(append.close());
    CHECK_FALSE(append.openAndAppend(path, "azerty"));
    CHECK_FALSE(append.close());
    CHECK_FALSE(append.ensureFlushed());
    CHECK_FALSE(append.open(path));
    CHECK_FALSE(append.ensureFlushed());
    CHECK_FALSE(append.append("qwerty"));
    CHECK_FALSE(append.append("bemol"));
    CHECK_FALSE(append.open(path));
    CHECK_FALSE(append.ensureFlushed());
    CHECK_FALSE(append.close());
    CHECK_FALSE(append.close());

    CHECK(std::filesystem::file_size(path) == 156);

    CHECK(std::filesystem::remove(path));
}

TEST_CASE("reading append", "[guideline][Append][reading]")
{
    const std::filesystem::path path{ "append5.txt" };
    const std::filesystem::path path2{ "append(2)5.txt" };
    std::filesystem::remove(path);
    std::filesystem::remove(path2);
    REQUIRE_FALSE(FileGuard::createFile(path));;
    REQUIRE_FALSE(FileGuard::createFile(path2));

    FileGuard::AppendGuideline append{ path };
    REQUIRE_FALSE(append.append(kData));

    auto res{ append.readAndRestore(kData.size()) };
    REQUIRE(res.has_value());
    CHECK(res.value() == kData);

    res = append.readAndRestore(kData.size());
    REQUIRE(res.has_value());
    CHECK(res.value() == kData);

    res = append.readAndRestore(kData.size());
    REQUIRE(res.has_value());
    CHECK(res.value() == kData);

    CHECK_FALSE(append.append("Hello world"));

    res = append.readAndRestore(kData.size() + 11);
    REQUIRE(res.has_value());
    CHECK(res.value().length() == kData.size() + 11);
    CHECK(res.value().ends_with('d'));

    res = append.readAndClose(kData.size() + 4);
    REQUIRE(res.has_value());
    CHECK(res.value().length() == kData.size() + 4);

    CHECK(append.append("error"));

    CHECK_FALSE(append.open(path));
    CHECK(append.readAndRestore(10).has_value());

    CHECK_FALSE(append.open(path2)); // other file.
    CHECK_FALSE(append.append("some"));
    res = append.readAndClose(4);
    REQUIRE(res.has_value());
    CHECK(res == "some");

    CHECK(std::filesystem::remove(path));
    CHECK(std::filesystem::remove(path2));
}

TEST_CASE("error handling append", "[guideline][Append]")
{
    const std::filesystem::path path{ "append6.txt" };
    const std::filesystem::path no{ "doesntExistAppend6.txt" };
    std::filesystem::remove(path);
    std::filesystem::remove(no);
    REQUIRE_FALSE(FileGuard::createFile(path));

    FileGuard::AppendGuideline append{};
    CHECK(append.append("error") == std::errc::invalid_argument);
    CHECK(append.open(no) == std::errc::no_such_file_or_directory);
    CHECK_FALSE(append.open(path));
    CHECK_FALSE(append.open(path));
    REQUIRE_FALSE(append.append(kData));
    auto err{ append.readAndRestore(kData.size() + 1) };
    REQUIRE_FALSE(err.has_value());
    CHECK(err.error() == std::errc::resource_unavailable_try_again);
    CHECK_FALSE(append.append(""));
    err = append.readAndClose(kData.size() + 1);
    REQUIRE_FALSE(err.has_value());
    CHECK(err.error() == std::errc::resource_unavailable_try_again);
    CHECK_FALSE(append.close());
    CHECK_FALSE(append.close());
    CHECK(append.close(std::make_error_code(std::errc::filename_too_long)) == std::errc::filename_too_long); // arbitrary error code, the function will still attempt to close the file.
    CHECK(append.close(std::make_error_code(std::errc::invalid_seek)) == std::errc::invalid_seek);
	CHECK_FALSE(append.open(path));
    CHECK(append.close(std::make_error_code(std::errc::already_connected)) == std::errc::already_connected);
	CHECK_FALSE(append.readAndRestore(10)); // Should be closed.
    CHECK_FALSE(append.close());

#ifdef NDEBUG
    CHECK_THROWS_AS(FileGuard::AppendGuideline(no), std::ios_base::failure);
    CHECK_THROWS_AS(FileGuard::AppendGuideline(no, ""), std::ios_base::failure);
    CHECK_THROWS_AS(FileGuard::AppendGuideline(no, "a"), std::ios_base::failure);
    CHECK_FALSE(std::filesystem::exists(no));
#endif //NDEBUG

    CHECK(std::filesystem::remove(path));
    std::filesystem::remove(no);
}