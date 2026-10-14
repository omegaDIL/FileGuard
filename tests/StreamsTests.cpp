// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <catch2/catch_test_macros.hpp>

module FileGuard;
import :Streams;

import std;

TEST_CASE("open files", "[streams][open files]")
{
    std::string text{ "Hello, World\n" };
    std::filesystem::path existing{ "existing1.txt" };
    std::filesystem::path notExisting{ "notExisting1.txt" };
    std::filesystem::path noPerms{ "noPerms1.txt" };

    std::filesystem::remove(existing);
    std::filesystem::remove(notExisting);
    std::filesystem::remove(noPerms);

    {
        std::ofstream existingStream{ existing, std::ios::app };
        std::ofstream noPermsStream { noPerms, std::ios::app };
    }
    std::filesystem::permissions(noPerms,
          std::filesystem::perms::owner_read
        | std::filesystem::perms::group_read
        | std::filesystem::perms::others_read,
        std::filesystem::perm_options::remove);

    const auto permissionDeniedChecksAreReliable = [&noPerms]
    {	// May not work on windows
        std::ifstream readProbe{ noPerms };
        std::fstream ioProbe{ noPerms, std::ios::in | std::ios::out };

        return !readProbe && !ioProbe;
    }();

    SECTION("open IO file")
    {
        // 1. Check Not Existing Behavior
        REQUIRE_FALSE(FileGuard::openIOFile(notExisting).has_value());
        CHECK_NOFAIL(FileGuard::openIOFile(notExisting).error() == std::errc::no_such_file_or_directory); // On most platform
        CHECK_FALSE(std::filesystem::exists(notExisting));

        // 2. Check Permission Denied Behavior
        if (permissionDeniedChecksAreReliable)
        {
            REQUIRE_FALSE(FileGuard::openIOFile(noPerms).has_value());
            CHECK_NOFAIL(FileGuard::openIOFile(noPerms).error() == std::errc::permission_denied); // On most platform
        }

        // 3. Check Successful Open & Modifiers
        CHECK(FileGuard::openIOFile(existing).has_value());
        CHECK(FileGuard::openIOFile(existing, std::ios::app).has_value());

        {
            // 4. Opened ?
            std::fstream file{ std::move(*FileGuard::openIOFile(existing, std::ios::binary)) };
            REQUIRE(file.is_open());
            REQUIRE_FALSE(file.fail()); // Freshly opened files should not be in a fail state

            // 5. Write verification
            file.seekp(0, std::ios::beg);
            file.write(text.data(), static_cast<uint32_t>(text.size()));
            file.flush(); // Ensure data hits disk/buffer stream cleanly
            CHECK(file);

            // 6. Read verification
            std::string output{};
            output.resize(text.size());
            file.seekg(0, std::ios::beg);
            file.read(output.data(), static_cast<uint32_t>(text.size()));
            CHECK(file);
            CHECK(output == text);
        }	// Clear file.

        // 7. Working modifiers
        CHECK(FileGuard::openIOFile(existing).has_value());
        CHECK(std::filesystem::file_size(existing) != 0); // Shouldn't truncate.

        CHECK(FileGuard::openIOFile(existing, std::ios::trunc).has_value()); // Side effect: truncates
        CHECK(std::filesystem::file_size(existing) == 0);
    }

    SECTION("open reading file")
    {
        // 1. Check Not Existing Behavior
        REQUIRE_FALSE(FileGuard::openReadingFile(notExisting).has_value());
        CHECK_NOFAIL(FileGuard::openReadingFile(notExisting).error() == std::errc::no_such_file_or_directory); // On most platform
        CHECK_FALSE(std::filesystem::exists(notExisting));

        // 2. Check Permission Denied Behavior
        if (permissionDeniedChecksAreReliable)
        {
            REQUIRE_FALSE(FileGuard::openReadingFile(noPerms).has_value());
            CHECK_NOFAIL(FileGuard::openReadingFile(noPerms).error() == std::errc::permission_denied); // On most platform
        }

        // 3. Check Successful Open
        REQUIRE(FileGuard::openReadingFile(existing).has_value());

        // 4. Opened ?
        std::ifstream file{ std::move(*FileGuard::openReadingFile(existing, std::ios::binary)) };
        CHECK(file.is_open());
        CHECK_FALSE(file.fail()); // Freshly opened files should not be in a fail state
    }

    SECTION("open writing file")
    {
        // 1. Check Not Existing Behavior
        REQUIRE_FALSE(FileGuard::openWritingFile(notExisting).has_value());
        CHECK_NOFAIL(FileGuard::openWritingFile(notExisting).error() == std::errc::no_such_file_or_directory);  // On most platform
        CHECK_FALSE(std::filesystem::exists(notExisting));

        // 2. Check Successful Open & Modifiers
        CHECK(FileGuard::openWritingFile(existing).has_value());
        CHECK(FileGuard::openWritingFile(existing, std::ios::app).has_value());

        {
            // 3. Opened ?
            std::ofstream file{ std::move(*FileGuard::openWritingFile(existing, std::ios::binary)) };
            REQUIRE(file.is_open());
            REQUIRE_FALSE(file.fail()); // Freshly opened files should not be in a fail state

            // 4. Write verification
            file.write(text.data(), static_cast<uint32_t>(text.size()));
            file.flush();
            CHECK(std::filesystem::file_size(existing) == text.size());
        }	// Clear file.

        // 5. Working modifiers
        CHECK(FileGuard::openWritingFile(existing).has_value());
        CHECK(std::filesystem::file_size(existing) != 0); // Shouldn't truncate.

        CHECK(FileGuard::openWritingFile(existing, std::ios::trunc).has_value()); // Side effect: truncates
        CHECK(std::filesystem::file_size(existing) == 0);
    }

    CHECK(std::filesystem::remove(existing));
    std::filesystem::remove(notExisting);
    CHECK(std::filesystem::remove(noPerms));
}

