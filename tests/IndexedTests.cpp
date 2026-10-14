// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <catch2/catch_test_macros.hpp>

module FileGuard;
import :Indexed;

import std;

TEST_CASE("constructor indexed", "[guideline][indexed]")
{
    SECTION("default constructor")
    {
        const std::filesystem::path path{ "indexed0.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::IndexedGuideline<16> guideline{};
        CHECK_FALSE(guideline.open(path));
        CHECK(guideline.write("Hello world", 0)); // Error
        CHECK_FALSE(guideline.write("Hello worldpaddingpaddingpadding", 0)); // Is valid
        CHECK_FALSE(guideline.close());

        CHECK(std::filesystem::remove(path));
    }

    SECTION("encryption constructor")
    {
        const std::filesystem::path path{ "indexed1.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));
        REQUIRE_FALSE(FileGuard::g_toyWrapper.generateKey("azerty"));

        FileGuard::IndexedGuideline<16, FileGuard::EncryptionWrapper::s_defaultNonceSize> guideline{ FileGuard::g_toyWrapper };
        CHECK_FALSE(guideline.open(path));
        CHECK(guideline.write("Hello world", 0)); // Error
        CHECK_FALSE(guideline.write("Hello worldpaddingpaddingpadding", 0)); // Is valid
        CHECK_FALSE(guideline.close());

        
#ifdef NDEBUG
        CHECK_THROWS_AS((FileGuard::IndexedGuideline<16, 1>{ FileGuard::g_toyWrapper }), std::invalid_argument);
#endif  //NDEBUG

        FileGuard::g_toyWrapper.clearKey();
        CHECK(std::filesystem::remove(path));
    }

    SECTION("open constructor")
    {
        const std::filesystem::path path{ "indexed2.txt" };
        const std::filesystem::path path2{ "indexed2DontExist.txt" };
        std::filesystem::remove(path);
        std::filesystem::remove(path2);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::IndexedGuideline<16> guideline{ path };
        CHECK(guideline.write("Hello world", 0)); // Error
        CHECK_FALSE(guideline.write("Hello worldpaddingpaddingpadding", 0)); // Is valid
        CHECK_FALSE(guideline.open(path));
        CHECK_FALSE(guideline.write("Hello worldpaddingpaddingpadding", 0)); // Is valid
        CHECK_FALSE(guideline.close());

#ifdef NDEBUG
        CHECK_THROWS_AS((FileGuard::IndexedGuideline<16>{ path2 }), std::ios_base::failure);
#endif  //NDEBUG

        CHECK(std::filesystem::remove(path));
    }

    SECTION("open + encryption constructor")
    {
        const std::filesystem::path path{ "indexed3.txt" };
        const std::filesystem::path path2{ "indexed3DontExist.txt" };
        std::filesystem::remove(path);
        std::filesystem::remove(path2);
        REQUIRE_FALSE(FileGuard::createFile(path));
        REQUIRE_FALSE(FileGuard::g_toyWrapper.generateKey("azerty"));

        FileGuard::IndexedGuideline<16, FileGuard::EncryptionWrapper::s_defaultNonceSize> guideline{ path, FileGuard::g_toyWrapper };
        CHECK(guideline.write("Hello world", 0)); // Error
        CHECK_FALSE(guideline.write("Hello worldpaddingpaddingpadding", 0)); // Is valid
        CHECK_FALSE(guideline.open(path));
        CHECK_FALSE(guideline.write("Hello worldpaddingpaddingpadding", 0)); // Is valid
        CHECK_FALSE(guideline.close());


#ifdef NDEBUG
        CHECK_THROWS_AS((FileGuard::IndexedGuideline<16, FileGuard::EncryptionWrapper::s_defaultNonceSize>{ path2, FileGuard::g_toyWrapper }), std::ios_base::failure);
        CHECK_THROWS_AS((FileGuard::IndexedGuideline<16, 1>{ path, FileGuard::g_toyWrapper }), std::invalid_argument);
#endif  //NDEBUG

        FileGuard::g_toyWrapper.clearKey();
        CHECK(std::filesystem::remove(path));
    }

    SECTION("reopening")
    {
        const std::filesystem::path path{ "indexed4.txt" };
        const std::filesystem::path path2{ "indexed(v2)4.txt" };
        std::filesystem::remove(path);
        std::filesystem::remove(path2);
        REQUIRE_FALSE(FileGuard::createFile(path));
        REQUIRE_FALSE(FileGuard::createFile(path2));

        FileGuard::IndexedGuideline<5> indexed{ path };
        CHECK_FALSE(indexed.open(path));
        CHECK_FALSE(indexed.write("chose", 0));
        CHECK_FALSE(indexed.write("chose", 1));
        CHECK_FALSE(indexed.open(path));
        CHECK_FALSE(indexed.write("thing", 0));
        CHECK_FALSE(indexed.ensureFlushed());
        CHECK_FALSE(indexed.close());
        CHECK_FALSE(indexed.open(path2));
        CHECK_FALSE(indexed.write("other", 10));
        CHECK_FALSE(indexed.close());

        CHECK(std::filesystem::remove(path));
        CHECK(std::filesystem::remove(path2));
    }
}

TEST_CASE("writing and reading indexed", "[guideline][Indexed]")
{
    SECTION("no encryption")
    {
        const std::filesystem::path path{ "indexed5.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::IndexedGuideline<12> indexed{};
        REQUIRE_FALSE(indexed.open(path));
        
        CHECK(indexed.read(0, 1).error() == std::make_error_code(std::errc::resource_unavailable_try_again));
        CHECK(indexed.read(1, 2).error() == std::make_error_code(std::errc::resource_unavailable_try_again));

        CHECK_FALSE(indexed.write("Hello, world", 0));
        CHECK_FALSE(indexed.write("Hello, worldazertyqwerty", 1));
        CHECK_FALSE(indexed.write("Hello, worldHello, worldazertyqwerty", 3));
        CHECK_FALSE(indexed.write("Hello, worldHello, worldHello, world", 2));
        CHECK(indexed.write("not12character", 1) == std::make_error_code(std::errc::invalid_argument));
        CHECK_FALSE(indexed.write("Hello, world", 20));
        CHECK_FALSE(indexed.write("paddingpad!!", 21));
        CHECK(indexed.write("lessThan12", 5) == std::make_error_code(std::errc::invalid_argument));
        CHECK_FALSE(indexed.write("Hello, worldpaddingpad!!", 23));

        auto result{ indexed.read(0, 6) };
        REQUIRE(result.has_value());
        CHECK(result == "Hello, worldHello, worldHello, worldHello, worldHello, worldazertyqwerty");

        result = indexed.read(0, 0);;
        REQUIRE(result.has_value());
        CHECK(result == "");

        result = indexed.read(20, 2);
        REQUIRE(result.has_value());
        CHECK(result == "Hello, worldpaddingpad!!");

        CHECK(indexed.read(100, 10).error() == std::make_error_code(std::errc::resource_unavailable_try_again));
        CHECK(indexed.read(100, 1) .error() == std::make_error_code(std::errc::resource_unavailable_try_again));

        result = indexed.read(24, 1);
        REQUIRE(result.has_value());
        CHECK(result == "paddingpad!!");

        CHECK_FALSE(indexed.write("azertyqwerty", 24));
        result = indexed.read(23, 2);
        REQUIRE(result.has_value());
        CHECK(result == "Hello, worldazertyqwerty");

        CHECK_FALSE(indexed.close());

        CHECK(std::filesystem::remove(path));
    }

    SECTION("with encryption")
    {
        const std::filesystem::path path{ "indexed6.txt" };
        std::filesystem::remove(path);
        REQUIRE_FALSE(FileGuard::createFile(path));

        FileGuard::IndexedGuideline<12, FileGuard::EncryptionWrapper::s_defaultNonceSize> indexed{ FileGuard::g_toyWrapper };
        CHECK_FALSE(FileGuard::g_toyWrapper.generateKey("someinfo"));
        REQUIRE_FALSE(indexed.open(path));
        
        CHECK_FALSE(indexed.write("Hello, world", 0));
        CHECK_FALSE(indexed.write("Hello, worldazertyqwerty", 1));
        CHECK_FALSE(indexed.write("Hello, worldHello, worldazertyqwerty", 3));
        CHECK_FALSE(indexed.write("Hello, worldHello, worldHello, world", 2));
        CHECK_FALSE(indexed.write("Hello, world", 20));
        CHECK_FALSE(indexed.write("paddingpad!!", 21));
        CHECK_FALSE(indexed.write("Hello, worldpaddingpad!!", 23));

        auto result{ indexed.read(0, 6) };
        REQUIRE(result.has_value());
        CHECK(result == "Hello, worldHello, worldHello, worldHello, worldHello, worldazertyqwerty");

        result = indexed.read(0, 0);;
        REQUIRE(result.has_value());
        CHECK(result == "");

        result = indexed.read(20, 2);
        REQUIRE(result.has_value());
        CHECK(result == "Hello, worldpaddingpad!!");

        result = indexed.read(24, 1);
        REQUIRE(result.has_value());
        CHECK(result == "paddingpad!!");

        CHECK_FALSE(indexed.write("azertyqwerty", 24));
        result = indexed.read(23, 2);
        REQUIRE(result.has_value());
        CHECK(result == "Hello, worldazertyqwerty");

        CHECK_FALSE(FileGuard::g_toyWrapper.generateKey("notthesame"));
        result = indexed.read(23, 2);
        REQUIRE(result.has_value());
        CHECK_FALSE(result == "Hello, worldazertyqwerty"); // Not the same key.

        CHECK_FALSE(indexed.closeAndClearKey());
		CHECK_FALSE(FileGuard::g_toyWrapper.isThereKey());

        CHECK(std::filesystem::remove(path));
    }
}

TEST_CASE("error handling indexed", "[guideline][Indexed]")
{
    SECTION("w/o encryption")
    {
        const std::filesystem::path path{ "indexed7.txt" };
        const std::filesystem::path no{ "doesntExistIndexed7.txt" };
        std::filesystem::remove(path);
        std::filesystem::remove(no);
        REQUIRE_FALSE(FileGuard::createFile(path));

#ifdef NDEBUG
        // constructors
        CHECK_THROWS_AS(FileGuard::IndexedGuideline<8>{ no }, std::ios_base::failure);
#endif //NDEBUG

        FileGuard::IndexedGuideline<8> indexed{};

        CHECK(indexed.write("error!!!", 67) == std::errc::invalid_argument);

        REQUIRE(indexed.open(no) == std::errc::no_such_file_or_directory);
        CHECK(indexed.write("error!!!", 67) == std::errc::invalid_argument);

        CHECK_FALSE(std::filesystem::exists(no));

        REQUIRE_FALSE(indexed.open(path));

        CHECK(indexed.read(0, 1).error() == std::errc::resource_unavailable_try_again);
        CHECK(indexed.read(1, 2).error() == std::errc::resource_unavailable_try_again);

        CHECK_FALSE(indexed.write("no error", 1)); // is still valid after all errors.

        CHECK(indexed.write("moreThan8", 1) == std::errc::invalid_argument);
        CHECK(indexed.write("ReallyALotMoreThan8", 1) == std::errc::invalid_argument);
        CHECK(indexed.write("not8", 5) == std::errc::invalid_argument);

        CHECK(indexed.read(100, 10).error() == std::errc::resource_unavailable_try_again);
        CHECK(indexed.read(100, 1) .error() == std::errc::resource_unavailable_try_again);

        CHECK_FALSE(indexed.write("no error", 2)); // is still valid after all errors.
        CHECK_FALSE(indexed.write("no error", 0)); // is still valid after all errors.

        CHECK_FALSE(indexed.close());
        CHECK_FALSE(indexed.close());
        CHECK(indexed.close(std::make_error_code(std::errc::filename_too_long)) == std::errc::filename_too_long); // arbitrary error code, the function will still attempt to close the file.
        CHECK(indexed.close(std::make_error_code(std::errc::invalid_seek)) == std::errc::invalid_seek);
        CHECK_FALSE(indexed.open(path));
        CHECK(indexed.close(std::make_error_code(std::errc::already_connected)) == std::errc::already_connected);
        CHECK_FALSE(indexed.read(0, 1)); // Should be closed.
        CHECK_FALSE(indexed.close());

        REQUIRE(std::filesystem::remove(path));
        std::filesystem::remove(no);
    }

    SECTION("w/ encryption")
    {
        const std::filesystem::path path{ "indexed7(2).txt" };
        const std::filesystem::path no{ "doesntExistIndexed7(2).txt" };
        std::filesystem::remove(path);
        std::filesystem::remove(no);
        REQUIRE_FALSE(FileGuard::createFile(path));

#ifdef NDEBUG
        // constructors
        CHECK_THROWS_AS((FileGuard::IndexedGuideline<8, 1>{ FileGuard::g_toyWrapper }), std::invalid_argument);
        CHECK_THROWS_AS((FileGuard::IndexedGuideline<8, FileGuard::EncryptionWrapper::s_defaultNonceSize>{ no, FileGuard::g_toyWrapper }), std::ios_base::failure);
#endif //NDEBUG

        FileGuard::IndexedGuideline<8, FileGuard::EncryptionWrapper::s_defaultNonceSize> indexed{ FileGuard::g_toyWrapper };
        CHECK_FALSE(FileGuard::g_toyWrapper.generateKey("someinfo"));

        CHECK(indexed.write("error!!!", 67) == std::errc::invalid_argument);

        REQUIRE(indexed.open(no) == std::errc::no_such_file_or_directory);
        CHECK(indexed.write("error!!!", 67) == std::errc::invalid_argument);

        CHECK_FALSE(std::filesystem::exists(no));

        REQUIRE_FALSE(indexed.open(path));

        CHECK(indexed.read(0, 1).error() == std::errc::resource_unavailable_try_again);
        CHECK(indexed.read(1, 2).error() == std::errc::resource_unavailable_try_again);

        CHECK_FALSE(indexed.write("no error", 1)); // is still valid after all errors.

        CHECK(indexed.write("moreThan8", 1) == std::errc::invalid_argument);
        CHECK(indexed.write("ReallyALotMoreThan8", 1) == std::errc::invalid_argument);
        CHECK(indexed.write("not8", 5) == std::errc::invalid_argument);

        CHECK(indexed.read(100, 10).error() == std::errc::resource_unavailable_try_again);
        CHECK(indexed.read(100, 1) .error() == std::errc::resource_unavailable_try_again);

        FileGuard::g_toyWrapper.clearKey();

        CHECK(indexed.write("error!!!", 2) == FileGuard::ErrorCode::emptyKey); // no key

        CHECK_FALSE(FileGuard::g_toyWrapper.generateKey("someinfo"));

        CHECK_FALSE(indexed.write("no error", 2)); // is still valid after all errors.
        CHECK_FALSE(indexed.write("no error", 0)); // is still valid after all errors.

        CHECK_FALSE(indexed.close());
        CHECK_FALSE(indexed.close());
        CHECK(indexed.closeAndClearKey(std::make_error_code(std::errc::filename_too_long)) == std::errc::filename_too_long); // arbitrary error code, the function will still attempt to close the file.
        CHECK_FALSE(FileGuard::g_toyWrapper.isThereKey());
        CHECK(indexed.close(std::make_error_code(std::errc::invalid_seek)) == std::errc::invalid_seek);
        CHECK_FALSE(indexed.open(path));
        CHECK(indexed.close(std::make_error_code(std::errc::already_connected)) == std::errc::already_connected);
        CHECK_FALSE(indexed.read(0, 1)); // Should be closed.

        REQUIRE(std::filesystem::remove(path));
        std::filesystem::remove(no);
    }
}