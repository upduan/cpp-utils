#pragma once

#include <deque> // for std::deque<>
#include <functional> // for std::function<>
#include <string> // for std::string

#include "WebSocket.h" // for util::WebSocket::Message

namespace util {
    class RetrySender {
    public:
        enum class Status { PROGRESS, SUCCESS, FAILURE };

        using UINT_PTR = unsigned long long;

        RetrySender(std::function<void(util::WebSocket::Message&& message)>&& sender, util::WebSocket::Message&& message, std::deque<int>&& timeouts,
            std::function<void()>&& failure) noexcept;

        void setSuccess() noexcept;

        ~RetrySender() noexcept;

        std::function<void(util::WebSocket::Message&& message)> sender() const noexcept {
            return sender_;
        }

        util::WebSocket::Message message() const noexcept {
            return message_;
        }

        std::deque<int> timeouts() noexcept {
            return timeouts_;
        }

        std::function<void()> failure() const noexcept {
            return failure_;
        }

        UINT_PTR timerId() const noexcept {
            return timerId_;
        }

        void timerId(UINT_PTR tid) noexcept {
            timerId_ = tid;
        }

        void setFailure() noexcept {
            status_ = Status::FAILURE;
        }

    private:
        std::function<void(util::WebSocket::Message&& message)> sender_;
        util::WebSocket::Message message_;
        std::deque<int> timeouts_;
        std::function<void()> failure_;
        UINT_PTR timerId_;
        Status status_ = Status::PROGRESS; // atomic this field access
    };
} // namespace util