TEST_CASE("reading streams", "[files][IO]")
{
    std::string data{ "Hello, World" };
    std::istringstream stream{ data };

    SECTION("reading whole stream")
    {
        stream.clear();
        stream.seekg(0, std::ios::beg);

        std::expected<std::string, std::error_code> expect{ FileGuard::readStream(stream, 12) };
        REQUIRE(expect.has_value()); // May set eof but the documentation says it's not an error
        CHECK_NOFAIL(stream.eof());
        CHECK(*expect == "Hello, World");
        CHECK(stream.exceptions() == std::ios::goodbit);

        stream.seekg(0, std::ios::beg);
        stream.exceptions(std::ios::failbit);
        CHECK(FileGuard::readStream(stream, 3).has_value());
        CHECK(stream.exceptions() == std::ios::failbit);
    }

    SECTION("reading block by block")
    {
        stream.clear();
        stream.seekg(0, std::ios::beg);

        std::expected<std::string, std::error_code> expect{ FileGuard::readStream(stream, 4) };
        REQUIRE(expect.has_value());
        CHECK(*expect == "Hell");
        CHECK(stream);

        // Cursor is shifted

        expect = FileGuard::readStream(stream, 6);
        REQUIRE(expect.has_value());
        CHECK(*expect == "o, Wor");
        CHECK(stream);

        expect = FileGuard::readStream(stream, 0);
        REQUIRE(expect.has_value());
        CHECK(expect->empty());
        CHECK(stream);

        expect = FileGuard::readStream(stream, 2);
        REQUIRE(expect.has_value());
        CHECK(*expect == "ld");
        CHECK_NOFAIL(stream.eof());
    }

    SECTION("Error while using reading stream")
    {
        stream.seekg(0, std::ios::end);

        std::expected<std::string, std::error_code> expect{ FileGuard::readStream(stream, 4) };
        REQUIRE_FALSE(expect.has_value());
        CHECK(expect.error() == std::errc::resource_unavailable_try_again);

        stream.clear();
        stream.seekg(0, std::ios::beg);

        expect = FileGuard::readStream(stream, 13);
        REQUIRE_FALSE(expect.has_value());
        CHECK(expect.error() == std::errc::resource_unavailable_try_again);
        CHECK(stream.exceptions() == std::ios::goodbit);

        stream.exceptions(std::ios::badbit);
        expect = FileGuard::readStream(stream, 13);
        REQUIRE_FALSE(expect.has_value());
        CHECK(stream.exceptions() == std::ios::badbit); // mask is maintained
    }
}

