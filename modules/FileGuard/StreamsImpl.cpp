// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <cerrno>

module FileGuard;
import :Streams;

import std;

namespace FileGuard
{

const Category& Category::getInstance() noexcept
{
    static Category instance{};
    return instance;
}

const char* Category::name() const noexcept
{
    return "FileGuard error";
}

std::string Category::message(int ev) const
{
    switch (static_cast<ErrorCode>(ev))
    {
        case ErrorCode::none:
            return "No abnormal crash, data loss or encryption failure detected.";
        case ErrorCode::unknown:
            return "Something bad happened, but FileGuard can't tell you what went wrong.";

        case ErrorCode::emptyKey:
            return "The key passed to an encryption wrapper instance is empty.";
        case ErrorCode::emptyNonce:
            return "The nonce passed to an encryption wrapper instance is empty.";
        case ErrorCode::invalidNonce:
            return "The nonce passed to an encryption wrapper instance is invalid.";

        case ErrorCode::cleaningFailed:
            return "An abnormal crash was detected during the cleaning of a AllOrNothing guideline writing operation. The recovery was successful without data loss.";
        case ErrorCode::invalidNewer:
            return "An abnormal crash was detected during a writing operation of the AllOrNothing guideline. The data are lost, however, an older save was restored.";
        case ErrorCode::noValidFile:
            return "An unknown error was detected by the AllOrNothing guideline. The data don't exist or are corrupted. The suspected cause is a bad user interaction with internal files.";

        default:
            std::unreachable();
   }
}


std::expected<std::ifstream, std::error_code> openReadingFile(const std::filesystem::path& path, const std::ios::openmode mode) noexcept
{
    errno = 0;
    std::error_code ec{};

    std::ifstream stream{};

    try
    {
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.open(path, std::ios::in | mode);
        stream.exceptions(std::ios_base::goodbit);
        return std::move(stream); // Exit early.
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }
    }
    catch (const std::bad_alloc&)
    {
        ec = std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
    }

    // An error happened.
    return std::unexpected{ ec };
}

std::expected<std::ofstream, std::error_code> openWritingFile(const std::filesystem::path& path, const std::ios::openmode mode) noexcept
{
    errno = 0;
    std::error_code ec{};

    if (!std::filesystem::exists(path, ec)) [[unlikely]]
        return std::unexpected{ ec ? ec
        : std::make_error_code(std::errc::no_such_file_or_directory) };

    std::ofstream stream{};

    try
    {
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.open(path, std::ios::in | std::ios::out | mode);
        stream.exceptions(std::ios_base::goodbit);
        return std::move(stream); // Exit early.
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }
    }
    catch (const std::bad_alloc&)
    {
        ec = std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
    }

    // An error happened.
    return std::unexpected{ ec };
}

std::expected<std::fstream, std::error_code> openIOFile(const std::filesystem::path& path, const std::ios::openmode mode) noexcept
{
    errno = 0;
    std::error_code ec{};

    if (!std::filesystem::exists(path, ec)) [[unlikely]]
        return std::unexpected{ ec ? ec
        : std::make_error_code(std::errc::no_such_file_or_directory) };

    std::fstream stream{};

    try
    {
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.open(path, std::ios::in | std::ios::out | mode);
        stream.exceptions(std::ios_base::goodbit);
        return std::move(stream); // Exit early.
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }
    }
    catch (const std::bad_alloc&)
    {
        ec = std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
    }

    // An error happened.
    return std::unexpected{ ec };
}


std::expected<std::string, std::error_code> readStream(std::istream& stream, const std::uint32_t requestedAmount) noexcept
{
    if (stream.fail()) [[unlikely]]
        return std::unexpected{ std::make_error_code(std::errc::invalid_argument) };

    errno = 0;
    std::error_code ec{};

    const auto oldMask{ stream.exceptions() };

    try
    {
        stream.exceptions(std::ios::badbit);

        std::string output(requestedAmount, '0');
        stream.read(output.data(), requestedAmount);

        stream.exceptions(oldMask);

        const std::streamsize gcount{ stream.gcount() }; // Get the actual amount of data extracted.
        if (gcount != static_cast<std::streamsize>(requestedAmount)) [[unlikely]] 
            return std::unexpected<std::error_code>{ std::make_error_code(std::errc::resource_unavailable_try_again) };

        return output; // Exit early.
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }
    }
    catch (const std::bad_alloc&)
    {
        ec = std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
    }

    // An error happened.
    stream.clear();
    stream.exceptions(oldMask);
    return std::unexpected{ ec };
}

