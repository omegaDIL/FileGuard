/**********************************************************************************************************************
 * \file   IndexedGuideline.cppm IndexedGuidelineImpl.cpp
 * \brief  Provides the indexed guideline.
 *
 * \copyright Boost Software License - Version 1.0 - August 17th, 2003
 * \author    omegaDIL, lesaffreismael@proton.me
 * \date      July 2026
 *
 * \note Use the same wrapper and guideline for each file.
 * \note Functions aren't `constexpr` for obvious reason.
 *********************************************************************************************************************/
module;

#include <cassert>

export module FileGuard:Indexed;
import :Streams;
import :Encryption;

import std;

#ifndef NDEBUG
#define NOEXCEPTDEBUG noexcept
#else
#define NOEXCEPTDEBUG
#endif  //NDEBUG

namespace FileGuard
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internals for this guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Reads `blocks` blocks of each `length` characters, starting at `seekingPos` * `length`, in the file stream.
 * \complexity O(N), where N is blocks * length.
 *
 * Automatically flushes the buffer. You do not need to call `ensureFlushed()`.
 *
 * Fails if the file is not opened. `std::errc::invalid_argument` is returned.
 * 
 * `seekingPos` should be within bounds of the file. If it is greater than the number of bytes in the file, an error
 * will be returned. It may be `resource_unavailable_try_again` but it is not guaranteed. Try with a lower value.
 * 
 * `std::errc::not_enough_memory` is returned when there is not enough memory to load data.
 * 
 * \param[in] file:       The file where the data are.
 * \param[in] seekingPos: The position within the file where the reading will begin.
 * \param[in] length:     The length of one block.
 * \param[in] blocks:     The number of block to read.
 * 
 * \return a `std::error_code` indicating failure or the data read.
 * 
 * \pre  The file/stream should be valid (invalid_argument)
 * \pre  The seeking operation should be within bounds
 * \pre  This function should be able to flush the file.
 * \pre  Should have enough file space for potential leftover in the buffer.
 * \post The file will be read.
 * \note The function will fail otherwise.
 * 
 * \see `indexedWrite()`.
 */
std::expected<std::string, std::error_code> indexedRead(std::fstream& file, std::uint32_t seekingPos, std::uint32_t length, std::uint32_t blocks) noexcept;

/**
 * \brief Writes `data`, starting at `seekingPos` * `length`, in the file stream. 
 * \complexity O(N), where N is blocks * length.
 *
 * If you seek past the end, a file hole will be created. This is expected to happen and will not result in an error.
 *
 * Fails if the file is not opened. `std::errc::invalid_argument` is returned.
 *
 * `std::errc::not_enough_memory` is returned when there is not enough memory to write data.
 *
 * \param[out] file:       The file where the data will be written.
 * \param[in]  data:       The data to write.
 * \param[in]  seekingPos: The position within the file where the reading will begin.
 * \param[in]  length:     The length of one block.
 *
 * \return a `std::error_code` indicating failure or success.
 *
 * \pre  The file/stream should be valid (invalid_argument)
 * \pre  This function should be able to flush the file.
 * \pre  Should have enough file space.
 * \post Data will be written.
 * \note The function will fail otherwise.
 *
 * \see `indexedRead()`.
 */
