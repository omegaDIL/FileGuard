// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <catch2/catch_test_macros.hpp>

module FileGuard;
import :Encryption;

import std;

using Wrapper = FileGuard::EncryptionWrapper;

TEST_CASE("get key with toy", "[encryption][toy]")
{
    std::vector<std::uint8_t> key{};
    std::vector<std::uint8_t> key2{};

    SECTION("Has correct value")
    {
        CHECK_FALSE(FileGuard::toyKeyRetriever("", key));
        CHECK(key.size() == 16);
        key.clear();

        CHECK_FALSE(FileGuard::toyKeyRetriever("1", key));
        CHECK(key.size() == 16);
        key.clear();

        CHECK_FALSE(FileGuard::toyKeyRetriever("azerty", key));
        CHECK(key.size() == 16);
        key.clear();

        CHECK_FALSE(FileGuard::toyKeyRetriever("qwertyuiopasdfghjklm", key));
        CHECK(key.size() == 20);
        key.clear();
    }

    SECTION("same key")
    {
        // Not random

        FileGuard::toyKeyRetriever("1st", key);
        FileGuard::toyKeyRetriever("1st", key2);
        CHECK(key == key2);

        FileGuard::toyKeyRetriever("yay", key);
        FileGuard::toyKeyRetriever("yay", key2);
        CHECK(key == key2);

        FileGuard::toyKeyRetriever("", key);
        FileGuard::toyKeyRetriever("", key2);
        CHECK(key == key2);
    }

    SECTION("different keys")
    {
        FileGuard::toyKeyRetriever("1st", key);
        FileGuard::toyKeyRetriever("2nd", key2);
        CHECK_FALSE(key == key2);

        FileGuard::toyKeyRetriever("yay", key);
        FileGuard::toyKeyRetriever("noo", key2);
        CHECK_FALSE(key == key2);

        FileGuard::toyKeyRetriever("1info", key);
        FileGuard::toyKeyRetriever("2info", key2);
        CHECK_FALSE(key == key2);

        FileGuard::toyKeyRetriever("size", key);
        FileGuard::toyKeyRetriever("other size", key2);
        CHECK_FALSE(key == key2);

        FileGuard::toyKeyRetriever("UPPERCASE", key);
        FileGuard::toyKeyRetriever("upperCase", key2);
        CHECK_FALSE(key == key2);

        FileGuard::toyKeyRetriever("", key);
        FileGuard::toyKeyRetriever("a", key2);
        CHECK_FALSE(key == key2);

        FileGuard::toyKeyRetriever("empty", key);
        FileGuard::toyKeyRetriever("", key2);
        CHECK_FALSE(key == key2);
    }

    // Since this is a toy example, we don't need to check security.
}