TEST_CASE("overwriting streams", "[files][IO]")
{
    std::ostringstream stream{ std::ios::binary };
    stream.seekp(0, std::ios::beg);

    CHECK(FileGuard::overwriteStream(stream, "Hello, World", 12).value() == 0);
    CHECK(FileGuard::overwriteStream(stream, "Hello, World", 4).value() == 0);
    CHECK(FileGuard::overwriteStream(stream, "other test", 0).value() == 0);
    CHECK(FileGuard::overwriteStream(stream, "other test", 100).value() == 0);
    CHECK(stream.str() == "Hello, WorldHellother test");
    CHECK(stream);
    CHECK(stream.exceptions() == std::ios::goodbit);

    stream.exceptions(std::ios::badbit);
    stream.seekp(0, std::ios::beg);
    CHECK_FALSE(FileGuard::overwriteStream(stream, "test", 100));
    CHECK(stream.exceptions() == std::ios::badbit);
}

TEST_CASE("flushing writing file", "[files][flushing]")
{
    std::filesystem::path fileA{ "existingA3.txt " };
    std::filesystem::path fileB{ "existingB3.txt " };
    REQUIRE_FALSE(FileGuard::createFile(fileA).value());
    REQUIRE_FALSE(FileGuard::createFile(fileB).value());

    auto streamW{ FileGuard::openWritingFile(fileA) };
    auto streamIO{ FileGuard::openIOFile(fileB) };
    REQUIRE(streamW);
    REQUIRE(streamIO);

    SECTION("flush data using ofstream")
    {
        streamW = FileGuard::openWritingFile(fileA);
        REQUIRE(streamW);

        *streamW << "Hello, World";
        std::error_code ec{ FileGuard::flushWritingFile(*streamW) }; // First flush.
        CHECK_FALSE(ec);
        REQUIRE(streamW->is_open());
        REQUIRE(std::filesystem::exists(fileA));

        *streamW << "Azerty";
        ec = FileGuard::flushWritingFile(*streamW); // Second flush with new data.
        CHECK_FALSE(ec);
        REQUIRE(streamW->is_open());
        REQUIRE(std::filesystem::exists(fileA));

        ec = FileGuard::flushWritingFile(*streamW); // Third flush with no data
        CHECK_FALSE(ec);
        REQUIRE(streamW->is_open());
        REQUIRE(std::filesystem::exists(fileA));

        ec = FileGuard::flushWritingFile(*streamW); // Fourth flush with no data
        CHECK_FALSE(ec);
        REQUIRE(streamW->is_open());
        REQUIRE(std::filesystem::exists(fileA));

        *streamW << "Hello!";
        ec = FileGuard::flushWritingFile(*streamW); // Fifth flush with new data
        CHECK_FALSE(ec);
        CHECK(streamW->is_open());
        CHECK(std::filesystem::exists(fileA));

        CHECK(std::filesystem::file_size(fileA) == 24);
    }

    SECTION("flush data using fstream")
    {
        streamIO = FileGuard::openIOFile(fileB);
        REQUIRE(streamIO);

        *streamIO << "Hello, World";
        std::error_code ec{ FileGuard::flushWritingFile(*streamIO) }; // First flush.
        CHECK_FALSE(ec);
        REQUIRE(streamIO->is_open());
        REQUIRE(std::filesystem::exists(fileB));

        *streamIO << "Azerty";
        ec = FileGuard::flushWritingFile(*streamIO); // Second flush with new data.
        CHECK_FALSE(ec);
        REQUIRE(streamIO->is_open());
        REQUIRE(std::filesystem::exists(fileB));

        ec = FileGuard::flushWritingFile(*streamIO); // Third flush with no data
        CHECK_FALSE(ec);
        REQUIRE(streamIO->is_open());
        REQUIRE(std::filesystem::exists(fileB));

        ec = FileGuard::flushWritingFile(*streamIO); // Fourth flush with no data
        CHECK_FALSE(ec);
        REQUIRE(streamIO->is_open());
        REQUIRE(std::filesystem::exists(fileB));

        *streamIO << "Hello!";
        ec = FileGuard::flushWritingFile(*streamIO); // Fifth flush with new data
        CHECK_FALSE(ec);
        CHECK(streamIO->is_open());
        CHECK(std::filesystem::exists(fileB));

        CHECK(std::filesystem::file_size(fileB) == 24);
    }

    SECTION("never opened ofstream")
    {
        std::ofstream toTest;

        std::error_code ec{ FileGuard::flushWritingFile(toTest) };

        CHECK_FALSE(ec);
        CHECK_FALSE(toTest.is_open());
    }

    SECTION("never opened fstream")
    {
        std::fstream toTest;

        std::error_code ec{ FileGuard::flushWritingFile(toTest) };

        CHECK_FALSE(ec);
        CHECK_FALSE(toTest.is_open());
    }

    SECTION("Failure before flushing ofstream")
    {
        streamW = FileGuard::openWritingFile(fileA);
        REQUIRE(streamW);

        streamW->setstate(std::ios_base::badbit);
        std::error_code ec{ FileGuard::flushWritingFile(*streamW) };

        CHECK(ec.value() == 0);
    }

    SECTION("Failure before flushing fstream")
    {
        streamIO = FileGuard::openIOFile(fileB);
        REQUIRE(streamIO);

        streamIO->setstate(std::ios_base::badbit);
        std::error_code ec{ FileGuard::flushWritingFile(*streamIO) };

        CHECK(ec.value() == 0);
    }

    streamW->close();
    streamIO->close();
    CHECK(std::filesystem::remove(fileA));
    CHECK(std::filesystem::remove(fileB));
}

