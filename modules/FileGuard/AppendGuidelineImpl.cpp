// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module;

#include <cassert>

module FileGuard;
import :Append;

import std;

#ifndef NDEBUG
#define NOEXCEPTDEBUG noexcept
#else
#define NOEXCEPTDEBUG
#endif  //NDEBUG

namespace FileGuard
{

AppendGuideline::~AppendGuideline() noexcept
{
#ifndef NDEBUG
    assert(!m_file.is_open() && "You forget to close the file of a AppendGuideline instance. Be sure to call either .close() or .readAndClose(). If you called .readAndClose() remember that the closing operation is guaranteed only if the function does not fail.");
#endif  //NDEBUG
}

AppendGuideline::AppendGuideline(const std::filesystem::path& path) NOEXCEPTDEBUG
    : AppendGuideline{}
{
    std::error_code ec{ this->open(path) };

#ifndef NDEBUG
    assert(!ec && "AppendGuideline failed to open the file in the constructor.");
#else   //NDEBUG
    if (ec) [[unlikely]] 
        throw std::ios_base::failure{ ec.message() };
#endif  //NDEBUG
}

AppendGuideline::AppendGuideline(const std::filesystem::path& path, const std::string_view add) NOEXCEPTDEBUG
    : AppendGuideline{ path }
{
    std::error_code ec{ this->append(add) };

#ifndef NDEBUG
    assert(!ec && "AppendGuideline failed to append data to the file in the constructor.");
#else   //NDEBUG
    if (ec) [[unlikely]] 
        throw std::ios_base::failure{ ec.message() };
#endif  //NDEBUG
}


std::error_code AppendGuideline::open(const std::filesystem::path& path) noexcept
{
    if (m_file.is_open())
    {   // Closes the file if one.
        std::error_code ec{};
        ec = this->close();
        if (ec) [[unlikely]] return ec;
    }

    auto result{ openIOFile(path, std::ios::binary | std::ios::app) };
    if (result) [[likely]]
        m_file = std::move(*result);

    return result.error_or({});
}

std::error_code AppendGuideline::openAndAppend(const std::filesystem::path& path, const std::string_view data) noexcept
{
    if (const std::error_code ec{ this->open(path) })
        [[unlikely]] return ec;

    return this->append(data);
}

std::error_code AppendGuideline::append(const std::string_view data) noexcept
{
    if (!m_file.is_open()) [[unlikely]]
        return std::make_error_code(std::errc::invalid_argument);

    return overwriteStream(m_file, data);
}

std::expected<std::string, std::error_code> AppendGuideline::readAndClose(const std::uint32_t charactersToExtract) noexcept
{
    if (!m_file.is_open()) [[unlikely]]
        return std::unexpected{ std::make_error_code(std::errc::invalid_argument) };

    std::error_code ec{};
    ec = this->ensureFlushed();
    if (ec) [[unlikely]] 
        return std::unexpected{ ec };

    m_file.seekg(-static_cast<std::streampos>(charactersToExtract), std::ios::end);
    if (!m_file) [[unlikely]]
    {
        m_file.clear();
        return std::unexpected{ std::make_error_code(std::errc::resource_unavailable_try_again) };
    }

    const auto result{ readStream(m_file, charactersToExtract) };
    if (!result) [[unlikely]]
    {
        m_file.clear();
        return result;
    }

    ec = closeWritingFile(m_file);
    if (ec) [[unlikely]] 
        return std::unexpected{ ec };

    return result;
}

std::expected<std::string, std::error_code> AppendGuideline::readAndRestore(const std::uint32_t charactersToExtract) noexcept
{
    if (!m_file.is_open()) [[unlikely]]
        return std::unexpected{ std::make_error_code(std::errc::invalid_argument) };

    std::error_code ec{};
    ec = this->ensureFlushed();
    if (ec) [[unlikely]] return std::unexpected{ ec };

    m_file.seekg(-static_cast<std::streampos>(charactersToExtract), std::ios::end);
    if (!m_file) [[unlikely]]
    {
        m_file.clear();
        return std::unexpected{ std::make_error_code(std::errc::resource_unavailable_try_again) };
    }

    const auto result{ readStream(m_file, charactersToExtract) };

    m_file.clear(); // Seeking is automatically handled because the stream was opened in append mode.

    return result;
}

std::error_code AppendGuideline::ensureFlushed() noexcept
{
    return flushWritingFile(m_file);
}

std::error_code AppendGuideline::close(const std::error_code ec) noexcept
{
    const std::error_code closeEc{ closeWritingFile(m_file) };

    // The error code passed as argument is prioritized because it is the first one that happened.
    return (ec) ? ec : closeEc;
}

} // FileGuard namespace.