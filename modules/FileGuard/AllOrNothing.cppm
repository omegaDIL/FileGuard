/**********************************************************************************************************************
 * \file   AllOrNothing.cppm AllOrNothing.cpp
 * \brief  Provides the all or nothing guideline.
 *
 * \copyright Boost Software License - Version 1.0 - August 17th, 2003
 * \author    omegaDIL, lesaffreismael@proton.me
 * \date      June 2026
 * 
 * \note Use the same wrapper and guideline for each file.
 * \note Functions aren't `constexpr` for obvious reason.
 *********************************************************************************************************************/

export module FileGuard:AllOrNothing;
import :Encryption;

import std;

namespace FileGuard
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internals for this guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief Checks if a file is valid by verifying the presence of the confirmation string at the end.
 * \complexity O(1).
 *
 * The file is opened in binary mode.
 *
 * \param[in] path: Path to the file.
 *
 * \return `true` if valid.
 *
 * \note The content is loaded into a string, so be aware of that conversion as it may alter data.
 *
 * \see `restoreFile()`.
 */
[[nodiscard]] bool isFileValid(const std::filesystem::path& path) noexcept;

/**
 * \brief Checks the integrity of a file and restores it if possible.
 * \complexity O(1) if everything is fine.
 * \complexity file copy O(N) if a deletion is needed, where N is the size of the file.
 *
 * When using the `AllOrNothing` guideline, a copy is edited to ensure that the original version remains safe if there
 * is a crash. Then, the copy replaces the original. This function checks if a crash interrupted this process and
 * choose the proper file if needed.
 *
 * The original file is named `path`. The other (the copy) is named `path + .tmp`. To check the validity of the file,
 * the function will look for both and choose the one there is left.
 *
 * If both exist, the end of the files are checked. They are supposed to contain a string confirmation stored at last.
 * If a crash occurs mid-writing, that file is incomplete and this string is missing. If a file has that string, it is
 * chosen over the other one.
 *
 * If both have the string, the copy (named `path + .tmp`) is prioritized because it is the newer version.
 *
 * The error code returned reports a potential crash, and how concerning it is. The type is `ErrorCode`. When
 * `invalidNewer` is returned, the data is outdated but valid (you can proceed but alert the user). When `noValidFile`
 * is returned, the data is corrupted or lost (not supposed to happen unless the user directly interact with files).
 * Others like `none` (no crash), `noMainFileRec` and `oldFileNotRemoved` tells you that while a crash may have happened,
 * everything is back to normal.
 *
 * Any error code that does not have a error category of `FileGuard::Category` represents failures while executing this
 * function (internal STL functions failed).
 *
 * The older version (named `path`) is supposed to be valid at all time (that is to say having the string confirmation).
 * Yet, if none has this string, `NoValidFile` is returned. The main reason why that would happen is the user that
 * would alter data themselves.
 *
 * Normally, there is only one file left after the execution. This may not be the case if the function fails to access
 * files (any `std::generic_category`) or `noValidFile` is returned.
 *
 * A new path is allocated (path + ".tmp") which is usually small enough to be handled by SSO. Though it *may* trigger
 * a `std::bad_alloc` exception, which is not handled by this function. This is because such event would be fatal and
 * the program should terminate anyway.
 *
 * \param[in] path: Path to the file.
 *
 * \return a `std::error_code` indicating success or failure.
 *
 * \pre     The main file should exist.
 * \note    In that case, in release, the function will fail and the error code will be `noValidFile`.
 * \warning In that case, in debug, the function will assert.
 *
 * \pre     Do not have any streams open on the file or the tmp file.
 * \warning If it needs to delete a file, it will fail, and so will the restoration.
 *
 * \note With no `.tmp`, the function cannot do anything to restore the file.
 *
 * \see `isFileValid()`.
 * \see `FileGuard::Category`.
 */
[[nodiscard]] std::error_code restoreFile(const std::filesystem::path& path) noexcept;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internals for this guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The All Or Nothing guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \brief A simple guideline that prevents data corruption by using transactional, all-or-nothing operations.
 * 
 * Ideal for critical system configurations, save states, or high-priority metadata.
 * 
 * This guideline reads and writes files in their entirety during each call. It guarantees that an operation either
 * completes perfectly or fails entirely, never leaving the file in a partially written or corrupted state.
 * 
 * **Data Integrity & Safety:**
 * This is the most secure guideline available. To guarantee integrity, writing operations actually modify a backup
 * copy instead of the original file. If a crash or power failure occurs during a write, the next call to `.read()`
 * will automatically trigger a recovery mechanism. This mechanism determines whether the original file or the backup
 * copy is the most up-to-date and valid.
 * 
 * **Limitations:**
 * - File Size Constraints: While excellent for small-to-medium files, it is NOT efficient for large files because the
 *   entire content must be re-read and re-written every time. For larger datasets, consider using a different
 *   guideline or breaking your data down into multiple smaller files.
 * - Performance Asymmetry: Writing is significantly slower than reading. Reading is relatively efficient, provided no
 *   crash recovery/restoration is required.
 * - External Modifications: Any external write operations performed on the end of the file will bypass the safety
 *   mechanisms and corrupt the file.
 * 
 * **Security & Encryption:**
 * Encryption is supported natively. When encryption is enabled, the guideline automatically manages and stores the
 * cryptographic nonce immediately before the file's verification/confirmation string.
 * 
 * **Usage & Lifecycle Instructions:**
 * Ease of Use: This guideline is entirely static and does not maintain internal state. Consequently, it requires no
 * instance management, tracking, or explicit closing routines, making it the easiest guideline to integrate.
 * 
 * \note Any external modification to the end of the file will cause data corruption.
 * 
 * \see `restoreFile`.
 */