std::error_code indexedWrite(std::fstream& file, std::string_view data, std::uint32_t seekingPos, std::uint32_t length) noexcept;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internals for this guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The Indexed guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief A guideline made to edit, add, remove data wherever you want in a file.
 *
 * Ideal for large or complex files.
 *
 * Data is stored in fixed-size blocks next to each other without overlapping. All blocks are of the same size, which
 * is determined by the sum of the template arguments `DataLength` and `NonceLength`.
 * 
 * To write/read a block, you must know its index. The index is a number that identifies the block in the file, that is
 * equal to the block's position in the file. For example, the first block is at index 0, the second at index 1, and so
 * on.
 * 
 * If you use an index that is right at the eof, the file will be extended. If the index is past the eof, a file hole
 * will be created. You can still access that element past the file hole as long as you know its index. Its position
 * remains fixed and will not be moved right next to the eof.
 * 
 * Whether the disk allocates space for this file hole is determined by the OS (usually not allocated). This also means
 * that the behavior when reading a file hole is OS specific. In practice, it is usually read back as zeroes. That
 * being said, while creating a file hole is expected to happen, it is not recommended to read from it.
 * 
 * To shorten a file, you can use `std::filesystem::resize_file()`.
 * 
 * `NonceLength` must be `0` at compile time when NO wrapper is provided. When a wrapper is provided, `NonceLength`
 * must NOT be `0` at compile time. At runtime, the actual wrapper's nonce size must match `NonceLength` otherwise the
 * guideline will fail (asserts or exceptions, see the constructor's doc).
 *
 * **Data Integrity & Safety:**
 * This guideline is not corruption free. If the program crashes during a flush operation on a file, the given block
 * may end up being partially written and corrupted (though this is rare in practice). However, the rest of the file
 * will remain intact. This is because every blocks are independent and have their own fixed index.
 *
 * **Limitations:**
 * - Encryption slows down operations.
 * - You manage indexes yourself. This is notoriously **very** error-prone (though unlike typical memory management,
 *   there are no memory leaks or undefined behaviors, just erroneous behavior).
 * - Performance is limited by the I/O buffer size (usually 4KB but is OS-dependent). To fully benefit from the I/O
 *   buffer, you should limit the number of flushes you make. A flush happens every time you read (if buffer not
 *   empty), write OR when the buffer is full. Hence, try to put as much data as possible in the buffer before reading
 *   without exceeding its size. To achieve this, you can either increase the `DataLength` template argument or
 *   read/write several blocks at once. Keep in mind that the size of the nonce will increase the size of each block.
 * 
 * **Performance:**
 * Highly scalable and suitable for files of any size. However, to fully benefit from the I/O buffer, you should limit
 * the number of flushes you make. A flush happens every time you read (if buffer not empty), write or when the buffer
 * is full. Therefore, prefer fewer, larger reads and writes. Reading/writing several blocks at once is a good idea
 * because it'll fill the buffer more efficiently and reduce the number of flushes (any big enough writes/reads will
 * flush full buffers). 
 *
 * **Security & Encryption:**
 * Encryption is supported natively. When encryption is enabled, the guideline automatically manages and stores the
 * cryptographic nonce immediately after each data block, increasing their size by the nonce's size. A new nonce is
 * generated for every block write operation, even though that slows down operations.
 *
 * **Usage & Lifecycle Instructions:**
 * - Initialization: While you can open a file directly at construction time for any instance of `IndexedGuideline`,
 *   handling errors inside a constructor is difficult. Therefore, it is highly recommended to use a non opening
 *   constructor and explicitly open the file later using either `.open()`.
 * - Explicit Closing: You **must** call `.close()` before an instance is destroyed.
 * - Destructor Assertion: To enforce proper error tracking, the destructor will assert if the file is still open.
 *   While RAII would normally prevent memory leaks, relying on it to close the file silently hides potential I/O save
 *   errors. You must ensure that every valid execution path explicitly calls `.close()` to handle errors properly and
 *   avoid triggering an assertion.
 *
 * **Index management**
 * - Indexes are plain byte offsets that you choose and track yourself; the guideline does not allocate or remember
 *   them for you.
 * - Once you pick an index for a piece of data, keep it fixed for the file's lifetime: there is no way to relocate or
 *   delete a block, only to overwrite it.
 * - Every block occupies exactly `DataLength + NonceLength` bytes, and that size never changes for a given indexed
 *   guideline. Two distinct consecutive indexes are spaced a full block apart so they never overlap.
 * - When reading or writing several blocks together, consecutive indexes must line up exactly.
 * - Do not fear UB or memory leaks: if you mismanage indexes, the worst that can happen is erroneous behavior.
 * 
 * \note Prefer using the default constructor or the encryption wrapper constructor.
 * \warning Call `.close()` before the destructor to handle potential errors properly. Otherwise, will assert.
 * \warning Each instance of this guideline uses a pointer to your `EncryptionWrapper` instance. DO NOT DEALLOCATE IT.
 *          Otherwise, there will be UB. (when encryption is enabled only)
 */
export template<std::uint32_t DataLength, std::uint32_t NonceLength = 0u>
class IndexedGuideline
{
    static_assert(DataLength > 0, "Length must be greater than 0");

private:

    std::fstream m_file{}; ///< The stream to the file.

    [[maybe_unused]] EncryptionWrapper* m_wrapper{ nullptr }; ///< This wrapper is used when the nonce is not equal to 0. Otherwise, this is dead code (the compiler will remove it).


