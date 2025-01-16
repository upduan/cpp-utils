#pragma once

#include <string>

namespace utils::base64 {
    std::string const encode(std::string_view data);

    std::string const decode(std::string_view data);
}

