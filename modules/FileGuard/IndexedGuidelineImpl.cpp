// copyright: Boost Software License - Version 1.0 - August 17th, 2003
// author:    omegaDIL, lesaffreismael@proton.me

module FileGuard;
import :Indexed;

import std;

namespace FileGuard
{

std::expected<std::string, std::error_code> indexedRead(std::fstream& file, const std::uint32_t seekingPos, const std::uint32_t length, const std::uint32_t blocks) noexcept
{
    if (!file.is_open() || file.fail()) [[unlikely]]
        return std::unexpected{ std::make_error_code(std::errc::invalid_argument) };

    std::error_code ec{};

    ec = flushWritingFile(file);
    if (ec) [[unlikely]]
        return std::unexpected{ ec };

    file.seekg(seekingPos * length);
    if (!file) [[unlikely]]
        return std::unexpected{ std::make_error_code(std::errc::resource_unavailable_try_again) };

    const std::uint32_t totalToRead{ blocks * length };
    const std::expected data{ readStream(file, totalToRead) };

    return data;
}

std::error_code indexedWrite(std::fstream& file, const std::string_view data, const std::uint32_t seekingPos, const std::uint32_t length) noexcept
{
    if (!file.is_open() || file.fail()) [[unlikely]]
        return std::make_error_code(std::errc::invalid_argument);

    file.seekp(seekingPos * length); // Errors are checked by overwriteStream(). File holes are authorized.
    return overwriteStream(file, data);
}

} // FileGuard namespace.