TEST_CASE("encrypt/decrypt with toy", "[encryption][toy]")
{
    const std::vector<uint8_t> data { 123, 35, 76, 45, 56, 67, 79, 254, 0, 32, 67, 167, 78, 99, 124 };
    const std::vector<uint8_t> key  { 34, 45, 124, 34, 56, 89, 0, 206, 10, 3, 178, 167, 39, 199, 224, 129 };
    const std::vector<uint8_t> nonce{ 56, 12, 2, 255, 8, 45, 79, 33, 100, 3, 99, 167, 76, 1 };
    const std::vector<uint8_t> empty{};

    std::string output{};

    SECTION("No Throw")
    {
        CHECK_NOTHROW(FileGuard::symmetricalToyEncryption(data, empty, nonce, output));
        CHECK_NOTHROW(FileGuard::symmetricalToyEncryption(data, key, empty, output));
        CHECK_NOTHROW(FileGuard::symmetricalToyEncryption(data, empty, empty, output));
        CHECK_NOTHROW(FileGuard::symmetricalToyEncryption(data, key, nonce,output));
    }

    SECTION("Has (no) value")
    {
        CHECK(FileGuard::symmetricalToyEncryption(data, empty, nonce, output) == FileGuard::ErrorCode::emptyKey);
        CHECK(FileGuard::symmetricalToyEncryption(data, key, empty, output) == FileGuard::ErrorCode::emptyNonce);
        CHECK(FileGuard::symmetricalToyEncryption(data, empty, empty, output));
        CHECK_FALSE(FileGuard::symmetricalToyEncryption(data, key, nonce, output));
    }

    SECTION("Fails with empty nonce/key")
    {
        CHECK(FileGuard::symmetricalToyEncryption(data, empty, nonce, output) == FileGuard::ErrorCode::emptyKey);
        CHECK(FileGuard::symmetricalToyEncryption(data, key, empty, output) == FileGuard::ErrorCode::emptyNonce);
        CHECK(FileGuard::symmetricalToyEncryption(data, empty, empty, output));
    }

    SECTION("Performs encryption/decryption")
    {
        REQUIRE_FALSE(FileGuard::symmetricalToyEncryption(data, key, nonce, output));

        std::vector<uint8_t> encrypted(output.begin(), output.end());
        REQUIRE_FALSE(encrypted == data);

        std::span encryptedViewed{ reinterpret_cast<std::uint8_t*>(output.data()), encrypted.size() };
        std::string decrypted{};
        REQUIRE_FALSE(FileGuard::symmetricalToyEncryption(encryptedViewed, key, nonce, decrypted));

        std::vector<uint8_t> decryptedConverted(decrypted.begin(), decrypted.end());
        CHECK(decryptedConverted == data);
    }
}


TEST_CASE("get empty nonce", "[encryption][nonce]") 
{
    SECTION("error handling")
    {
        CHECK_NOTHROW(Wrapper::getEmptyNonce());
        CHECK_NOTHROW(Wrapper::getEmptyNonce(16));
        CHECK_NOTHROW(Wrapper::getEmptyNonce(30));

#ifdef NDEBUG
        CHECK_THROWS_AS(Wrapper::getEmptyNonce(Wrapper::s_maxNonceSize + 1), std::invalid_argument);
        CHECK_THROWS_AS(Wrapper::getEmptyNonce(Wrapper::s_maxNonceSize + 2), std::invalid_argument);
        CHECK_THROWS_AS(Wrapper::getEmptyNonce(0), std::invalid_argument);
#endif //NDEBUG
    }

    SECTION("respect size") 
    {
        CHECK(Wrapper::getEmptyNonce().size() == Wrapper::s_defaultNonceSize);
        CHECK(Wrapper::getEmptyNonce(30ull).size() == 30ull);
        CHECK(Wrapper::getEmptyNonce(Wrapper::s_maxNonceSize).size() == Wrapper::s_maxNonceSize);

        CHECK(std::ranges::all_of(Wrapper::getEmptyNonce(15ull), [](char c) { return c == '\0'; }));
    }
}

TEST_CASE("get crypto nonce", "[encryption][nonce]")
{
    SECTION("error handling")
    {
        CHECK_NOTHROW(Wrapper::getCryptoNonce());
        CHECK_NOTHROW(Wrapper::getCryptoNonce(16));
        CHECK_NOTHROW(Wrapper::getCryptoNonce(30));
#ifdef NDEBUG
        CHECK_THROWS_AS(Wrapper::getCryptoNonce(Wrapper::s_maxNonceSize + 1), std::invalid_argument);
        CHECK_THROWS_AS(Wrapper::getCryptoNonce(Wrapper::s_maxNonceSize + 2), std::invalid_argument);
        CHECK_THROWS_AS(Wrapper::getCryptoNonce(0), std::invalid_argument);
#endif //NDEBUG
    }

    SECTION("respect size")
    {
        CHECK(Wrapper::getCryptoNonce().size() == Wrapper::s_defaultNonceSize);
        CHECK(Wrapper::getCryptoNonce(30ull).size() == 30ull);
        CHECK(Wrapper::getEmptyNonce(Wrapper::s_maxNonceSize).size() == Wrapper::s_maxNonceSize);
    }

    SECTION("Uniqueness")
    {
        const std::string nonce{ Wrapper::getCryptoNonce() };
        const std::unordered_set<char> unique_bytes(nonce.begin(), nonce.end());
        CHECK(unique_bytes.size() >= Wrapper::s_defaultNonceSize / 3 + 1);
    }

    SECTION("High entropy")
    {
        constexpr size_t iterations{ 100 };
        std::unordered_set<std::string> seen_nonces{};

        for (size_t i{ 0 }; i < iterations; ++i)
            seen_nonces.insert(Wrapper::getCryptoNonce());

        CHECK(seen_nonces.size() == iterations);
        // If there are no duplicates, the set size must equal the iteration count
        // Assumption: A nonce has 256^12 = 2^8^12 = 2^96. 100 different nonces should never even have one duplicate.
    }
}