TEST_CASE("closing writing stream", "[files][closing]") 
{
    std::filesystem::path fileA{ "existingA2.txt " };
    std::filesystem::path fileB{ "existingB2.txt " };
    REQUIRE_FALSE(FileGuard::createFile(fileA).value());
    REQUIRE_FALSE(FileGuard::createFile(fileB).value());
    
    auto streamW{ FileGuard::openWritingFile(fileA) };
    auto streamIO{ FileGuard::openIOFile(fileB) };
    REQUIRE(streamW);
    REQUIRE(streamIO);

    SECTION("no data closing using ofstream")
    {
        std::error_code ec{ FileGuard::closeWritingFile(*streamW) };

        CHECK_FALSE(ec);
        CHECK_FALSE(streamW->is_open());
        CHECK(std::filesystem::exists(fileA));
        CHECK(std::filesystem::file_size(fileA) == 0);
    }

    SECTION("no data closing using fstream")
    {
        std::error_code ec{ FileGuard::closeWritingFile(*streamIO) };

        CHECK_FALSE(ec);
        CHECK_FALSE(streamIO->is_open());
        CHECK(std::filesystem::exists(fileB));
        CHECK(std::filesystem::file_size(fileB) == 0);
    }

    SECTION("w/ data closing using ofstream")
    {
        streamW = FileGuard::openWritingFile(fileA);
        REQUIRE(streamW);

        *streamW << "Hello, World";
        
        std::error_code ec{ FileGuard::closeWritingFile(*streamW) };

        CHECK_FALSE(ec);
        CHECK_FALSE(streamW->is_open());
        CHECK(std::filesystem::exists(fileA));
        CHECK(std::filesystem::file_size(fileA) == 12);
    }

    SECTION("w/ data closing using fstream")
    {
        streamIO = FileGuard::openIOFile(fileB);
        REQUIRE(streamIO);

        *streamIO << "Hello, World";

        std::error_code ec{ FileGuard::closeWritingFile(*streamIO) };

        CHECK_FALSE(ec);
        CHECK_FALSE(streamIO->is_open());
        CHECK(std::filesystem::exists(fileB));
        CHECK(std::filesystem::file_size(fileB) == 12);
    }

    SECTION("already closed using ofstream")
    {
        streamW->close();

        const auto before{ std::filesystem::file_size(fileA) };

        std::error_code ec{ FileGuard::closeWritingFile(*streamW) };

        CHECK_FALSE(ec);
        CHECK_FALSE(streamW->is_open());
        CHECK(std::filesystem::exists(fileA));
        CHECK(std::filesystem::file_size(fileA) == before);
    }

    SECTION("already closed using fstream")
    {
        streamIO->close();

        const auto before{ std::filesystem::file_size(fileB) };

        std::error_code ec{ FileGuard::closeWritingFile(*streamIO) };

        CHECK_FALSE(ec);
        CHECK_FALSE(streamIO->is_open());
        CHECK(std::filesystem::exists(fileB));
        CHECK(std::filesystem::file_size(fileB) == before);
    }

    SECTION("never opened ofstream")
    {
        std::ofstream toTest;

        std::error_code ec{ FileGuard::closeWritingFile(toTest) };

        CHECK_FALSE(ec);
        CHECK_FALSE(toTest.is_open());
    }

    SECTION("never opened fstream")
    {
        std::fstream toTest;

        std::error_code ec{ FileGuard::closeWritingFile(toTest) };

        CHECK_FALSE(ec);
        CHECK_FALSE(toTest.is_open());
    }

    SECTION("Failure before closing ofstream")
    {
        streamW = FileGuard::openWritingFile(fileA);
        REQUIRE(streamW);

        streamW->setstate(std::ios_base::badbit);
        std::error_code ec{ FileGuard::closeWritingFile(*streamW) };

        CHECK(ec.value() == 0);
    }

    SECTION("Failure before closing fstream")
    {
        streamIO = FileGuard::openIOFile(fileB);
        REQUIRE(streamIO);

        streamIO->setstate(std::ios_base::badbit);
        std::error_code ec{ FileGuard::closeWritingFile(*streamIO) };

        CHECK(ec.value() == 0);
    }

    streamW->close();
    streamIO->close();
    CHECK(std::filesystem::remove(fileA));
    CHECK(std::filesystem::remove(fileB));
}


