/**********************************************************************************************************************
 * \file  Encryption.cppm Encryption.cpp
 * \brief A file that defines an Encryption wrapper class for handling any type of encryption/decryption.
 *
 * \copyright Boost Software License - Version 1.0 - August 17th, 2003
 * \author    omegaDIL, lesaffreismael@proton.me
 * \date      August 2025
 * 
 * \note Functions are thread-safe only when operating on different data. Concurrent operations on the same data are 
 *	     not thread-safe.
 * \note Low usage of `constexpr` because nonces are generated at runtime. Keys must not be hard written as well.
 *********************************************************************************************************************/

module;

#include <version> // includes __cpp_lib_move_only_function
#include <cassert>

export module FileGuard:Encryption;
import :Streams;

import std;

#ifndef NDEBUG
#define NOEXCEPTDEBUG noexcept
#else
#define NOEXCEPTDEBUG
#endif //NDEBUG

// libc++ does not support std::move_only_function yet
#ifdef __cpp_lib_move_only_function // is included by <version>
#define STLFUNCTION std::move_only_function
#else
#define STLFUNCTION std::function
#endif

namespace FileGuard
{

using InfoViewer = std::string_view;
using KeyType = std::vector<std::uint8_t>;
export using KeyRetriever = STLFUNCTION<std::error_code(InfoViewer, KeyType&)>; ///< `STLFUNCTION` is `std::move_only_function` or `std::function` if your standard library doesn't support it.

using DataViewer = std::span<const std::uint8_t>;
using KeyViewer = std::span<const std::uint8_t>;
using NonceViewer = std::span<const std::uint8_t>;
export using Encryptor = STLFUNCTION<std::error_code(DataViewer, KeyViewer, NonceViewer, std::string& output)>; ///< `STLFUNCTION` is `std::move_only_function` or `std::function` if your standard library doesn't support it.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The EncryptionWrapper class.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief A class that provides encryption and decryption functionalities using user-defined functions.
 *
 * The `EncryptionWrapper` class allows users to use custom encryption and decryption algorithms through std::function
 * objects. It does not implement any specific and pre-made encryption algorithm, making it versatile and adaptable to
 * various encryption needs.
 * 
 * It also supports key retrieval from a specified context such as file path, server port, etc. The key is zeroed
 * when the wrapper is destroyed or when the key is cleared (RAII).
 * 
 * The class is designed to handle both symmetric and asymmetric encryption.
 * 
 * Although the class is defined in the FileGuard namespace, it can be used by more than files.
 * 
 * This class's API uses `std::string` and `std::string_view` to make it easier to use with FileGuard's guideline.
 * However, data are converted to `std::span` and `std::vector` (for the key) internally for encryption and decryption.
 *
 * No functions assume that `0` is the only successful error code.
 * 
 * \note The performance and security of the encryption depend on the user-defined functions.
 * \note All functions are set at construction time and cannot be changed later, nor retrieved later. The same applies
 *       to the key.
 * \warning The user must ensure that the provided functions are valid and handle errors appropriately.
 *
 * \see `FileGuard::Category`.
 */
export class EncryptionWrapper
{
private:

    KeyRetriever m_keyRetriever{}; ///< Function that retrieves the key.
    Encryptor m_encryptor{}; ///< Function that encrypts data.
    Encryptor m_decryptor{}; ///< Function that decrypts data.

    std::uint16_t m_nonceSize{}; ///< The nonce size.
    KeyType m_key{}; ///< The encryption key.

public:

    constexpr static std::uint16_t s_defaultNonceSize{ 12 }; ///< The default size of a nonce.
    constexpr static std::uint16_t s_maxNonceSize{ 1024 }; ///< The largest size a nonce can reasonably have.