TEST_CASE("key state", "[encryption][encrypt]")
{
    SECTION("No throw")
    {
        CHECK_NOTHROW(FileGuard::g_toyWrapper.generateKey(""));
        CHECK_NOTHROW(FileGuard::g_toyWrapper.clearKey());
        CHECK_NOTHROW(FileGuard::g_toyWrapper.isThereKey());
    }

    SECTION("key generation")
    {
        FileGuard::g_toyWrapper.clearKey();

        CHECK_FALSE(FileGuard::g_toyWrapper.isThereKey());
        REQUIRE(FileGuard::g_toyWrapper.generateKey("test").value() == 0);
        REQUIRE(FileGuard::g_toyWrapper.generateKey("addv").value() == 0);
        CHECK(FileGuard::g_toyWrapper.isThereKey());
    }
}

TEST_CASE("data encryption", "[encryption][encrypt]")
{
    const std::string data{ "encrypt me!\nDid it work?" };
    const std::string nonce{ Wrapper::getCryptoNonce() };
    const std::string emptyNonce{};

    std::string output{};

    SECTION("error handling: encrypt")
    {
        FileGuard::g_toyWrapper.clearKey();

        REQUIRE_NOTHROW(FileGuard::g_toyWrapper.encrypt(data, nonce, output));

        // No key -> must fail.
        CHECK(FileGuard::g_toyWrapper.encrypt(data, nonce, output)== FileGuard::ErrorCode::emptyKey);

        // No nonce -> must fail.
        CHECK(FileGuard::g_toyWrapper.generateKey("fgh\n\t#ssdf'\"").value() == 0);
        CHECK(FileGuard::g_toyWrapper.encrypt(data, emptyNonce, output) == FileGuard::ErrorCode::invalidNonce);
    }

    SECTION("Error handling: decrypt")
    {
        FileGuard::g_toyWrapper.clearKey();

        REQUIRE_NOTHROW(FileGuard::g_toyWrapper.decrypt(data, nonce, output));

        // No key -> must fail.
        CHECK(FileGuard::g_toyWrapper.decrypt(data, nonce, output) == FileGuard::ErrorCode::emptyKey);

        // No nonce -> must fail.
        CHECK(FileGuard::g_toyWrapper.generateKey("fgh\n\t#ssdf'\"").value() == 0);
        CHECK(FileGuard::g_toyWrapper.decrypt(data, emptyNonce, output) == FileGuard::ErrorCode::invalidNonce);
    }

    SECTION("error handling: encrypt and add nonce")
    {
        FileGuard::g_toyWrapper.clearKey();

        REQUIRE_NOTHROW(FileGuard::g_toyWrapper.encryptAndAddNonce(data, nonce, output));

        // No key -> must fail.
        CHECK(FileGuard::g_toyWrapper.encryptAndAddNonce(data, nonce, output)== FileGuard::ErrorCode::emptyKey);

        // No nonce -> must fail.
        CHECK(FileGuard::g_toyWrapper.generateKey("fgh\n\t#ssdf'\"").value() == 0);
        CHECK(FileGuard::g_toyWrapper.encryptAndAddNonce(data, emptyNonce, output) == FileGuard::ErrorCode::invalidNonce);
    }

    SECTION("Error handling: decrypt with nonce appended")
    {
        FileGuard::g_toyWrapper.clearKey();

        REQUIRE_NOTHROW(FileGuard::g_toyWrapper.decryptWithNoneAppended(data, output));

        // No key -> must fail.
        CHECK(FileGuard::g_toyWrapper.decryptWithNoneAppended(data, output) == FileGuard::ErrorCode::emptyKey);

        // No nonce -> must fail.
        CHECK(FileGuard::g_toyWrapper.generateKey("fgh\n\t#ssdf'\"").value() == 0);
        const std::string invalidData{ "notEnough" }; // must contain a 12 bytes nonce, which it does not.
        CHECK(FileGuard::g_toyWrapper.decryptWithNoneAppended(invalidData, output) == FileGuard::ErrorCode::invalidNonce);
    }

    SECTION("working encryption")
    {
        REQUIRE(FileGuard::g_toyWrapper.generateKey("my34new#-\"^\'key").value() == 0);

        CHECK_FALSE(FileGuard::g_toyWrapper.encrypt(data, nonce, output));

        std::string decrypted{};
        CHECK_FALSE(FileGuard::g_toyWrapper.decrypt(output, nonce, decrypted));

        CHECK(decrypted == data);
    }

    SECTION("working encryption with guideline functions")
    {
        REQUIRE(FileGuard::g_toyWrapper.generateKey("my34new#-\"^\'key").value() == 0);

        CHECK_FALSE(FileGuard::g_toyWrapper.encryptAndAddNonce(data, nonce, output));

        std::string decrypted{};
        CHECK_FALSE(FileGuard::g_toyWrapper.decryptWithNoneAppended(output, decrypted));

        CHECK(decrypted == data);
    }

    FileGuard::g_toyWrapper.clearKey();
}

