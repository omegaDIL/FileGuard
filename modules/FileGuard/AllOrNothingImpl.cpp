// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <cassert>

module FileGuard;
import :AllOrNothing;

import std;

namespace FileGuard
{

// Contains the string that will be written at the end to confirm that the file was saved successfully.
static constexpr std::string_view kStringConfirmation{ "__@'/!__SEcUriTy_BY_FiLegUArd_FiLe_IS_SuCCeSsFULly_SAVed_#[)\"&__" };

bool isFileValid(const std::filesystem::path& path) noexcept
{
    // We don't care about the error code here, we just want to check the validity of the file.
    // If an error occurs, we will consider the file as invalid.

    auto fileStream{ openReadingFile(path, std::ios::binary) };
    if (!fileStream) return false; // Corrupted file if we can't open it.

    std::error_code ec{};
    auto fileSize{ std::filesystem::file_size(path, ec) };
    if (ec || fileSize < kStringConfirmation.size()) [[unlikely]]
        return false; // Fail if error or file is too short.

    constexpr std::streamoff pos{ -static_cast<std::streamoff>(kStringConfirmation.size()) };
    fileStream->seekg(pos, std::ios::end); // Failure will be checked by readingStream.

    const std::expected result{ readStream(*fileStream, static_cast<std::uint32_t>(kStringConfirmation.size())) };

    if (!result) return false;
    return *result == kStringConfirmation;
}

std::error_code restoreFile(const std::filesystem::path& path) noexcept
{
    std::error_code ec{};

    std::filesystem::path tmpPath{ path };
    tmpPath += ".tmp";

    const bool fileExist{ std::filesystem::exists(path, ec) && !ec };
    const bool newerExist{ std::filesystem::exists(tmpPath, ec) && !ec }; // If true, an error happened. 
    // The newer file is the tmp file. tmps are supposed to be renamed.

#ifndef NDEBUG
    assert((fileExist || newerExist) && "The reading operation of AllOrNothing was performed on a file that does not exist");
#endif //NDEBUG

    if (fileExist && !newerExist) [[likely]] // No error.
        return ErrorCode::none; // No loss.

    else if (!fileExist && newerExist)
    { // Main files always exist except when right before renaming the temporary file which is valid.
        std::filesystem::rename(tmpPath, path, ec);

        if (!ec) [[likely]]
            return ErrorCode::cleaningFailed; // Full recovery

        return ec; // Rename operation failed
    }

    // Reaching this point means both files exist -> fileExist && newerExist are true
    // Therefore, we need to both check their validity.
    // In order to do so, we will look at the end of both files for the validity confirmation string.
    // The validity confirmation string is always stored at last, so if it is found, the file is not
    // corrupted and everything before was written correctly.

    else if (isFileValid(tmpPath))
    {
        std::filesystem::remove(path, ec);
        if (ec) [[unlikely]] return ec;

        std::filesystem::rename(tmpPath, path, ec);
        if (ec) [[unlikely]] return ec;

        return ErrorCode::cleaningFailed;
    }

    else if (isFileValid(path)) [[likely]]
    {
        std::filesystem::remove(tmpPath);
        return ErrorCode::invalidNewer;
    }

    return ErrorCode::noValidFile;
}


AllOrNothingGuideline::ReadResult::ReadResult(const std::error_code code) noexcept
    : ec{ code }, isUpToDate{ false }
{
    static constexpr int cleaningFailed{ static_cast<int>(ErrorCode::cleaningFailed) };

    const bool isOfFileGuardCategory{ code.category() == Category::getInstance() };
    const bool isTheSameErrorCode{ code.value() == cleaningFailed };
    const bool wasItRestored{ isOfFileGuardCategory && isTheSameErrorCode };

    if (code.value() == 0 || wasItRestored) isUpToDate = true;
}

std::pair<std::string, AllOrNothingGuideline::ReadResult> AllOrNothingGuideline::read(const std::filesystem::path& path, EncryptionWrapper* wrapper)
{
    const ReadResult fileStatus{ restoreFile(path) };

    if (fileStatus.ec.value() == static_cast<int>(ErrorCode::noValidFile)) [[unlikely]]
        return { {}, fileStatus };

    auto file{ openReadingFile(path, std::ios::binary) }; // The cursor is at the beginning.
    if (!file) [[unlikely]] 
        return { {}, file.error() };

    std::error_code ec{};
    const std::uint32_t fileSize{ static_cast<std::uint32_t>(std::filesystem::file_size(path, ec)) };
    if (ec) [[unlikely]] return { {}, ec };

#ifndef NDEBUG
    assert(fileSize >= kStringConfirmation.size() && "This file is not an AllOrNothing file. It is too short to have the confirmation string.");
    // Since this guideline ensures that there is no corruption unless direct user interaction with files, this check
    // might seem redundant. The file should always contain the string confirmation.
    // However, the user may still pass the wrong file. One way of knowing is checking the size of the file. In debug,
    // we assert to make it clearer.
    // In release, there is no undefined behavior even though we don't assert. The reason is that when we call
    // readStream, the number of character will not be exact (integer overflow). But, since readStream fails in that
    // case (number of character extracted != requested), the function will simply return an error code.
#endif //NDEBUG
    std::expected data{ readStream(*file, fileSize - static_cast<std::uint32_t>(kStringConfirmation.size())) };

    if (!data) [[unlikely]]
        return { {}, data.error() };

    if (wrapper != nullptr) // Decrypting the data if needed.
    {
        std::string decrypted{};
        ec = wrapper->decryptWithNoneAppended(*data, decrypted);
        if (ec) [[unlikely]] // Handles throwing
            return { {}, ec };
        data = std::move(decrypted);
    }

    return { std::move(*data), fileStatus }; // With URVO, this is supposed not to copy.
}

std::error_code AllOrNothingGuideline::write(const std::filesystem::path& path, std::string_view data, EncryptionWrapper* wrapper)
{
    std::string encrypted{}; // Don't let data outlive result.
    if (wrapper != nullptr)
    {
        const auto nonce{ EncryptionWrapper::getCryptoNonce(wrapper->getNonceSize()) };
        if (const std::error_code ec{ wrapper->encryptAndAddNonce(data, nonce, encrypted) }) [[unlikely]]
           return ec;
        data = encrypted;
    }
    
    std::filesystem::path newer{ path.string() };
    newer += ".tmp";

    std::error_code ec{};

    ec = createFile(newer);
    if (ec) [[unlikely]] return ec;

    auto file{ openWritingFile(newer, std::ios::binary | std::ios::trunc) };
    if (!file) [[unlikely]]
        return file.error();

    ec = overwriteStream(*file, data); // file's cursor is shifted.
    if (ec) [[unlikely]] return ec;

    ec = overwriteStream(*file, kStringConfirmation);
    if (ec) [[unlikely]] return ec;

    ec = closeWritingFile(*file); // Will always be reached. No exceptions thrown between the opening operation and now.
    if (ec) [[unlikely]] return ec;

    std::filesystem::remove(path, ec);
    if (ec) [[unlikely]] return ec;

    std::filesystem::rename(newer, path, ec);
    return ec;
}

} // FileGuard namespace.