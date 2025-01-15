// #include "pch.h"

#include "Socket.h"

// #include <WS2tcpip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <atomic>
#include <thread>

#include "Log.h"

namespace util {
    namespace {
        std::atomic<bool> wsaInited_ = false;
        // WSADATA wsaData;

        void justRead(int workSocket, char* buffer, int length) noexcept {
            int totalLength = 0;
            int remainLength = length;
            while (totalLength < length) {
                int reallyLength = recv(workSocket, buffer + totalLength, remainLength, 0);
                totalLength += reallyLength;
                remainLength -= reallyLength;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    } // namespace

    struct Socket::ContextType {
        std::atomic<bool> isQuit = false;
        int socket = 0;
    };

    Socket::Socket(unsigned short port) : context_(new ContextType()) {
        // if (!wsaInited_) {
        //     WSAStartup(MAKEWORD(2, 2), &wsaData);
        //     wsaInited_ = true;
        // }
        // Log::log("Socket port:%d", port);
        // connect to unity
        context_->socket = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serverAddress {};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port /*25000*/);
        inet_pton(AF_INET, /*ip.c_str()*/ "127.0.0.1", &serverAddress.sin_addr);

    retry:
        int retryCount = 0;
        int r = connect(context_->socket, (sockaddr*)&serverAddress, sizeof(sockaddr_in));
        if (r) {
            retryCount++;
            auto en = errno;
            Log::log("!!!!!\nconnect unity error! error! error!\n %d: %s\n", en, strerror(en));
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (retryCount < 10) {
                goto retry;
            } else {
                throw std::exception();
            }
        } else {
            Log::log("connect to Unity via socket ok! ok! ok!");
        }
        // create receive thread for read unity message and disptach to receiver

        std::thread([this] {
            while (!context_->isQuit) {
                int length = 0;
                justRead(context_->socket, (char*)&length, sizeof(int));
                if (length) {
                    auto m = (char*)std::malloc(length);
                    justRead(context_->socket, (char*)m, length);
                    if (auto r = receiver_.lock(); r) {
                        // Log::log("receive data %p, length is %d", m, length);
                        r->onReceiver(m, length);
                    } else {
                        Log::log("not register receiver");
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }).detach();
        // Log::log("Socket out");
    }

    Socket::~Socket() noexcept {
        context_->isQuit = true;
        shutdown(context_->socket, SHUT_RDWR);
    }

    void Socket::send(/*std::span<std::uint8_t> data*/ char const* data, int length) noexcept {
        std::scoped_lock<std::mutex> lock{mutex_};
        // Log::log("before send data %p, length is %d, type is %d", data, length, *(int*)data);
        ::send(context_->socket, (char const*)&length, sizeof(int), 0);
        ::send(context_->socket, data, length, 0);
        // Log::log("after send data %p, length is %d, type is %d", data, length, *(int*)data);
    }
} // namespace util