export struct AllOrNothingGuideline
{
    /**
     * \brief This struct contains data to understand the status of the restoration.
     */
    struct ReadResult
    {
        std::error_code ec{ 0, std::generic_category() }; ///< The actual error code.
        bool isUpToDate{ true };    ///< `true` if the data extracted aren't outdated.

        /**
         * \brief Creates an instance that represents no error.
         * \complexity O(1).
         */
        [[nodiscard]] ReadResult() noexcept = default;

        /**
         * \brief Creates an instance. Sets `isUpToDate` to `true` if code is equal to none/`CleaningFailed` (FileGuard).
         * \complexity O(1).
         *
         * Is not marked `explicit` by design.
         *
         * \param[in] code: see `.ec`.
         */
        [[nodiscard]] ReadResult(std::error_code code) noexcept;
    };


    AllOrNothingGuideline(const AllOrNothingGuideline&) noexcept = delete;
    AllOrNothingGuideline& operator=(const AllOrNothingGuideline&) & noexcept = delete;
    // All special functions are disabled.


    /**
     * \brief Reads data from a file into a string, with optional decryption.
     * \complexity O(N), where N is the size of the file (no encryption).
     * \complexity O(N + encryption).
     *
     * The file is opened in binary mode.
     *
     * This function checks for any bad crash that would have happened during the previous write operation. See the
     * return section.
     * 
     * Fails if the file doesn't exist. Assert in debug. In release the error code is Code::noValidFile.
     *
     * You might receive `std::errc::resource_unavailable_try_again` in release. This happens if the file exists but is
     * not large enough to have the confirmation string placed by the `.write()` function. In debug, this asserts. Keep
     * in mind that you might not receive this error code every time.
     * 
     * `std::errc::not_enough_memory` is returned when there is not enough memory to load or decrypt data.
     * 
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * wrapper-specific exceptions may still throw. You are safe if it doesn't.
     * 
     * \param[in] path:    The path to the file to read from.
     * \param[in] wrapper: The encryption wrapper to use for decryption. No decryption if nullptr.
     *
     * \return Because not all `error_code` are unrecoverable, data may still be returned despite an error code
     *         different than 0. Hence, this function returns a `std::pair`. If the string is empty, an unrecoverable
     *         error happened. If not (string not empty), you should still check if the error code is initialized to
     *         something else than 0 (recoverable error).
     *
     * \pre     You should have called .write() (even empty) at least once on the file before calling this function.
     * \pre     The end of that file should have been left unchanged.
     * \post    The reading will succeed, potentially with data loss depending on previous crashes.
     * \note    If not, the function will fail.
     * \warning In debug, the program asserts if the end of the file is invalid.
     *
     * \pre  The key must have been generated (if encryption). (FileGuard::ErrorCode::emptyKey)
     * \pre  The custom decryption function must succeed.
     * \post The decryption will be performed.
     * \note If not, the function will fail.
     *
     * \pre  The encryption wrapper you use should be the same one you called to write data.
     * \note If not, the data won't be readable in that case.
     *
     * \note Failure and exceptions also depend on the encryption wrapper if provided.
     * \note YOU handle the key (generating and clearing).
     *
     * \see `.write()`.
     * \see `Code`.
     */
    [[nodiscard]] static std::pair<std::string, ReadResult> read(const std::filesystem::path& path, EncryptionWrapper* wrapper = nullptr);

    /**
     * \brief Writes data into a file from a string, with optional encryption.
     * \complexity O(N), where N is the size of the data (no encryption).
     * \complexity O(N + encryption).
     *
     * The file is opened in binary mode.
     *
     * The function edits a copy to ensure that no corruption can happen if there is a crash. There may be some data
     * loss, but no corruption.
     * 
     * If the file does not exist, the function creates it.
     *
     * `std::errc::not_enough_memory` is returned when there is not enough memory to write or encrypt data.
     * 
     * Although not marked as `noexcept`, this function is designed to avoid throwing exceptions. However,
     * wrapper-specific exceptions may still throw. You are safe if it doesn't throw.
     *
     * Always flushes the file. The error code is updated if the flush fails.
     *
     * \param[in] path: The path to the file to write into.
     * \param[in] data: The source of the data.
     * \param[in] wrapper: The encryption wrapper to use for decryption. Data is decrypted if it is
     *					   not nullptr.
     *
     * \return a `std::error_code` indicating success or failure.
     *
     * \pre  The file should exist, be openable and writable for this function.
     * \pre  Should have the permissions to create a new file or empty it (path / ".tmp").
     * \pre  Should have enough file space.
     * \post The writing will succeed.
     * \note If not, the function will fail.
     *
     * \pre  The key must have been generated (if encryption). (FileGuard::ErrorCode::emptyKey)
     * \pre  The custom encryption function must succeed.
     * \post The encryption will be performed.
     * \note If not, the function will fail.
     *
     * \note Failure and exceptions also depends on the encryption wrapper if provided.
     * \note YOU handle the key (generating and clearing).
     *
     * \see `.read()`.
     */
    [[nodiscard]] static std::error_code write(const std::filesystem::path& path, std::string_view data, EncryptionWrapper* wrapper = nullptr);
};

export using AllOrNothing = AllOrNothingGuideline; ///< A shorter identifier for the AllOrNothing guideline.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The All Or Nothing guideline.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // FileGuard namespace.