/**********************************************************************************************************************
 * \file   AppendGuideline.cppm AppendGuidelineImpl.cpp
 * \brief  Provides the append guideline.
 *
 * \copyright Boost Software License - Version 1.0 - August 17th, 2003
 * \author    omegaDIL, lesaffreismael@proton.me
 * \date      June 2026
 *
 * \note Use the same guideline for each file.
 * \note Functions aren't `constexpr` for obvious reason.
 *********************************************************************************************************************/

export module FileGuard:Append;

import std;

#ifndef NDEBUG
#define NOEXCEPTDEBUG noexcept
#else
#define NOEXCEPTDEBUG
#endif  //NDEBUG

namespace FileGuard
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The Append guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief A guideline made to progressively add data at the end of a file. 
 * 
 * Ideal for logs, chats, and audit trails.
 * 
 * This is the most efficient guideline. It appends data at the end of the file without the overhead of seeking or
 * tracking internal positions. To read data, you can extract a specific number of characters starting backward from
 * the end of the file. The output string is not returned backwards though.
 * 
 * **Data Integrity & Safety:**
 * Previously written data is completely safe from corruption. The only form of corruption that can occur is a partial
 * write of your current data, which typically only happens during a sudden power outage.
 * 
 * **Limitations:**
 * - No Modifications: You cannot edit or overwrite previous data that has already been written.
 * - No Built-in Encryption: Encryption is not supported directly. You must encrypt your data before passing it to the
 *   guideline.
 *
 * **Performance:**
 * Highly scalable (writing) and suitable for files of any size.
 * 
 * **Usage & Lifecycle Instructions:**
 * - Initialization: While you can open a file directly at construction time for any instance of `AppendGuideline`,
 *   handling errors inside a constructor is difficult. Therefore, it is highly recommended to use the default
 *   constructor and explicitly open the file later using either `.open()` or `.openAndAppend()`.
 * - Explicit Closing: You **must** call `.close()` before an instance is destroyed. Because all functions are marked
 *   `noexcept`, you do not need to wrap your code in `try/catch` blocks (unless unrelated throwing functions are 
 *   used). This `noexcept` guarantee drastically reduces the number of conditional branches and error-handling paths
 *   in your code, minimizing the number of explicit `.close()` calls you have to write.
 * - Destructor Assertion: To enforce proper error tracking, the destructor will assert if the file is still open.
 *   While RAII would normally prevent memory leaks, relying on it to close the file silently hides potential I/O save
 *   errors. You must ensure that every valid execution path explicitly calls `.close()` to handle errors properly and 
 *   avoid triggering an assertion.
 * 
 * \note Prefer using the default constructor.
 * \warning Call `.close()` before the destructor to handle potential errors properly. Otherwise, will assert.
 */
export class AppendGuideline
{
private:

    std::fstream m_file{}; ///< The stream to the file.

public:

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
     * \note Prefer using the default constructor.
     * 
     * \see `.open()`.
     */
    [[nodiscard]] explicit AppendGuideline(const std::filesystem::path& path) NOEXCEPTDEBUG;
    
    /**
     * \brief Constructs an instance, opens a stream to a file and writes in it.
     * \complexity O(N), where N is the size of the data to add.
     *
     * Does not close the file nor flush it.
     * 
     * \param[in] path: The path of the file.
     * \param[in] add:  The data to add at the end of the file.
     *
     * \pre  The file should exist, be openable and writable for this function.
     * \pre  Should have enough file space.
     * \post A stream to the file will be opened and data will be written.
     * \warning In debug, otherwise, asserts
     * \throw std::ios_base::failure In release, otherwise.
     *
     * \note Prefer using the default constructor.
     *
     * \see `.openAndAppend()`.
     */
    [[nodiscard]] AppendGuideline(const std::filesystem::path& path, std::string_view add) NOEXCEPTDEBUG;

    [[nodiscard]] AppendGuideline() noexcept = default;
    [[nodiscard]] AppendGuideline(AppendGuideline&&) noexcept = default;
    [[nodiscard]] AppendGuideline& operator=(AppendGuideline&&) & noexcept = default;
    ~AppendGuideline() noexcept; /// Asserts in debug if the file is not closed.
    // Copy functions are deleted because the file stream is not copyable.


    /**
     * \brief Opens the file to the given path
     * \complexity O(1).
     * 
     * Closes the file if one was already opened. You do not need to call `ensureFlushed()`.
     * 
     * The file is opened in binary mode.
     *
     * If you immediately append data after opening, you may want to use `.openAndAppend()`.
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
     * \see `openAndAppend()`.
     */
    [[nodiscard]] std::error_code open(const std::filesystem::path& path) noexcept;