    /** 
     * \brief Constructs an `EncryptionWrapper` with the provided key retriever, encryptor, and decryptor.
     * \complexity O(1).
     * 
     * If the decryptor function is nullptr, the encryptor function will be used instead (works for symmetric
     * encryption only).
     * 
     * \param[in] keyRetriever: The function to retrieve the encryption key. Uses a string that could represent
     *                          anything (e.g. a server port).
     * \param[in] encryptor: The function to encrypt data with.
     * \param[in] decryptor: The function to decrypt data with.
     * \param[in] nonceSize: The size of the nonce in bytes. Max is 1024 (included)
     * 
     * \pre     `keyRetriever` must be non-nullptr.
     * \pre     `encryptor` must be non-nullptr.
     * \pre     `nonceSize` must be smaller than `s_defaultNonceSize` and greater than 0.
     * \warning Otherwise, in debug, will assert.
     * \throw   std::invalid_argument Otherwise, in release.
     */
    [[nodiscard]] EncryptionWrapper(KeyRetriever keyRetriever, Encryptor encryptor, Encryptor decryptor = nullptr, std::uint16_t nonceSize = s_defaultNonceSize) NOEXCEPTDEBUG;

    [[nodiscard]] EncryptionWrapper(EncryptionWrapper&&) noexcept; // O(N) because zeroize a key.
    [[nodiscard]] EncryptionWrapper& operator=(EncryptionWrapper&&) & noexcept; // O(N) because zeroize a key.
    ~EncryptionWrapper() noexcept { clearKey(); } // O(N) Zeroize the key.
    // No copy semantics for security reasons: the key should not be copied.


    /**
     * \brief Encrypts the provided data using the provided function and the generated key.
     * \complexity The complexity depends on the user-defined encryptor function.
     *
     * If the key was not generated, the function returns `FileGuard::ErrorCode::emptyKey`.
     * If the nonce has an invalid size, the function returns `FileGuard::ErrorCode::invalidNonce`.
     *
     * A bad alloc exception thrown by your custom decryption function is caught and a `std::errc::not_enough_memory`
     * is returned. Other exceptions are simply rethrown.
     *
     * The exact returned code otherwise, if failure, is that of the encryption algorithm.
     *
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * algo-specific exceptions may still be thrown.
     *
     * The output is cleared before computing the result. The state of `output` when the function fails depends on your
     * encryption algorithm.
     *
     * \param[in] data:    The data to be encrypted.
     * \param[in] nonce:   A nonce that will be used for encryption.
     * \param[out] output: Where to place the output.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  The key must have been generated.
     * \pre  The `nonce` size must be the same as provided in the constructor.
     * \pre  The custom encryption function must succeed.
     * \note Otherwise, this function will fail.
     *
     * \note Generate a key before using this function.
     * \note While no exceptions are thrown by this function, the algorithm used may.
     *
     * \see `.decrypt()`.
     * \see `.encryptAndAddNonce()`.
     */
    [[nodiscard]] std::error_code encrypt(std::string_view data, std::string_view nonce, std::string& output);

    /**
     * \brief Decrypts the provided data using the provided function and the generated key.
     * \complexity The complexity depends on the user-defined encryptor function.
     *
     * If the key was not generated, the function returns `FileGuard::ErrorCode::emptyKey`.
     * If the nonce has an invalid size, the function returns `FileGuard::ErrorCode::invalidNonce`.
     *
     * A bad alloc exception thrown by your custom decryption function is caught and a `std::errc::not_enough_memory`
     * is returned. Other exceptions are simply rethrown.
     *
     * The exact returned code otherwise, if failure, is that of the decryption algorithm.
     *
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * algo-specific exceptions may still be thrown.
     *
     * The output is cleared before computing the result. The state of `output` when the function fails depends on your
     * decryption algorithm.
     *
     * \param[in]  data:   The data to be decrypted.
     * \param[in]  nonce:  A nonce that will be used for encryption.
     * \param[out] output: Where to place the output.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  The key must have been generated.
     * \pre  The `nonce` size must be the same as provided in the constructor.
     * \pre  The custom decryption function must succeed.
     * \note Otherwise, this function will fail.
     *
     * \note Generate a key before using this function.
     * \note While no exceptions are thrown by this function, the algorithm used may.
     *
     * \see `encrypt()`.
     */
    [[nodiscard]] std::error_code decrypt(std::string_view data, std::string_view nonce, std::string& output);