    /**
     * \brief Decrypts data, block by block, each using their nonce.
     * \complexity O(N + ?), where N is the size of the data and ? is the decrypt complexity of the wrapper used.
     * 
     * `std::errc::not_enough_memory` is returned when there is not enough memory to decrypt data.
     *
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * wrapper-specific exceptions may still throw. You are safe if it doesn't throw.
     * 
     * \param[in] data:   The data to decrypt.
     * \param[in] blocks: The number of blocks in data
     * 
     * \return a `std::error_code` indicating failure or the data decrypted.
     * 
     * \pre  The key must have been generated. (EncryptionWrapper::Code::emptyKey)
     * \pre  The custom decryption function must succeed.
     * \post The decryption will be performed.
     * \note If not, the function will fail.
     *
     * \pre  The encryption wrapper you use should be the same one you called to write data.
     * \pre  Each block must follow the expected structure of having data first and then the nonce (with their
     *       according size.
     * \pre  The data must have the correct size, a multiple of the template argument `DataLength`.
     * \post You will get the decrypted data.
     * \note If not, the data won't be readable in that case.
     * 
     * \note Failure and exceptions also depend on the encryption wrapper if provided.
     * \note YOU handle the key (generating and clearing).
     * 
     * \see `encrypt`
     */
    [[nodiscard]] [[maybe_unused]] std::expected<std::string, std::error_code> decrypt(const std::string_view data, const std::uint32_t blocks) const
    {
        /// The number of characters that makes up one block.
        constexpr std::uint32_t lengthPerBlock{ DataLength + NonceLength };
        /// The total number of characters that makes up each data block.
        const std::uint32_t totalBlockBytes{ lengthPerBlock * blocks };

        if (totalBlockBytes != data.size()) [[unlikely]]
            return std::unexpected{ std::make_error_code(std::errc::invalid_argument) };

        std::string decrypted{};
        decrypted.reserve(DataLength * blocks); // Decrypted does not include nonces.

        std::string curDecryptedIndex{};

        for (std::uint32_t offset{ 0 }; offset < totalBlockBytes; offset += lengthPerBlock)
        {
            const std::string_view wholeIndex{ data.data() + offset, lengthPerBlock };

            if (const std::error_code ec{ m_wrapper->decryptWithNoneAppended(wholeIndex, curDecryptedIndex) }) [[unlikely]]
                return std::unexpected{ ec };

            decrypted += curDecryptedIndex;

            const std::span curDecryptedIndexSpan(reinterpret_cast<std::uint8_t*>(curDecryptedIndex.data()), curDecryptedIndex.size());
            zeroizeValue(curDecryptedIndexSpan);
        }

        return decrypted;
    }

    /**
     * \brief Encrypts data, block by block, each using a new nonce.
     * \complexity O(N + ?), where N is the size of the data and ? is the encrypt complexity of the wrapper used.
     * 
     * `std::errc::not_enough_memory` is returned when there is not enough memory to encrypt data.
     *
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * wrapper-specific exceptions may still throw. You are safe if it doesn't throw.
     * 
     * \param[in] data: The data to decrypt.
     *
     * \return a `std::error_code` indicating failure or the data encrypted.
     * 
     * \pre  The data must have the correct size, a multiple of the template argument `DataLength`.
     * \post The data will fit in each block.
     * \note Otherwise, the function returns std::errc::invalid_argument.
     *
     * \pre  The key must have been generated. (EncryptionWrapper::Code::emptyKey)
     * \pre  The custom encryption function must succeed.
     * \post The encryption will be performed.
     * \note If not, the function will fail.
     *
     * \note Failure and exceptions also depend on the encryption wrapper if provided.
     * \note YOU handle the key (generating and clearing).
     * 
     * \see `decrypt`
     */
    [[nodiscard]] [[maybe_unused]] std::expected<std::string, std::error_code> encrypt(const std::string_view data) const
    {
        /// The number of characters to encrypt.
        const std::uint32_t totalInputBytes{ static_cast<std::uint32_t>(data.size()) };
        /// The number of data blocks to encrypt.
        const std::uint32_t numberOfBlocks{ totalInputBytes / DataLength };
        /// The number of characters needed to store all nonces for each block.
        const std::uint32_t totalNonceBytes{ NonceLength * numberOfBlocks };

        /// The total number of characters that makes up data (nonce excluded) in each block.
        const std::uint32_t totalDataBytes{ DataLength * numberOfBlocks };
        if (totalDataBytes != totalInputBytes) [[unlikely]]
            return std::unexpected{ std::make_error_code(std::errc::invalid_argument) };

        std::string encrypted{};
        encrypted.reserve(totalInputBytes + totalNonceBytes);

        for (std::uint32_t offset{ 0 }; offset < totalDataBytes; offset += DataLength)
        {
            const std::string_view wholeIndex{ data.data() + offset, DataLength };

            const auto curNonce{ EncryptionWrapper::getCryptoNonce(m_wrapper->getNonceSize()) };

            std::string curEncryptedIndex{};
            if (const std::error_code ec{ m_wrapper->encryptAndAddNonce(wholeIndex, curNonce, curEncryptedIndex) }) [[unlikely]]
                return std::unexpected{ ec };

            encrypted += curEncryptedIndex;
        }

        return encrypted;
    }

public:

