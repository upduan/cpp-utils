#pragma once

#include <memory> // for std::weak_ptr<>
#include <mutex> // for std::mutex

namespace util {
    class Socket {
    public:
        class Receiver {
        public:
            virtual void onReceiver(/*std::span<std::uint8_t> data*/ char const* data, int length) noexcept = 0;
        };

        Socket(unsigned short port);

        ~Socket() noexcept;

        void send(/*std::span<std::uint8_t> data*/ char const* data, int length) noexcept;

        void registerReceiver(std::weak_ptr<Receiver> receiver) noexcept {
            receiver_ = receiver;
        }

    private:
        std::weak_ptr<Receiver> receiver_;
        struct ContextType;
        ContextType* context_ = nullptr;
        std::mutex mutex_;
    };
} // namespace util