    /**
     * \brief Behaves exactly like `.encrypt()`, except it appends the nonce at the end of the string.
     *
     * \note The nonce is appended only if your encryption algorithm returns a 0 exit code.
     *
     * \see `encrypt()`.
     */
    [[nodiscard]] std::error_code encryptAndAddNonce(std::string_view data, std::string_view nonce, std::string& output);

    /**
     * \brief Behaves exactly like `.decrypt()`, except it uses the end of the `data` argument as the nonce.
     *
     * \pre `data` must be long enough to contain more than `m_nonceSize` bytes.
     * \note Will fail and return ErrorCode::invalidNonce otherwise.
     *
     * \see `decrypt()`.
     */
    [[nodiscard]] std::error_code decryptWithNoneAppended(std::string_view data, std::string& output);

    /**
     * \brief (Re)Generates the encryption key using the provided key retriever function.
     * \complexity The complexity depends on the user-defined key retriever function.
     * 
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * algo-specific exceptions may still throw. The most common is `std::bad_alloc`.
     *
     * The exact return code, if failure, is that of the encryption wrapper.
     *
     * A bad alloc exception thrown by your custom decryption function is caught and a `std::errc::not_enough_memory`
     * is returned. Other exceptions are simply rethrown.
     *
     * The key is cleared if it already exists. The key is not necessarily cleared if your custom key retriever fails
     * because the function doesn't know which error codes are considered a success. You have to call `clearKey()`
     * afterwards if needed. If an exception is thrown, the key will be cleared though.
     * 
     * \param[in] info: The info that can be used by the key retriever function.
     * 
     * \return a `std::error_code` indicating success or failure.
     * 
     * \pre  The custom key getter must succeed.
     * \post The key will be generated.
     * \note If not, the function will fail.
     * 
     * \note While no exceptions are thrown by this function, the algorithm used may.
     * 
     * \see `encrypt()`,`decrypt()`.
     */
    [[nodiscard]] std::error_code generateKey(std::string_view info);

    /**
     * \brief Clears the key from memory.
     * \complexity O(N), where N is the size of the key.
     * 
     * May be used to guarantee security by ensuring the key is not kept in memory longer than necessary.
     * Could also be used to reduce misuse of the same key for multiple data.
     * 
     * Uses zeroization.
     * 
     * No errors if no key.
     *
     * The destructor calls this function to ensure that the key is zeroed (RAII).
     */
    void clearKey() noexcept;

    /** 
     * \brief Returns `true` if there is a key.
     */
    [[nodiscard]] constexpr bool isThereKey() const noexcept { return !m_key.empty(); };

    /**
     * \brief Gets the size of the nonce used by the wrapper.
     */
    [[nodiscard]] constexpr std::uint16_t getNonceSize() const noexcept { return m_nonceSize; }

    /**
     * \brief Generates a cryptographically secure nonce of the given size.
     *
     * This function uses `std::random_device` to generate non-deterministic values. It does **not** use static
     * variables, so each call produces fresh entropy.
     * 
     * Allocates `s_defaultNonceSize` bytes on the heap. Since it is less than 1024 bytes, `std::bad_alloc` is usually
     * not thrown. If you target embedded systems, this assumption is false (but don't use `std::string` anyway).
     *
     * \param[in] size: The size of the nonce.
     *
     * \return The generated nonce.
     * 
     * \pre     `size` must be smaller than `s_defaultNonceSize` and greater than 0.
     * \warning Otherwise, in debug, will assert.
     * \throw   std::invalid_argument Otherwise, in release.
     *
     * \note Relatively slow, but suitable for cryptographic purposes.
     * \note How secured it is depends on your platform. The more modern, the safer it usually is (windows, macOS,
     *       linux). Be aware of embedded systems though.
     * \note You may still want to use your own crypto getter if your algo needs one.
     *
     * \see `getEmptyNonce()`.
     */
    [[nodiscard]] static std::string getCryptoNonce(std::uint16_t size = s_defaultNonceSize) NOEXCEPTDEBUG;