    /// Statically asserts if the nonce length is not 0.
    [[nodiscard]] IndexedGuideline() noexcept
    {
        static_assert(NonceLength == 0, "The nonce size should be 0 given that you did NOT provide an encryption wrapper.");
    }

    /**
     * \brief Constructs an instance without opening a stream to a file.
     * \complexity O(1).
     *
     * \param[in] wrapper: The path of the file.
     * 
     * \pre The nonce length template should not be 0.
     * \warning will fail to compile.
     * 
     * \pre The nonce length template should fit the wrapper's nonce size.
     * \warning In debug, otherwise, asserts
     * \throw std::invalid_argument In release, otherwise.
     *
     * \warning Each instance uses a pointer to the `EncryptionWrapper` instance. DO NOT DEALLOCATE IT. Otherwise,
     *          there will be UB.
     */
    [[nodiscard]] explicit IndexedGuideline(EncryptionWrapper& wrapper) NOEXCEPTDEBUG
        : m_wrapper{ &wrapper }
    {
        static_assert(NonceLength != 0, "The nonce size should NOT be 0 given that you provided an encryption wrapper.");

#ifndef NDEBUG
        assert(wrapper.getNonceSize() == NonceLength && "The nonce size does not fit");
#else   //NDEBUG
        if (wrapper.getNonceSize() != NonceLength) [[unlikely]]
            throw std::invalid_argument{ "The nonce size does not fit" };
#endif  //NDEBUG
    }

    /**
     * \brief Constructs an instance and opens a stream to a file.
     * \complexity O(1).
     *
     * \param[in] path: The path of the file.
     *
     * \pre  The file should exist, be openable and writable for this function.
     * \post A stream to the file will be opened.
     * \warning In debug, otherwise, asserts
     * \throw std::ios_base::failure In release, otherwise.
     * 
     * \pre The nonce length should be 0.
     * \warning will fail to compile.
     *
     * \note Prefer using the default constructor.
     *
     * \see `.open()`.
     * \see `.close()`
     */
    [[nodiscard]] explicit IndexedGuideline(const std::filesystem::path& path) NOEXCEPTDEBUG
        : IndexedGuideline{} // statically asserts if nonce size is not 0
    {
        const std::error_code ec{ this->open(path) };

#ifndef NDEBUG
        assert(!ec && "IndexedGuideline failed to open the file in the constructor.");
#else   //NDEBUG
        if (ec) [[unlikely]] 
            throw std::ios_base::failure{ ec.message() };
#endif  //NDEBUG
    }

    /**
     * \brief Constructs an instance and opens a stream to a file.
     * \complexity O(1).
     *
     * \param[in] path: The path of the file.
     * \param[in] wrapper: The wrapper to use for encryption.
     *
     * \pre  The file should exist, be openable and writable for this function.
     * \post A stream to the file will be opened.
     * \warning In debug, otherwise, asserts
     * \throw std::ios_base::failure In release, otherwise.
     *
     * \pre The nonce length template should not be 0.
     * \warning will fail to compile.
     *
     * \pre The nonce length template should fit the wrapper's nonce size.
     * \warning In debug, otherwise, asserts
     * \throw std::invalid_argument In release, otherwise.
     *
     * \note Prefer using the constructor that takes a encryption wrapper only.
     * \warning Each instance uses a pointer to the `EncryptionWrapper` instance. DO NOT DEALLOCATE IT. Otherwise,
     *          there will be UB.
     *
     * \see `.open()`.
     * \see `.close()`
     */
    [[nodiscard]] IndexedGuideline(const std::filesystem::path& path, EncryptionWrapper& wrapper) NOEXCEPTDEBUG
        : IndexedGuideline{ wrapper } // statically asserts if nonce size is 0
    {
        const std::error_code ec{ this->open(path) };

#ifndef NDEBUG
        assert(!ec && "IndexedGuideline failed to open the file in the constructor.");
#else   //NDEBUG
        if (ec) [[unlikely]] 
            throw std::ios_base::failure{ ec.message() };
#endif  //NDEBUG
    }

