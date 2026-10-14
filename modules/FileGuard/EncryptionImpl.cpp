// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <cassert>

module FileGuard;
import :Encryption;

import std;

#ifndef NDEBUG
#define NOEXCEPTDEBUG noexcept
#else
#define NOEXCEPTDEBUG
#endif  //NDEBUG

namespace FileGuard
{

EncryptionWrapper g_toyWrapper{ toyKeyRetriever, symmetricalToyEncryption };


EncryptionWrapper::EncryptionWrapper(KeyRetriever keyRetriever, Encryptor encryptor, Encryptor decryptor, const std::uint16_t nonceSize) NOEXCEPTDEBUG
    : m_keyRetriever{ std::move(keyRetriever) }, m_encryptor{ std::move(encryptor) }, m_decryptor{ std::move(decryptor) }, m_nonceSize{ nonceSize }
{
#ifndef NDEBUG
    assert(m_keyRetriever != nullptr);
    assert(m_encryptor != nullptr);
    assert(nonceSize <= s_maxNonceSize);
    assert(nonceSize > 0);
#else // NDEBUG
    if (m_keyRetriever == nullptr || m_encryptor == nullptr || nonceSize > s_maxNonceSize || nonceSize == 0) [[unlikely]]
        throw std::invalid_argument{ "Invalid argument(s) provided to EncryptionWrapper constructor." };
#endif // NDEBUG
}

EncryptionWrapper::EncryptionWrapper(EncryptionWrapper&& other) noexcept
    : m_keyRetriever{ std::move(other.m_keyRetriever) }
    , m_encryptor{ std::move(other.m_encryptor) }
    , m_decryptor{ std::move(other.m_decryptor) }
    , m_key{ std::move(other.m_key) }
{
    // NEVER LEAVE TWO COPY OF THE KEY IN MEMORY. Clear the key of the moved-from object immediately after moving it.
    other.clearKey();
}

EncryptionWrapper& EncryptionWrapper::operator=(EncryptionWrapper&& other) & noexcept
{
    std::swap(m_encryptor, other.m_encryptor);
    std::swap(m_decryptor, other.m_decryptor);
    std::swap(m_keyRetriever, other.m_keyRetriever);
    std::swap(m_key, other.m_key);

    // NEVER LEAVE TWO COPY OF THE KEY IN MEMORY. Clear the key of the moved-from object immediately after moving it.
    other.clearKey();

    return *this;
}

std::error_code EncryptionWrapper::encrypt(const std::string_view data, const std::string_view nonce, std::string& output)
{
    if (m_key.empty()) [[unlikely]]
        return make_error_code(ErrorCode::emptyKey);

    if (nonce.size() != m_nonceSize) [[unlikely]]
        return make_error_code(ErrorCode::invalidNonce);

    const std::span dataSpan (reinterpret_cast<const std::uint8_t*>(data.data()),  data.size());
    const std::span nonceSpan(reinterpret_cast<const std::uint8_t*>(nonce.data()), nonce.size() );

    try
    {
        output.clear();
        const std::error_code ec{ m_encryptor(dataSpan, m_key, nonceSpan, output) };
        return ec;
    }
    catch (const std::bad_alloc&)
    {
        return std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        throw;
    }

    std::unreachable();
}

std::error_code EncryptionWrapper::decrypt(const std::string_view data, const std::string_view nonce, std::string& output)
{
    if (m_key.empty()) [[unlikely]]
        return make_error_code(ErrorCode::emptyKey);

    if (nonce.size() != m_nonceSize) [[unlikely]]
        return make_error_code(ErrorCode::invalidNonce);

    const std::span dataSpan (reinterpret_cast<const std::uint8_t*>(data.data()),  data.size());
    const std::span nonceSpan(reinterpret_cast<const std::uint8_t*>(nonce.data()), nonce.size());

    Encryptor& decryptor{ (m_decryptor != nullptr) ? m_decryptor : m_encryptor };

    try
    {
        output.clear();
        const std::error_code ec{ decryptor(dataSpan, m_key, nonceSpan, output) };
        return ec;
    }
    catch (const std::bad_alloc&)
    {
        return std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        throw;
    }

    std::unreachable();
}

std::error_code EncryptionWrapper::encryptAndAddNonce(const std::string_view data, const std::string_view nonce, std::string& output)
{
    output.reserve(data.size() + nonce.size()); // Pre allocate space for the data and the nonce.
    // This is an assumption that isn't always true. It assumes that one byte from data will be encrypted into one byte
    // However, some algorithms encrypt by blocks. This assumption would not allocate enough memory in this case.

    const std::error_code ec{ encrypt(data, nonce, output) };

    try
    {
        // Now, we add the nonce at the end. However, it does not make sense to do it if the encryption process was not
        // successful. We can't know which error codes are considered a success, so we can't evaluate `ec`. Instead, we
        // simply check if output is empty. This is not the most robust check, but sufficient for most cases where output
        // is left unchanged/emptied. For other failure cases, there will be a performance penalty.
        if (!output.empty()) [[likely]]
            output += nonce;
    }
    catch (...) // Only exception that is throw is std::bad_alloc.
    {
        return std::make_error_code(std::errc::not_enough_memory);
    }


    // The line above adds the nonce at the end. Depending on the correctness of the assumption at the top, this might
    // reallocate. A copy might then live in memory (though it is freed but not zeroed). However, this copy is
    // encrypted so it's fine.
    return ec;
}

std::error_code EncryptionWrapper::decryptWithNoneAppended(const std::string_view data, std::string& output)
{
    if (data.length() <= m_nonceSize) [[unlikely]]
        return make_error_code(ErrorCode::invalidNonce);

    const std::size_t dataLength{ data.size() - m_nonceSize };
    const std::string_view actualData{ data.data(), dataLength };
    const std::string_view nonce{ actualData.data() + dataLength, m_nonceSize };

    return decrypt(actualData, nonce, output);
}

std::error_code EncryptionWrapper::generateKey(const std::string_view info)
{
    try
    {
        clearKey(); // Does nothing if it is already cleared.

        return m_keyRetriever(info, m_key); // May throw.
    }
    catch (const std::bad_alloc&)
    {
        return std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        clearKey(); // Clearing if exceptions.
        throw;
    }

    std::unreachable();
}

void EncryptionWrapper::clearKey() noexcept
{
    if (m_key.empty()) return;

    zeroizeValue(m_key);

    m_key.clear();
    m_key.shrink_to_fit();
}

std::string EncryptionWrapper::getCryptoNonce(const std::uint16_t size) NOEXCEPTDEBUG
{
#ifndef NDEBUG
    assert(size <= s_maxNonceSize);
    assert(size != 0);
#else   //NDEBUG
    if (size > s_maxNonceSize || size == 0) [[unlikely]]
        throw std::invalid_argument{ "Nonce size is invalid." };
#endif  //NDEBUG

    std::string nonce(size, char{}); // Reserve at construction time.

    std::random_device rd{}; // cryptographically secure on most platforms

    for (std::size_t i{ 0 }; i < size; ++i)
        // `char` is not guaranteed to be 8-bit long (even though it is on most platforms).
        nonce[i] = static_cast<char>(rd()); 

    return nonce;
}

void zeroizeValue(std::span<std::uint8_t> data) noexcept
{
    volatile std::uint8_t* p{ data.data() };
    for (std::size_t i{ 0 }; i < data.size(); ++i)
        p[i] = 0;
}


} // FileGuard namespace.