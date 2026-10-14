/**********************************************************************************************************************
 * \file   Streams.cppm Streams.cpp
 * \brief  Provides functions to manage files and streams.
 * 
 * All functions are wrappers of fstream (or similar).
 * 
 * These functions return `std::error_code` instead of bits or exceptions, even if an exception mask is set.  
 *
 * \copyright Boost Software License - Version 1.0 - August 17th, 2003
 * \author    omegaDIL, lesaffreismael@proton.me
 * \date      August 2025
 * 
 * \note Avoid using those functions if your project targets embedded systems. Most functions allocate on the heap, but
 *       it is generally so low that it won't throw `bad_alloc`. Hence, functions are noexcept.
 * \note All functions provided interact with files or streams which generally point to files. Don't expect things to
 *       be fast.
 * \note Functions are thread-safe only when operating on different files. Concurrent operations on the same file, even
 *       via different functions, are not thread-safe.
 * \note No functions are `constexpr` for obvious reason.
 *********************************************************************************************************************/

export module FileGuard:Streams;

import std;

namespace FileGuard
{

consteval void compileTimeError(std::string_view message)
{
    throw message;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Error category of FileGuard
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Represents the errors that may occur while using FileGuard.
 */
export enum class ErrorCode : std::uint8_t
{
    none = 0,
    unknown = 1,

    emptyKey     = 100,
    emptyNonce   = 101,
    invalidNonce = 102,

    cleaningFailed = 200, // Loss recovered (AllOrNothing).
    invalidNewer   = 201, // Partial loss   (AllOrNothing).
    noValidFile    = 202  // Complete loss  (AllOrNothing).
};

/**
 * \brief Custom error category for FileGuard errors.
 *
 * Get an instance of this class with `::getInstance()`. All constructors and assignment operators are deleted to
 * prevent instantiation outside of the class. This is a singleton class.
 *
 * All member functions are `noexcept` and `const`.
 *
 * \see `ErrorCode` to see the list of error codes that have a message.
 */
export struct Category : public std::error_category
{
private:

    /// Default and only constructor.
    constexpr Category() noexcept = default;

public:

    Category(const Category&) noexcept = delete;
    Category(Category&&) noexcept = delete;
    // All special functions are disabled

    /**
     * \brief Get an instance of this singleton class.
     */
    [[nodiscard]] static const Category& getInstance() noexcept;


    /**
     * \brief Returns the name of the error category.
     * \complexity O(1).
     */
    [[nodiscard]] const char* name() const noexcept override;

    /**
     * \brief Returns the error message corresponding to the given error code.
     * \complexity O(1).
     *
     * \param[in] ev: The error code value.
     *
     * \return The error message corresponding to the given error code, or a default message if the error code.
     */
    [[nodiscard]] std::string message(int ev) const override;
};

/// see `make_error_code` in the STL. This is an overload.
inline std::error_code make_error_code(ErrorCode e) noexcept
{
    return { static_cast<int>(e), Category::getInstance() };
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Error category of FileGuard
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions for handling	I/O with files.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Opens a file for reading.
 * \complexity O(1).
 * 
 * Checking `std::filesystem::exists()` may be worth it in terms of speed if your file might not exist as you can avoid
 * calling this function. Otherwise, this function will return the appropriate `std::error_code`. However, keep in 
 * mind that checking slows down the program. Overall, it is recommended to call `createFile()` and then call this
 * function. You can forget about `std::filesystem::exists()`.
 * 
 * `std::errc::no_such_file_or_directory` is returned if the file does not exist.
 *
 * \param[in] path: Path to the file.
 * \param[in] mode: Additional file mode. `std::ios::in` is always enabled.
 *
 * \return `std::fstream` if the function succeeded, `std::error_code` otherwise.
 *
 * \pre  The file must exist.
 * \pre  This function must have proper permissions to go to the file location.
 * \post A stream to the file will be opened in reading.
 * \note Otherwise, returns `std::error_code`.
 * 
 * \see `openWritingFile()`, `openIOFile()`.
 * \see `createFile()`.
 */
[[nodiscard]] std::expected<std::ifstream, std::error_code> openReadingFile(const std::filesystem::path& path, std::ios::openmode mode = std::ios::in) noexcept;

/**
 * \brief Opens a file for writing.
 * \complexity O(1).
 *
 * Doesn't truncate by default. You have to explicitly add the `std::ios::trunc` mode.
 * 
 * You do not need to check `std::filesystem::exist(path)`.
 * 
 * `std::errc::no_such_file_or_directory` is returned if the file does not exist.
 *
 * \param[in] path: Path to the file.
 * \param[in] mode: Additional file mode. `std::ios::in` and `std::ios::out` are always enabled.
 *
 * \return `std::fstream` if the function succeeded, `std::error_code{}` otherwise.
 *
 * \pre  The file should exist, be openable and writable for this function.
 * \post A stream to the file will be opened in writing.
 * \note Otherwise, returns `std::error_code`.
 * 
 * \see `openReadingFile()`, `openIOFile()`.
 * \see `createFile()`.
 * \see `closeWritingFile()`.
 */
[[nodiscard]] std::expected<std::ofstream, std::error_code> openWritingFile(const std::filesystem::path& path, std::ios::openmode mode = std::ios::out) noexcept;

/**
 * \brief Opens a file for writing and reading.
 * \see The doc of `openWritingFile()`.
 * 
 * \see `openWritingFile()`, `openReadingFile()`.
 * \see `createFile()`.
 * \see `closeWritingFile()`.
 */
[[nodiscard]] std::expected<std::fstream, std::error_code> openIOFile(const std::filesystem::path& path, std::ios::openmode mode = std::ios::in | std::ios::out) noexcept;


/**
 * \brief Reads from a stream.
 * \complexity O(N), where N is the amount of characters that will be read.
 *
 * How much is read is exactly `requestedAmount`.
 * 
 * `std::errc::resource_unavailable_try_again` is returned if `requestedAmount` is too large. It is useless to try
 * again with the same value for the argument. Try with a lower value.
 *
 * Accounts for the position of the cursor. The cursor is shifted by the amount of characters read.
 *
 * The stream can be non-seekable.
 * 
 * Doesn't clear eof flag at the end if reached.
 * 
 * To read the whole file, use `std::filesystem::file_size()`. You should use `std::ios::binary` for your stream in 
 * that case.
 * 
 * The mask does not matter. It is kept unchanged.
 * 
 * \param[out] stream: Where the data will be read.
 * \param[in] requestedAmount: The number of characters that will be read.
 *
 * \return a `std::error_code` indicating failure or the data read.
 *
 * \pre  At least `requestedAmount` of characters should be available after the cursor's pos.
 *       `std::errc::resource_unavailable_try_again` is returned. 
 * \pre  The stream should be valid. Will fail if eof.
 * \post The stream will remain valid. Wont fail if eof unless `requestedAmount` is not met (early eof).
 * \post `requestedAmount` of characters will be read.
 * \note Otherwise, returns `std::error_code`.
 * 
 * \note Be consistent with the use of `std::ios::binary` between reading and writing. Otherwise,
 *	     `requestedAmount` might not be as reliable.
 *
 * \see `overwriteStream`.
 */
[[nodiscard]] std::expected<std::string, std::error_code> readStream(std::istream& stream, std::uint32_t requestedAmount) noexcept;

/**
 * \brief Overwrites a stream.
 * \complexity O(N), where N is the amount of characters that will be written.
 *
 * How much is written is the minimum between `requestedAmount` and the size of `data`.
 *
 * Accounts for the position of the cursor. The cursor is shifted by the amount of characters read.
 *
 * The stream can be non-seekable.
 * 
 * The mask does not matter. It is kept unchanged.
 *
 * \param[in] data: The data that will be written.
 * \param[out] stream: Where the data will be written.
 * \param[in] requestedAmount: Max amount of characters that will be written.
 *
 * \return a `std::error_code` indicating success or failure.
 *
 * \pre  The stream should be valid.
 * \post The stream will remain valid and all characters needed will be written.
 * \note Otherwise, returns `std::error_code`.
 *
 * \note If you leave `requestedAmount` unchanged, it will write as many characters as there are in `data` (unless
 *       `requestedAmount` is larger than `data` which is unlikely).
 * \note Be consistent with the use of `std::ios::binary` between reading and writing. Otherwise, `requestedAmount`
 *       might not be as reliable.
 *
 * \see `readStream`.
 * \see `closeWritingFile()`.
 */
[[nodiscard]] std::error_code overwriteStream(std::ostream& stream, std::string_view data, std::uint32_t requestedAmount = std::numeric_limits<std::uint32_t>::max()) noexcept;

/**
 * \brief Flushes a writing stream to a file.
 * \complexity O(1).
 * 
 * Does not close the stream. The stream will remain valid after this function.
 *
 * If not opened, this function does nothing and return an empty `std::error_code`. 
 * 
 * Flushes the file even if the stream isn't set to goodbit. The bit is cleared. Any bad error code you may receive
 * happened during flushing, not before. This is useful if you want to flush a file regardless of other potential
 * previous errors.
 *
 * \param[out] stream: The file stream to flush.
 *
 * \return a `std::error_code` indicating success or failure.
 * 
 * \pre  This function should be able to flush the file.
 * \pre  Should have enough file space.
 * \pre  The permissions should not have been updated for this function.
 * \note Otherwise, the function will fail.
 *
 * \see `openWritingFile()`, `openIOFile()`.
 * \see `closeWritingFile()`.
 */
[[nodiscard]] std::error_code flushWritingFile(std::fstream& stream) noexcept;

/**
 * \see The overload taking `std::fstream`.
 */
[[nodiscard]] std::error_code flushWritingFile(std::ofstream& stream) noexcept;

/**
 * \brief Closes and flushes a writing stream to a file.
 * \complexity O(1).
 * 
 * You don't need to call `flushWritingFile()` before calling this function. This function will flush the stream before
 * closing it.
 * 
 * If not opened, this function does nothing and return an empty `std::error_code`.
 * 
 * Closes the file even if the stream isn't set to goodbit. The bit is cleared. Any bad error code you may receive
 * happened during closing, not before. This is useful if you want to close a file regardless of other potential
 * previous errors.
 * 
 * \param[out] stream: The file stream to close.
 *
 * \return a `std::error_code` indicating success or failure.
 *   
 * \pre  This function should be able to flush the file and close it.
 * \pre  Should have enough file space (leftover in the buffer).
 * \pre  The permissions should not have been updated for this function.
 * \note Otherwise, the function will fail.
 *
 * \see `openWritingFile()`, `openIOFile()`.
 * \see `flushWritingFile()`.
 */
[[nodiscard]] std::error_code closeWritingFile(std::fstream& stream) noexcept;

/**
 * \see The overload taking `std::fstream`.
 */
[[nodiscard]] std::error_code closeWritingFile(std::ofstream& stream) noexcept;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal functions for handling	I/O with files.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exported functions that are not present in `std::filesystem`.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Creates a file.
 * \complexity O(1).
 *
 * If `no_such_file_or_directory` is returned, the directory does not exist. It is not created.
 *
 * Does not fail if the file already exists. Returns an empty `std::error_code{}`.
 * Fails if the directory (if provided in `path`) does not exist though.
 * 
 * You do not need to check `std::filesystem::exist(path)`. 
 *
 * \param[in] path: Where to create the file.
 *
 * \return a `std::error_code` indicating success or failure.
 *
 * \pre  The directories in the path must exist.
 * \pre  This function should have proper permissions to go to the appropriate directory and create the file.
 * \post A file will exist under that name.
 * \note Otherwise, returns `std::error_code`.
 * 
 * \note Interacts with files. Don't expect things to be fast.
 * \note Is thread-safe only when operating on different files. Concurrent operations on the same file, even via other
 *       functions, are not thread-safe.
 * 
 * \see `std::filesystem`.
 */
export [[nodiscard]] std::error_code createFile(const std::filesystem::path& path) noexcept;

/**
 * \brief Zeroize a file if it exists.
 * \complexity O(N) where N is the number of character.
 *
 * The file is opened in binary mode.
 *
 * The file must exist. Considered an error otherwise.
 * 
 * Doesn't remove the file.
 * 
 * Is significantly slower then `std::filesystem::remove()`.
 * 
 * Is NOT military grade secure. Some filesystems might not overwrite the data on disk. Use appropriate libraries for
 * that.
 *
 * \param[in] path: The path to the file.
 *
 * \return a `std::error_code` indicating success or failure.
 *
 * \pre  The file must exist.
 * \pre  This function must have proper permissions to go to the file location.
 * \pre  The function should have proper permissions to open the file in writing.
 * \post A stream to the file will be opened in writing.
 * \note Otherwise, returns `std::error_code`.
 * 
 * \note Interacts with files. Don't expect things to be fast.
 * \note Is thread-safe only when operating on different files. Concurrent operations on the same file, even via other
 *       functions, are not thread-safe.
 * \note You should flush all streams connected to this file before calling this function.
 * \note This function takes 64 KB on the .rodata section. Be aware if you are in an embedded environment.
 * \warning BE EXTREMELY CAUTIOUS while using this function. There might be NO way to get your data back unless you
 *          made a backup.
 * 
 * \see `std::filesystem::resize_file()` if you want to truncate a file without zeroization
 * \see `std::filesystem::remove()` if you want to remove the file without zeroization (or use it after calling this
 *                                  function).
 */
export [[nodiscard]] std::error_code basicFileZeroing(const std::filesystem::path& path) noexcept;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exported functions that are not present in `std::filesystem`.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // FileGuard namespace

namespace std
{
    template <>
    struct is_error_code_enum<FileGuard::ErrorCode> : std::true_type {};
}   // std namespace.