TEST_CASE("constructing an encryption wrapper", "[encryption][construct]")
{
    SECTION("error handling")
    {
#ifdef NDEBUG
        CHECK_THROWS_AS(FileGuard::EncryptionWrapper(nullptr, nullptr), std::invalid_argument);
        CHECK_THROWS_AS(FileGuard::EncryptionWrapper(nullptr, FileGuard::symmetricalToyEncryption), std::invalid_argument);
        CHECK_THROWS_AS(FileGuard::EncryptionWrapper(FileGuard::toyKeyRetriever, nullptr), std::invalid_argument);
        CHECK_THROWS_AS(FileGuard::EncryptionWrapper(FileGuard::toyKeyRetriever, FileGuard::symmetricalToyEncryption, nullptr, FileGuard::EncryptionWrapper::s_maxNonceSize+1), std::invalid_argument);
        CHECK_THROWS_AS(FileGuard::EncryptionWrapper(FileGuard::toyKeyRetriever, FileGuard::symmetricalToyEncryption, nullptr, 0), std::invalid_argument);
#endif //NDEBUG 
    }

    SECTION("usable encryption")
    {
        FileGuard::EncryptionWrapper other{ FileGuard::toyKeyRetriever, FileGuard::symmetricalToyEncryption };
        std::string output{};

        CHECK_FALSE(other.generateKey("gresds"));
        CHECK_FALSE(other.encrypt("zaertfjhgnjfb", other.getCryptoNonce(), output));
    }

    FileGuard::g_toyWrapper.clearKey();
}

TEST_CASE("zeroize data", "[encryption][zeroize]")
{
    std::vector<std::uint8_t> empty{};
    FileGuard::zeroizeValue(empty);
    CHECK(empty.empty());

    std::vector<std::uint8_t> data{ 7, 9, 9, 3, 67 };
    FileGuard::zeroizeValue(data);
    CHECK(data == std::vector<std::uint8_t>{0, 0, 0, 0, 0});
}