std::error_code overwriteStream(std::ostream& stream, const std::string_view data, const std::uint32_t requestedAmount) noexcept
{
    if (stream.fail()) [[unlikely]]
        return std::make_error_code(std::errc::invalid_argument);

    errno = 0;
    std::error_code ec{};

    const auto oldMask{ stream.exceptions() };

    try
    {
        stream.exceptions(std::ios::failbit | std::ios::badbit);

        const std::streamsize dataSize{ static_cast<std::streamsize>(data.size()) };
        const std::streamsize requestedCasted{ static_cast<std::streamsize>(requestedAmount) };
        const auto toWrite{ std::min(dataSize, requestedCasted) };
        stream.write(data.data(), toWrite);

        // Mask is restored at the end
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }

        stream.clear();
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
        stream.clear();
    }

    stream.exceptions(oldMask);
    return ec;
}

std::error_code flushWritingFile(std::fstream& stream) noexcept
{
    if (!stream.is_open()) [[unlikely]] return {};

    errno = 0;
    std::error_code ec{};
    const auto oldMask{ stream.exceptions() };

    try
    {
        stream.clear(); // Do not prevent flushing data if an error occurred before calling this function.
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.flush();
        // Mask is restored at the end
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }

        stream.clear();
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
        stream.clear();
    }

    stream.exceptions(oldMask);
    return ec;
}

std::error_code flushWritingFile(std::ofstream& stream) noexcept
{
    if (!stream.is_open()) [[unlikely]] return {};

    errno = 0;
    std::error_code ec{};
    const auto oldMask{ stream.exceptions() };

    try
    {
        stream.clear(); // Do not prevent flushing data if an error occurred before calling this function.
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.flush();
        // The mask is restored at the end.
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }

        stream.clear();
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
        stream.clear();
    }

    stream.exceptions(oldMask);
    return ec;
}

std::error_code closeWritingFile(std::fstream& stream) noexcept
{
    if (!stream.is_open()) [[unlikely]] return {};
    
    errno = 0;
    std::error_code ec{}; 
    const auto oldMask{ stream.exceptions() };

    try 
    {
        stream.clear(); // Do not prevent flushing data if an error occurred before calling this function.
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.close();
        // The mask is restored at the end.
    }
    catch (const std::ios_base::failure& e) 
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]] 
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }

        stream.clear();
    }
    catch (...) 
    {
        ec = std::make_error_code(std::errc::io_error);
        stream.clear();
    }

    stream.exceptions(oldMask);
    return ec;
}

std::error_code closeWritingFile(std::ofstream& stream) noexcept
{
    if (!stream.is_open()) [[unlikely]] return {};

    errno = 0;
    std::error_code ec{};
    const auto oldMask{ stream.exceptions() };

    try
    {
        stream.clear(); // Do not prevent flushing data if an error occurred before calling this function.
        stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        stream.close();
        // The mask is restored at the end.
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }

        stream.clear();
    }
    catch (...)
    {
        ec = std::error_code{ EIO, std::generic_category() };
        stream.clear();
    }

    stream.exceptions(oldMask);
    return ec;
}


std::error_code createFile(const std::filesystem::path& path) noexcept
{
    std::error_code ec{};
    errno = 0;

    if (std::filesystem::exists(path, ec) || ec) [[unlikely]]
        return ec; // Success. No error (see doc)

    try
    {
        std::ofstream stream{};

        stream.exceptions(std::ios::badbit | std::ios::failbit);
        stream.open(path, std::ios::app);
    }
    catch (const std::ios_base::failure& e)
    {
        ec = e.code();

        if (ec == std::io_errc::stream || ec.value() == 0) [[unlikely]]
        {   // Unknown error.
            const int err{ (errno) ? errno : EIO };
            ec = std::error_code{ err, std::generic_category() };
        }
    }
    catch (const std::bad_alloc&)
    {
        ec = std::make_error_code(std::errc::not_enough_memory);
    }
    catch (...)
    {
        ec = std::make_error_code(std::errc::io_error);
    }

    return ec;
}

std::error_code basicFileZeroing(const std::filesystem::path& path) noexcept
{
    std::error_code ec{};

    // Checks the size of the file.
    std::uintmax_t fileSize{ std::filesystem::file_size(path, ec) };
    if (ec) [[unlikely]] return ec;

    // Opens the file.
    auto file{ openWritingFile(path, std::ios::binary) };
    if (!file) [[unlikely]] 
        return file.error();

    // Creates a buffer.
    constexpr std::size_t bufferSize{ 4 * 1024 }; // 4 KB.
    static constexpr std::array<char, bufferSize> zeroBuffer{};
    static constexpr std::string_view zeroView(zeroBuffer.data(), zeroBuffer.size());

    while (fileSize > bufferSize)
    {
        fileSize -= bufferSize;
        ec = overwriteStream(*file, zeroView);
        if (ec) return ec;
    }

    // FileSize is inferior to bufferSize. We can write the remaining bytes. Hence, conversion is safe because fileSize
    // is inferior to 65536.
    ec = overwriteStream(*file, zeroView, static_cast<std::uint32_t>(fileSize));
    if (ec) return ec;

    return closeWritingFile(*file); // Will always be reached. No exceptions thrown earlier.
}

} // FileGuard namespace.