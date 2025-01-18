#pragma once

#include <any>
#include <functional>
#include <string>

namespace util::FileTransfer {
    void initialize() noexcept;
    void transfer_files(std::any context, std::string const& folder, std::function<void(std::string const& file_name, int code, std::string const& message)>&& notify) noexcept;
    void finalize() noexcept;
}