    /**
     * \brief Gets an empty nonce of the expected size.
     * 
     * Does not allocate memory on the heap, so it is fast. Uses the .rodata section of the binary.
     * 
     * \param[in] size: The size of the nonce.
     * 
     * \return An empty nonce.
     * 
     * \pre     `size` must be smaller than `s_defaultNonceSize` and greater than 0.
     * \note    Otherwise, if evaluated at compile time, throws to force a compile time error.
     * \warning Otherwise, in debug at runtime, will assert.
     * \throw   std::invalid_argument Otherwise, in release at runtime.
     * 
     * \note This function takes 1024 bytes on the .rodata section. Be aware if you are in an embedded environment.
     *
     * \see `getCryptoNonce()`.
     */
    [[nodiscard]] constexpr static std::string_view getEmptyNonce(const std::uint16_t size = s_defaultNonceSize) NOEXCEPTDEBUG
    {
        if consteval
        {
            if (size > s_maxNonceSize || size == 0)
                compileTimeError("Nonce size is invalid (getEmptyNonce, FileGuard::EncryptionWrapper.");
        }
        else
        {
    #ifndef NDEBUG
            assert(size <= s_maxNonceSize);
            assert(size != 0);
    #else //NDEBUG
            if (size > s_maxNonceSize || size == 0) [[unlikely]]
                throw std::invalid_argument{ "Nonce size is invalid." };
    #endif //NDEBUG
        }

        constexpr static std::array<char, s_maxNonceSize> s_toView{};
        return std::string_view{ s_toView.data(), size };
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The EncryptionWrapper class.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for the EncryptionWrapper class.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Zeroes a value stored in RAM
 * \complexity O(N), where N is the size of the key.
 *
 * May be used to guarantee security by ensuring some data is not kept in memory longer than necessary.
 *
 * `data` is non owning so this function doesn't shrink to fit/clear.
 *
 * \param[in] data: the data to zeroize.
 *
 */
export void zeroizeValue(std::span<std::uint8_t> data) noexcept;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for the EncryptionWrapper class.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Algorithms for the EncryptionWrapper toy instance.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief A simple symmetric toy encryption algorithm for demonstration purposes.
 * \complexity O(N), where N is the size of the data.
 * 
 * The same function can be used for both encryption and decryption.
 * 
 * `FileGuard::ErrorCode::emptyKey`   is returned if the key is of size 0.
 * `FileGuard::ErrorCode::emptyNonce` is returned if the nonce is of size 0.
 *
 * `output` is left unchanged if the function fails unless the function throws `std::bad_alloc`, in which case `output`
 * is not specified.
 *
 * \param[in] data: data to be encrypted/decrypted.
 * \param[in] key: The key to encrypt/decrypted.
 * \param[in] nonce: The nonce to encrypt/decrypted.
 * \param[out] output: Where to place the output.
 *
 * \return a `std::error_code` indicating success or failure.
 * 
 * \pre   Depending on the length of your data, the program may not be able to allocate on the heap.
 * \throw std::bad_alloc In such cases.
 * 
 * \pre  The nonce should not be of size 0.
 * \pre  The key   should not be of size 0.
 * \note Otherwise, the function will fail.
 * 
 * \warning DO NOT UNDER NO CIRCUMSTANCES USE THIS ALGORITHM FOR REAL ENCRYPTION PURPOSES. It is not
 *			secure at all and is only meant for demonstration purposes.
 */
[[nodiscard]] constexpr std::error_code symmetricalToyEncryption(const DataViewer data, const KeyViewer key, const NonceViewer nonce, std::string& output)
{
    const auto lambdaXorCipher   = [](const std::uint8_t datum, const std::uint8_t curKey) -> std::uint8_t
    { return datum ^ curKey; };
    const auto lambdaComplement  = [](const std::uint8_t datum, const std::uint8_t curKey) -> std::uint8_t
    { return 255 - (datum + curKey); };
    const auto lambdaReverseBits = [](std::uint8_t datum) -> std::uint8_t
    {
        datum = ((datum & 0xF0) >> 4) | ((datum & 0x0F) << 4);
        datum = ((datum & 0xCC) >> 2) | ((datum & 0x33) << 2);
        datum = ((datum & 0xAA) >> 1) | ((datum & 0x55) << 1);
        return datum;
    };

    const std::size_t nonceSize{ nonce.size() };
    const std::size_t keySize{ key.size() };

    if (keySize == 0) [[unlikely]]
        return make_error_code(ErrorCode::emptyKey);

    if (nonceSize == 0) [[unlikely]]
        return make_error_code(ErrorCode::emptyNonce);

    output.reserve(data.size()); // This algorithm always encrypts one byte into one byte.

    for (std::size_t i{ 0 }; i < data.size(); ++i)
    {
        std::uint8_t datum{ data[i] };
        const std::uint8_t k_byte{ key[i % keySize] };
        const std::uint8_t n_byte{ nonce[i % nonceSize] };
        const std::uint8_t current_key{ static_cast<std::uint8_t>(k_byte ^ n_byte) };

        // f(h(g(h(f(x))))) is symmetrical here.
        datum = lambdaXorCipher(datum, current_key);
        datum = lambdaComplement(datum, current_key);
        datum = lambdaReverseBits(datum);
        datum = lambdaComplement(datum, current_key);
        datum = lambdaXorCipher(datum, current_key);

        output.push_back(static_cast<char>(datum));
    }

    return {};
    // Credits go to `wsecft` for the inspiration of this algorithm. I modified the original function so it can support
    // nonces. Further modifications were made to use std::span and std::error_code.
}

/**
 * \brief A simple key retriever for demonstration purposes.
 * \complexity O(N), where N is the size of the info string.
 * 
 * The size of the key is the maximum between 16 and the size of `info`.
 *
 * The key is in an unspecified state if the function fails.
 *
 * \param[in] info: the seed for the key retrieval.
 * \param[in] key: The key to generate
 * 
 * \return a `std::error_code` indicating success or failure.
 * 
 * \pre   Depending on the length of `info`, the program may not be able to allocate on the heap.
 * \throw std::bad_alloc In such cases. It's safe to assume it won't if your `info` size is short enough (IF you are
 *        not on embedded systems).
 * 
 * \warning DO NOT UNDER NO CIRCUMSTANCES USE THIS ALGORITHM FOR REAL ENCRYPTION PURPOSES. It is not secure at all and 
 *          is only meant for demonstration purposes.
 */
[[nodiscard]] constexpr std::error_code toyKeyRetriever(const InfoViewer info, KeyType& key)
{
    key.reserve(std::max(info.size(), 16uz));
    key.clear();

    for (const char c : info)
        key.push_back(c);

    // Ensure the key has a minimum length for security.
    while (key.size() < 16)
        key.push_back(char{}); // Padding. Sufficient for this toy example.

    return {};
}

/// This is a toy example of the encryption class. 
/// DO NOT UNDER NO CIRCUMSTANCES USE THIS ALGORITHM FOR REAL ENCRYPTION PURPOSES. It is not secure at all and is only 
/// meant for demonstration purposes.
export extern EncryptionWrapper g_toyWrapper;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Algorithms for the EncryptionWrapper instance.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // FileGuard namespace.