    /// Asserts in debug if the file is not closed. Does not clear the key.
    ~IndexedGuideline()
    {
#ifndef NDEBUG
        assert(!m_file.is_open() && "You forget to close the file of a IndexedGuideline instance. Be sure to call .close().");
#endif  //NDEBUG 
    }

    [[nodiscard]] IndexedGuideline(IndexedGuideline&&) noexcept = default;
    [[nodiscard]] IndexedGuideline& operator=(IndexedGuideline&&) & noexcept = default;
    // Copy functions are deleted because the file stream is not copyable.


    /**
     * \brief Opens the file to the given path
     * \complexity O(1).
     *
     * Closes the file if one was already opened.
     * 
     * The file is opened in binary mode.
     *
     * `std::errc::no_such_file_or_directory` is returned if the file does not exist.
     *
     * \param[in] path: The path to the file to open.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  The file should exist, be openable and writable for this function.
     * \pre  This function should be able to close the file if one is already opened.
     * \post A stream to the file will be opened.
     * \note Otherwise, the function will fail.
     *
     * \see `.close()`
     */
    [[nodiscard]] std::error_code open(const std::filesystem::path& path) noexcept
    {
         // Close the file if one. No effect if not opened.
        if (const std::error_code ec = closeWritingFile(m_file)) [[unlikely]]
            return ec;

        auto result{ openIOFile(path, std::ios::binary) };
        if (result) [[likely]]
            m_file = std::move(*result);

        return result.error_or({});
    } 

    /**
     * \brief Reads `blocks` blocks of data, starting at `seekingPos`, in the file.
     * \complexity O(N), where N is blocks * length.
     *
     * Automatically flushes the buffer. You do not need to call `ensureFlushed()`.
     *
     * Fails if the file has not been opened. `std::errc::invalid_argument` is returned.
     *
     * `seekingPos` should be within bounds of the file. If it is greater than the number of bytes in the file, an 
     * error will be returned. It may be `resource_unavailable_try_again` but it is not guaranteed. Try with a lower
     * value. If you seek in a file hole, whether `resource_unavailable_try_again` is returned is OS specific.
     *
     * `std::errc::not_enough_memory` is returned when there is not enough memory to read or decrypt data.
     *
     * Is `noexcept` if no encryption is needed.
     * 
     * Although not marked as `noexcept` when encryption is needed, this function is designed to avoid throwing
     * exceptions. However, wrapper-specific exceptions may still throw. You are safe if it doesn't throw.
     * 
     * \param[in] seekingPos: The position within the file where the reading will begin.
     * \param[in] blocks:     The number of block to read. Reading 0 blocks will return an empty string (on success).
     *
     * \return a `std::error_code` indicating failure or the data read.
     * 
     * \pre  The file should be opened.
     * \pre  The seeking operation should be within bounds
     * \pre  This function should be able to flush the file.
     * \pre  Should have enough file space for potential leftover in the buffer.
     * \post The file will be read.
     * \note The function will fail otherwise.
     * 
     * \pre  The key must have been generated (if encryption). (EncryptionWrapper::Code::emptyKey)
     * \pre  The custom decryption function must succeed.
     * \post The decryption will be performed.
     * \note If not, the function will fail.
     * 
     * \note Failure and exceptions also depend on the encryption wrapper if provided.
     * \note YOU handle the key (generating and clearing).
     *
     * \see `write()`.
     */
    [[nodiscard]] std::expected<std::string, std::error_code> read(const std::uint32_t seekingPos, const std::uint32_t blocks) noexcept(NonceLength == 0)
    {
        std::expected data{ indexedRead(m_file, seekingPos, DataLength + NonceLength, blocks) };
        m_file.clear();

        if constexpr (NonceLength != 0)
        {   // Decrypt data
            if (!data) [[unlikely]] return data;
            
            return this->decrypt(*data, blocks);
        }
        else
        {
            return data;
        }
    }
    