TEST_CASE("create file", "[files][creation]")
{
    std::filesystem::path file{ "existing4.txt " };
    std::filesystem::path noDirectFile{ "DoesntExist/shouldFail4.txt " };
    std::filesystem::remove(file);

    CHECK(FileGuard::createFile(file).value() == 0);
    CHECK(FileGuard::createFile(file).value() == 0); // No error if it already exist.
    CHECK(std::filesystem::exists(file));

    CHECK_FALSE(FileGuard::createFile(noDirectFile).value() == 0);
    CHECK_NOFAIL(FileGuard::createFile(noDirectFile).value() == static_cast<int>(std::errc::no_such_file_or_directory)); // On most platform
    CHECK_FALSE(std::filesystem::exists(noDirectFile));

    CHECK(std::filesystem::remove(file)); // Clean up.
    std::filesystem::remove(noDirectFile); // Doesn't exist.
}

TEST_CASE("zeroize file", "[files][zeroize]")
{
    std::filesystem::path doesNotExist{ "doesNotExist5.txt" };
    std::filesystem::path emptyFile{ "empty5.txt" };
    std::filesystem::path toZeroize{ "zeroize5.txt" };

    std::filesystem::remove(doesNotExist);
    {
        std::ofstream fileEmpty{ emptyFile, std::ios::app };
        std::ofstream fileZeroize{ toZeroize, std::ios::app };
        fileZeroize << "Hello, World";
    }

    SECTION("empty file")
    {
        CHECK(FileGuard::basicFileZeroing(emptyFile).value() == 0);
        CHECK(std::filesystem::exists(emptyFile)); // Still exists
    }

    SECTION("doesn't create files")
    {
        CHECK(FileGuard::basicFileZeroing(doesNotExist).value() == static_cast<int>(std::errc::no_such_file_or_directory));
        CHECK_FALSE(std::filesystem::exists(doesNotExist));
    }

    SECTION("regular file")
    {
        auto size{ std::filesystem::file_size(toZeroize) };
    
        CHECK(FileGuard::basicFileZeroing(toZeroize).value() == 0);
        REQUIRE(std::filesystem::exists(toZeroize)); // Still exists

        std::ifstream file{ toZeroize };
        std::string result{};
        file >> result;
        CHECK(result == std::string(12, '\0')); // Zeroed.

        CHECK(std::filesystem::file_size(toZeroize) == size); // Size is unchange)
    }

    CHECK(std::filesystem::remove(toZeroize));
    CHECK(std::filesystem::remove(emptyFile));
    std::filesystem::remove(doesNotExist);
}