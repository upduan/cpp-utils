#include "socket-client.h"

#include <thread>

#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "Log.h"

namespace util {
    namespace {
        ssize_t just_read(int workSocket, unsigned char* buffer, int length, int seconds) {
            ssize_t totalLength = 0;
            fd_set reads;
            FD_ZERO(&reads);
            FD_SET(workSocket, &reads);
            timeval timeout;
            timeout.tv_sec = seconds;
            timeout.tv_usec = 0;
            auto r = select(workSocket + 1, &reads, nullptr, nullptr, &timeout);
            if (r == 0) {
                // timeout
            } else if (r < 0) {
                // error
                totalLength = -1;
                // throw check_error(r, "select");
            } else {
                ssize_t remainLength = length;
                while (totalLength < length) {
                    ssize_t reallyLength = recv(workSocket, buffer + totalLength, remainLength, 0);
                    // for (ssize_t index = totalLength; index < totalLength + reallyLength; ++index) {
                    //     std::printf("%02X ", buffer[index]);
                    // }
                    if (reallyLength == -1) {
                        totalLength = -1;
                        break;
                        // throw check_error(reallyLength, "recv");
                    }
                    totalLength += reallyLength;
                    remainLength -= reallyLength;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                // std::printf("\n");
            }
            return totalLength;
        }
    } // namespace

    socket_exception check_error(int r, std::string const& act) noexcept {
        socket_exception result(0, SocketError::Ok, "OK", act);
        // SocketError result = SocketError::Ok;
        if (r == -1) {
            auto error_no = errno;
            result.error_no = error_no;
            // 根据 errno 判断连接是否断开
            if (error_no == ECONNRESET) {
                log_error << "Connection reset by peer.";
                result.error = SocketError::Final; // 连接被重置
                result.description = "Connection reset by peer.";
            } else if (error_no == EPIPE) {
                log_error << "Broken pipe: connection closed.";
                result.error = SocketError::Final; // 连接已关闭
                result.description = "Broken pipe: connection closed.";
            } else if (error_no == EAGAIN || error_no == EWOULDBLOCK) {
                log_info << "Non-blocking socket, would block.";
                result.error = SocketError::Temporary; // 非阻塞，稍后再试
                result.description = "Non-blocking socket, would block.";
            } else {
                log_error << "failed: " << strerror(error_no);
                result.error = SocketError::Temporary; // 非阻塞，稍后再试
                result.description = strerror(error_no);
            }
        }
        return result;
    }

    ssize_t send_packet(int sock, package const& pack) {
        int result = 0;
        auto head_length = ::send(sock, pack.head().data(), package::head_size(), 0);
        log_debug << "send_packet head_length:" << head_length;
        if (head_length == package::head_size()) {
            auto data_length = ::send(sock, pack.data.data(), pack.data.length(), 0);
            log_debug << "send_packet data_length:" << data_length;
            if (data_length == pack.data.length()) {
                result = head_length + data_length;
            } else {
                throw check_error(head_length, "send data");
            }
        } else if (head_length == -1) {
            throw check_error(head_length, "send header");
        }
        log_debug << "send_packet result:" << result;
        return result;
    }

    ssize_t send_data(int sock, int type, iovec const data[], std::size_t count) {
        static package header{};
        iovec* all_data = (iovec*)std::malloc((count + 1) * sizeof(iovec));
        all_data[0].iov_base = &header;
        all_data[0].iov_len = 12;
        int length = 0;
        for (int i = 0; i < count; ++i) {
            length += data[i].iov_len;
            all_data[i + 1] = data[i];
        }
        header.length = 8 + length;
        header.version = 1; // version
        header.type = type;
        int r = writev(sock, data, count);
        auto cr = check_error(r, "writev");
        if (cr.error != SocketError::Ok) {
            throw cr;
        }
        return r;
    }

    package receive_packet(int sock, int seconds) {
        package result;
        auto receive_size = just_read(sock, (unsigned char*)&result, package::head_size(), seconds);
        if (receive_size == package::head_size()) {
            if (auto data_length = result.length - 8; data_length > 0) {
                auto data = (unsigned char*)std::malloc(data_length);
                receive_size = just_read(sock, data, data_length, 2);
                if (receive_size == data_length) {
                    result.data = std::string_view((char*)data, data_length);
                    result.need_free = true;
                    log_debug << "receive_packet ok: " << sock;
                } else {
                    log_error << "receive_packet network transfer error 1";
                    throw check_error(receive_size, "receive body");
                }
            } else {
                log_error << "receive_packet header transfer error, maybe missing data boundary";
                throw check_error(receive_size, "header content check");
            }
        } else {
            log_error << "receive_packet header transfer error, not get full header";
            throw check_error(receive_size, "receive header");
        }
        return result;
        // throw std::system_error(std::make_error_code(std::io_errc::stream), "network transfer error");
    }
} // namespace util