    /**
     * \brief Opens the file to the given path and appends data.
     * \complexity O(N), where N is the size of the data.
     *
     * Closes the file if one was already opened. You do not need to call `ensureFlushed()`.
     *
     * The file is opened in binary mode.
     *
     * `std::errc::not_enough_memory` is returned when there is not enough memory to write data.
     *
     * `std::errc::no_such_file_or_directory` is returned if the file does not exist.
     *
     * \param[in] path: The path to the file to open.
     * \param[in] data: The data to add at the end of the file.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  The file should exist, be openable and writable for this function.
     * \pre  Should have enough file space.
     * \pre  This function should be able to close the file if one is already opened.
     * \post A stream to the file will be opened.
     * \post data will be written.
     * \note Otherwise, the function will fail.
     *
     * \see `open()`.
     * \see `append()`.
     */
    [[nodiscard]] std::error_code openAndAppend(const std::filesystem::path& path, std::string_view data) noexcept;

    /**
     * \brief Appends data at the end.
     * \complexity O(N), where N is the size of the data..
     *
     * Fails if the file has not been opened. `std::errc::invalid_argument` is returned.
     *
     * `std::errc::not_enough_memory` is returned when there is not enough memory to write data.
     *
     * \param[in] data: The data to add at the end of the file.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  The file should be opened.
     * \pre  Should have enough file space.
     * \post data will be written.
     * \note Otherwise, the function will fail.
     *
     * \see `openAndAppend()`.
     */
    [[nodiscard]] std::error_code append(std::string_view data) noexcept;

    /**
     * \brief Reads the last characters from the file and closes the file.
     * \complexity O(N + M), where N is the amount of characters that will be read. M is the amount of characters that
     *                       will need to be flushed before reading.
     *
     * Automatically flushes the buffer. You do not need to call `ensureFlushed()`.
     *
     * Fails if the file has not been opened. `std::errc::invalid_argument` is returned.
     *
     * `charactersToExtract` should be within bounds of the file. If it is greater than the number of bytes in the
     * file, an error will be returned. It may be `resource_unavailable_try_again` but it is not guaranteed. Try with a
     * lower value
     *
     * You cannot continue appending after calling this function.
     *
     * You only need to call `.close()` if the function returns a non-zero error code. On a successful execution (error
     * code is 0), the file is closed automatically. If this function fails, call either `.open()/openAndAppend()` or
     * `.close()`.
     *
     * `std::errc::not_enough_memory` is returned when there is not enough memory to load data.
     *
     * \param[in] charactersToExtract: The number of characters to read from the end of the file.
     *
     * \return a `std::error_code` indicating failure or the data read.
     *
     * \pre  The file should be opened.
     * \pre  This function should be able to flush the file and close it.
     * \pre  Should have enough file space for potential leftover in the buffer.
     * \post The file will be read.
     * \post the file will be closed if no error occurred.
     * \note Otherwise, this function will fail.
     *
     * \see `readAndRestore()`.
     */
    [[nodiscard]] std::expected<std::string, std::error_code> readAndClose(std::uint32_t charactersToExtract) noexcept;

    /**
     * \brief Reads the last characters from the file and restores its cursor to continue appending.
     * \complexity O(N + M), where N is the amount of characters that will be read. M is the amount of characters that
     *                       will need to be flushed before reading.
     * 
     * Automatically flushes the buffer. You do not need to call `ensureFlushed()`.
     *
     * Fails if the file has not been opened. `std::errc::invalid_argument` is returned.
     * 
     * `charactersToExtract` should be within bounds of the file. If it is greater than the number of bytes in the
     * file, an error will be returned. It may be `resource_unavailable_try_again` but it is not guaranteed. Try with a
     * lower value
     * 
     * You can continue appending after calling this function.
     * 
     * The file is restored regardless of whether the read operation was successful.
     * 
     * `std::errc::not_enough_memory` is returned when there is not enough memory to load data.
     *
     * \param[in] charactersToExtract: The number of characters to read from the end of the file.
     * 
     * \return a `std::error_code` indicating failure or the data read.
     * 
     * \pre  The file should be opened.
     * \pre  This function should be able to flush the file.
     * \pre  Should have enough file space for potential leftover in the buffer.
     * \post The file will be read.
     * \note Otherwise, this function will fail.
     * 
     * \see `readAndClose()`.
     */
    [[nodiscard]] std::expected<std::string, std::error_code> readAndRestore(std::uint32_t charactersToExtract) noexcept;

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
    [[nodiscard]] std::error_code ensureFlushed() noexcept;

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
     * \param[in] ec: The initial error code.
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
    [[nodiscard]] std::error_code close(std::error_code ec = std::error_code{}) noexcept;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The Append guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // FileGuard namespace.