#pragma once

#include <cstdlib>
#include <string>
#include <string_view>

#include <sys/uio.h>

namespace util {
    struct package {
        static constexpr std::size_t head_size() noexcept {
            return 12; // len + version + type
        }
        mutable int length{};
        int version{1};
        int type{};
        std::string_view data;
        bool need_free{false};
        // std::size_t size() const noexcept {
        //     return head_size() + data.length();
        // }
        std::string_view head() const noexcept {
            length = 8 + data.length();
            return std::string_view((char*)this, 12);
        }
        ~package() noexcept {
            if (need_free) {
                std::free((void*)data.data());
            }
        }
    };

    enum class SocketError : ssize_t { Ok, Temporary = -1, Final = -2 };

    struct socket_exception : public std::exception {
        socket_exception(int errno, SocketError err, std::string const& desc, std::string const& act) : std::exception{}, error_no{errno}, error{err}, description{desc}, action{act} {}

        char const* what() const noexcept override {
            switch (error) {
            case SocketError::Ok:
                return (action + ": no error").c_str();
            case SocketError::Temporary:
                return (action + "temporary error").c_str();
            case SocketError::Final:
                return (action + "final error").c_str();
            default:
                return (action + "unknown error").c_str();
            }
        }

        int error_no;
        SocketError error;
        std::string description;
        std::string action;
    };

    socket_exception check_error(int r, std::string const& act) noexcept;

    ssize_t send_packet(int sock, package const& data);
    int send_with_check(int socket_fd, const void* buf, size_t len, int flags);
    ssize_t send_data(int sock, int type, iovec const data[], std::size_t count);
    package receive_packet(int sock, int seconds);
} // namespace util
