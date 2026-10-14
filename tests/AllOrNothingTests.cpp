// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <catch2/catch_test_macros.hpp>

module FileGuard;
import :AllOrNothing;

import std;

static constexpr std::string_view kData{ "Some really critical data that needs to be saved at all cost even if it launches nuclear missiles in the process!!!" };

TEST_CASE("All Or Nothing w/o enc",  "[guideline][allOrNothing][noEnc]")
{
    
    const std::filesystem::path test{ "allornothing0.txt" };
    std::filesystem::remove(test);
    REQUIRE_FALSE(FileGuard::createFile(test));
    
    const std::filesystem::path invalid{ "invalid.txt" };
    std::filesystem::remove(invalid);

    SECTION("Simple save")
    {
        REQUIRE_FALSE(FileGuard::AllOrNothing::write(test, kData));
        auto [dataRead, error] = FileGuard::AllOrNothing::read(test);
        CHECK(error.isUpToDate);
        CHECK_FALSE(error.ec);
        CHECK_FALSE(dataRead.empty());
        CHECK(dataRead == kData);
        CHECK(std::filesystem::exists(test));
    }

    SECTION("overwritting")
    {
        constexpr std::string_view otherData{ "data that will be overrriden" };
        CHECK_FALSE(FileGuard::AllOrNothing::write(test, kData));
        REQUIRE_FALSE(FileGuard::AllOrNothing::write(test, otherData));

        auto [dataRead, error] = FileGuard::AllOrNothing::read(test);
        CHECK(error.isUpToDate);
        CHECK_FALSE(error.ec);
        CHECK_FALSE(dataRead.empty());
        CHECK(dataRead == otherData);
        CHECK(std::filesystem::exists(test));
    }

    SECTION("no file")
    {
#ifdef NDEBUG
        auto [dataRead, error] = FileGuard::AllOrNothing::read(invalid);
        CHECK(dataRead.empty());
        CHECK(error.ec == FileGuard::ErrorCode::noValidFile);
        CHECK_FALSE(error.isUpToDate);
        CHECK_FALSE(std::filesystem::exists(invalid));
#endif //NDEBUG

        REQUIRE_FALSE(FileGuard::AllOrNothing::write(invalid, kData));
        CHECK(std::filesystem::exists(invalid)); // Write create file
        CHECK(FileGuard::AllOrNothing::read(invalid).first == kData);
    }

    std::filesystem::remove(test);
    std::filesystem::remove(invalid);
}

TEST_CASE("All Or Nothing w/ enc", "[guideline][allOrNothing][enc]")
{
    const std::filesystem::path test{ "allornothing1.txt" };
    std::filesystem::remove(test);
    REQUIRE_FALSE(FileGuard::createFile(test));
    REQUIRE_FALSE(FileGuard::g_toyWrapper.generateKey("azerty"));

    SECTION("decryption")
    {
        REQUIRE_FALSE(FileGuard::AllOrNothing::write(test, kData, &FileGuard::g_toyWrapper));

        auto [dataRead, error] = FileGuard::AllOrNothing::read(test, nullptr);
        CHECK(error.isUpToDate);
        CHECK_FALSE(error.ec);
        CHECK_FALSE(dataRead == kData);

        std::tie(dataRead, error) = FileGuard::AllOrNothing::read(test, &FileGuard::g_toyWrapper);
        CHECK(error.isUpToDate);
        CHECK_FALSE(error.ec);
        CHECK(dataRead == kData);
    }

    FileGuard::g_toyWrapper.clearKey();
    std::filesystem::remove(test);
}

TEST_CASE("All Or Nothing crash test", "[guideline][allOrNothing][crash]")
{
    const std::filesystem::path test{ "allornothing2.txt" };
    REQUIRE_FALSE(FileGuard::AllOrNothing::write(test, kData));

    SECTION("tmp exists")
    {
        REQUIRE_FALSE(FileGuard::createFile(test.string() + ".tmp")); // Would signal crash before/while writing copy.

        auto [dataRead, error] = FileGuard::AllOrNothing::read(test);
        CHECK_FALSE(dataRead.empty());
        CHECK(error.ec == FileGuard::ErrorCode::invalidNewer);
        CHECK_FALSE(error.isUpToDate);

        std::filesystem::remove(test.string() + ".tmp");
    }

    SECTION("tmp valid")
    {
        std::filesystem::copy(test, test.string() + ".tmp", std::filesystem::copy_options::overwrite_existing); // Crash after finishing the copy but before update.

        auto [dataRead, error] = FileGuard::AllOrNothing::read(test);
        CHECK_FALSE(dataRead.empty());
        CHECK(error.ec == FileGuard::ErrorCode::cleaningFailed);
        CHECK(error.isUpToDate);

        std::filesystem::remove(test.string() + ".tmp");
    }

    SECTION("main does not exist")
    {
        std::filesystem::copy(test, test.string() + ".tmp", std::filesystem::copy_options::overwrite_existing); // Failed to rename to "test" without tmp.
        std::filesystem::remove(test);

        auto [dataRead, error] = FileGuard::AllOrNothing::read(test);
        CHECK_FALSE(dataRead.empty());
        CHECK(error.ec == FileGuard::ErrorCode::cleaningFailed);
        CHECK(error.isUpToDate);

        std::filesystem::remove(test.string() + ".tmp");
    }

    std::filesystem::remove(test);
    // no valid file is tested during the previous test case.
}