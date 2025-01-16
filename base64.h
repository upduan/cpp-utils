#pragma once

#include <string>
#include <vector>

namespace util::base64 {
    std::string encode(std::string_view data) noexcept;

    std::vector<std::uint8_t> decode(std::string_view data) noexcept;
}