    /**
     * \brief Writes `data`, starting at `seekingPos`, in the file.
     * \complexity O(N), where N is the size of `data`.
     *
     * Fails if the file has not been opened. `std::errc::invalid_argument` is returned.
     * 
     * `std::errc::invalid_argument` is also returned if the character count doesn't meet the template argument.
     *
     * If you seek past the end, a file hole will be created. This is expected to happen and will not result in an error.
     *
     * `std::errc::not_enough_memory` is returned when there is not enough memory to write or encrypt data.
     *
     * Is `noexcept` if no encryption is needed.
     * 
     * Although not marked as `noexcept` when encryption is needed, this function is designed to avoid throwing
     * exceptions. However, wrapper-specific exceptions may still throw. You are safe if it doesn't throw.
     *
     * \param[in] data:       The data to write.
     * \param[in] seekingPos: The position within the file where the reading will begin.
     *
     * \return a `std::error_code` indicating failure or success.
     * 
     * \pre  The file should be opened.
     * \pre  This function should be able to flush the file.
     * \pre  Should have enough file space.
     * \pre  The data should be a multiple of `DataLength`. (std::errc::invalid_argument)
     * \post Data will be written.
     * \note The function will fail otherwise.
     *
     * \pre  The key must have been generated (if encryption). (EncryptionWrapper::Code::emptyKey)
     * \pre  The custom encryption function must succeed.
     * \post The encryption will be performed.
     * \note If not, the function will fail.
     *
     * \note Failure and exceptions also depend on the encryption wrapper if provided.
     * \note YOU handle the key (generating and clearing).
     *
     * \see `read()`.
     */
    [[nodiscard]] std::error_code write(const std::string_view data, const std::uint32_t seekingPos) noexcept(NonceLength == 0)
    {
        if (data.size() % DataLength != 0) [[unlikely]]
            return std::make_error_code(std::errc::invalid_argument);

        std::error_code result{};
        if constexpr (NonceLength != 0)
        {   // Encrypt data
            std::expected encrypted{ encrypt(data) }; // Must not be outlived by `data` in the local scope.
            if (!encrypted) [[unlikely]] 
                return encrypted.error();
            
            result = indexedWrite(m_file, *encrypted, seekingPos, DataLength + NonceLength);
        }
        else
        {
            result = indexedWrite(m_file, data, seekingPos, DataLength);
        }

        m_file.clear();
        return result;
    }

    /**
     * \brief Ensures any appended data is written to the file.
     * \complexity O(N) where N is the number of bytes to write in the buffer.
     *
     * This function does not close the file. You can continue to append data after calling this function without
     * reopening the file.
     *
     * You don't need to call this function if you call `close()` right after.
     *
     * No effect if the file is not opened or if it doesn't exist.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  This function should be able to flush the file.
     * \pre  Should have enough file space.
     * \post The stream will be flushed.
     * \note Otherwise, the function will fail.
     */
    [[nodiscard]] std::error_code ensureFlushed() noexcept
    {
        return flushWritingFile(m_file);
    }

    /**
     * \brief Closes the file if it is open.
     * \complexity O(N) where N is the number of bytes to write in the buffer.
     *
     * You must reopen the file if you want to append more data after calling this function.
     *
     * No effect if the file is not opened or if it doesn't exist.
     * 
     * Even if the guideline failed during a previous call, you must call this function to close the stream.
     *
     * Regardless of the error code you passed to this function, the function will attempt to close the file.
     *
     * \return If the error code you passed is not 0, that error code will be returned. Otherwise, the
     * `std::error_code` of this function indicating success or failure is returned.
     *
     * \pre  This function should be able to close the file.
     * \pre  Should have enough file space (leftover in the buffer).
     * \post The stream will be flushed.
     * \note Otherwise, the function will fail.
     *
     * \warning Be sure to call this function before destruction any instance.
     */
    [[nodiscard]] std::error_code close(std::error_code ec = std::error_code{}) noexcept
    {
        const std::error_code closeEc{ closeWritingFile(m_file) };

        // The error code passed as argument is prioritized because it is the first one that happened.
        return (ec) ? ec : closeEc;
    }

    /**
     * \see `close()`.
     * \brief Closes the file if it is open and clears the key.
     * \complexity O(N+M) where N is the number of bytes to write in the buffer and M is the number of bytes to clear.
     * 
	 * \note This function is only available when the nonce length is not 0.
     */
	[[nodiscard]] std::error_code closeAndClearKey(std::error_code ec = std::error_code{}) noexcept requires (NonceLength != 0)
    {
		m_wrapper->clearKey(); // m_wrapper is guaranteed to be not null because NonceLength != 0

        return close(ec);
    }

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The Indexed guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // FileGuard namespace.