#include "connection-pool.h"

// #include <atomic>
// #include <functional>
// #include <iostream>
// #include <memory>
// #include <thread>

void connection_pool_test() {
    util::ConnectionPool pool("127.0.0.1", 2222, 10, 100);
    auto sockfd = pool.GetObject(); // get the socket, value wrap by std::optional
    if (sockfd.has_value()) {
        log_debug << "get connection from pool";
        // Do something with the socket
        pool.RecycleObject(sockfd.value()); // release the socket
        log_debug << "recycled connection to pool";
    }
}

#include <functional>
#include <string>
#include <string_view>

#include "Log.h"

namespace util {
    // 1. TransferStart[or All]: Robot -- Total-Length, Version, Type, FileMetaInfo[, content(when Type is All)] --> PC
    // 2. Ack: PC -- Total-Length, Version, Type, TransferId --> Robot
    // 3. Chunk[repeat]: Robot -- Total-Length, Version, Type, TransferId, Offset, Length, Data --> PC
    // -- 4. MD5: Robot -- Total-Length, Version, Type, TransferId, MD5 --> PC
    // 5. Ack: PC -- Total-Length, Version, Type[, TransferId(when Type is not All)] --> Robot
    // -- 6. NAck: PC -- Total-Length, Version, Type[, TransferId(when Type is not All)] --> Robot

    enum class FileTransferType : int { TransferStart = 0xFFFF01, Chunk, All, Ack, MD5, NAck };

    enum class FileType : std::uint32_t { TotalJson = 6, TaskJson, Video, Timestamp = 10, Pose };

    struct FileMetaInfo {
        static std::size_t fill_buffer(char* buffer, FileMetaInfo const& that) noexcept {
            std::size_t offset = 0;
            log_info << "that.file_type " << (std::uint32_t)that.file_type;
            *(std::uint32_t*)(buffer + offset) = (std::uint32_t)that.file_type;
            offset += sizeof(std::uint32_t);
            *(int*)(buffer + offset) = that.big_task_id.length();
            offset += sizeof(int);
            std::memcpy(buffer + offset, that.big_task_id.data(), that.big_task_id.length());
            offset += that.big_task_id.length();
            *(int*)(buffer + offset) = that.task_id.length();
            offset += sizeof(int);
            std::memcpy(buffer + offset, that.task_id.data(), that.task_id.length());
            offset += that.task_id.length();
            *(int*)(buffer + offset) = that.camera.length();
            offset += sizeof(int);
            std::memcpy(buffer + offset, that.camera.data(), that.camera.length());
            offset += that.camera.length();
            *(int*)(buffer + offset) = that.file_size;
            offset += sizeof(int);
            return offset;
        }

        FileMetaInfo(FileType ft, std::string const& btid, std::string const& tid, std::string const& c, int fs) noexcept
            : file_type{ft}, big_task_id{btid}, task_id{tid}, camera{c}, file_size{fs} {}

        FileType file_type;
        std::string big_task_id;
        std::string task_id;
        std::string camera;
        int file_size;

        std::size_t size() const noexcept {
            return 4 + 4 + big_task_id.length() + 4 + task_id.length() + 4 + camera.length() + 4;
        }

        std::string_view serialize() const noexcept {
            buffer_ = (char*)std::malloc(size());
            std::size_t offset = fill_buffer(buffer_, *this);
            return std::string_view(buffer_, offset);
        }

        virtual ~FileMetaInfo() noexcept {
            if (buffer_) {
                log_trace << "~FileMetaInfo()";
                std::free(buffer_);
            }
        }

    protected:
        mutable char* buffer_ = nullptr;
    };

    struct FileMetaInfoWithContent : FileMetaInfo {
        FileMetaInfoWithContent(FileType ft, std::string const& btid, std::string const& tid, std::string const& c, int fs, std::string_view ctt) noexcept
            : FileMetaInfo(ft, btid, tid, c, fs), content{ctt} {}

        std::string_view content;

        std::size_t size() const noexcept {
            return FileMetaInfo::size() + content.length();
        }

        std::string_view serialize() const noexcept {
            log_trace << "serialize size:" << size();
            buffer_ = (char*)std::malloc(size());
            std::size_t offset = FileMetaInfo::fill_buffer(buffer_, *this);
            //  log_info << "content:" << content;
            std::memcpy(buffer_ + offset, content.data(), content.length());
            offset += content.length();
            log_trace << "FileMetaInfoWithContent serialize";
            return std::string_view(buffer_, offset);
        }

        // ~FileMetaInfoWithContent() noexcept {
        //     if (!content.empty()) {
        //         std::free((void*)content.data());
        //     }
        // }
    };

    struct ChunkHead {
        std::uint64_t transfer_id;
        int offset;
        int length;
        // char const* data;
        static constexpr std::size_t size() noexcept {
            return 8 + 4 + 4;
        }
    };

    void send_file(std::string const& filename, FileMetaInfo& data) noexcept;

    void send_data(std::string const& filename, FileMetaInfoWithContent& data) noexcept;

    int send_data(FileMetaInfoWithContent const& data) noexcept;
} // namespace util

#include "file-transfer.h"

#include <filesystem>
#include <fstream>

#include "connection-pool.h"
#include "socket-client.h"
#include "thread-pool.h"

namespace util {
    namespace {
        constexpr std::size_t BUFFER_SIZE = 1024 * 100;

        std::shared_ptr<ConnectionPool> connection_pool = nullptr;

        int chunk_send_file(const int sock, std::string const& filename, std::string_view file_meta_info, std::size_t fileSize,
            std::function<int(const int sock, std::uint64_t transfer_id, int offset, char const* buffer, int length)>&& sender) noexcept {
            int result = 0;
            std::ifstream file;
            file.open(filename, std::ios::binary);
            if (file.is_open()) {
                auto pack = util::package{1, int(FileTransferType::TransferStart), file_meta_info};
                auto send_length = util::send_packet(sock, pack);
                if (send_length == pack.size()) {
                    std::uint64_t transfer_id = 0;
                    try {
                        auto p = util::receive_packet(sock); // FileTransferType::TransferId
                        transfer_id = *(std::uint64_t*)p.data.data();
                        log_trace << "sock: " << sock << ", transfer_id: " << transfer_id << ", filename: " << filename;
                        // util::md5 digest;
                        char buffer[BUFFER_SIZE];
                        std::size_t offset = 0;
                        if (fileSize) {
                            while (!file.eof()) {
                                auto chunk_length = std::min(BUFFER_SIZE, fileSize - offset);
                                file.read(buffer, chunk_length);
                                // std::string_view data(buffer, chunk_length);
                                // digest.update(data.begin(), data.end());
                                auto read_length = file.gcount();
                                if (read_length) {
                                    if (read_length == chunk_length) {
                                        auto send_length = sender(sock, transfer_id, offset, buffer, read_length);
                                        if (send_length == read_length) {
                                            offset += read_length;
                                        } else {
                                            log_info << "send_length != read_length. send_length: " << send_length << ", read_length: " << read_length;
                                            result = -2;
                                            break;
                                        }
                                    } else {
                                        log_info << "read_length != chunk_length. send_length: " << send_length << ", chunk_length: " << chunk_length;
                                        result = -2;
                                        break;
                                    }
                                } else {
                                    log_trace << "sock: " << sock << ", transfer_id: " << transfer_id << ", filename: " << filename << " finish";
                                    break;
                                }
                            }
                        }
                        if (result == 0 && fileSize) {
                            // auto d = digest.hex_digest<std::string>();
                            // receive ack
                            try {
                                log_debug << "sock: " << sock << ", wait for receive_packet begin";
                                auto ack = util::receive_packet(sock); // FileTransferType::Ack
                                log_debug << "sock: " << sock << ", wait for receive_packet end";
                                // transfer_id = *(std::uint64_t*)ack.data.data();
                            } catch (std::system_error& e) {
                                log_error << "sock: " << sock << ", socket reset: " << e.what();
                                result = -2;
                            }
                        }
                    } catch (std::system_error& e) {
                        log_error << "sock: " << sock << ", socket reset: " << e.what();
                        result = -2;
                    }
                } else {
                    log_error << "send package fail";
                    result = -2;
                }
                file.close();
            } else {
                log_error << "file open fail: " << filename;
                result = -1;
            }
            return result;
        }
    } // namespace

    void send_file(std::string const& filename, FileMetaInfo& meta_info) noexcept {
        if (!connection_pool) {
            connection_pool = std::make_shared<ConnectionPool>(10, 100);
        }
        auto socket = connection_pool->GetObject();
        if (socket.has_value()) {
            ThreadPoolManager::submit_task(
                5 * 60 * 1000, [] {},
                [](int sockfd, std::string const& filename, FileMetaInfo& meta_info) {
                    log_info << "socket: " << sockfd << ", retry_send_file: " << filename;
                    for (int count = 0; count < 3; ++count) {
                        if (std::filesystem::exists(filename)) {
                            // result = send_file_(sockfd, filename, type);
                            meta_info.file_size = (int)std::filesystem::file_size(filename);
                            log_info << "socket: " << sockfd << ", filename: " << filename << ", fileSize: " << meta_info.file_size;
                            int result = chunk_send_file(sockfd, filename, meta_info.serialize(), meta_info.file_size,
                                [&filename, &meta_info](const int sock, std::uint64_t transfer_id, int offset, char const* buffer, int length) {
                                    int result = -2;
                                    iovec data[2];
                                    auto chunk = ChunkHead{transfer_id, offset, length /*, buffer*/};
                                    data[0].iov_base = &chunk;
                                    data[0].iov_len = ChunkHead::size();
                                    data[1].iov_base = (void*)buffer;
                                    data[1].iov_len = length;
                                    auto r = util::send_data(sock, int(FileTransferType::Chunk), data, 2);
                                    {
                                        log_error << "send chunk body error send_length != length. " << filename << ", send_length: " << send_length << ", length: " << length;
                                        result = -2;
                                        log_error << "send chunk head error " << filename << ", send_length: " << send_length << ", sizeof(ChunkHead): " << sizeof(ChunkHead);
                                        result = -2;
                                        log_error << "send package head error " << filename << ", send_length: " << send_length
                                                  << ", util::package::head_size(): " << util::package::head_size();
                                        result = -2;
                                    }
                                    return result;
                                });
                            if (result == 0) {
                                // std::filesystem::remove(filename);
                                break;
                            } else if (result == -2) {
                                auto r = connection_pool->Reset(sockfd);
                                if (r.has_value()) {
                                    sockfd = r.value();
                                } else {
                                    connection_pool->ForceReset();
                                }
                            }
                        } else {
                            log_info << "socket: " << sockfd << ", file: " << filename << " not exist, type: " << type;
                            break;
                        }
                    }
                },
                socket.value(), filename, meta_info);
        }
    }

    void send_data(std::string const& filename, FileMetaInfoWithContent& data) noexcept {
        data.file_size = (int)std::filesystem::file_size(filename);
        char* content = nullptr;
        if (data.file_size) {
            content = (char*)std::malloc(data.file_size);
            auto file = std::fopen(filename.c_str(), "rb");
            auto len = std::fread(content, 1, data.file_size, file);
            std::fclose(file);
            data.content = std::string_view(content, len);
            send_data(data);
            // std::filesystem::remove(filename);
            std::free(content);
        } else {
            log_error << "send whole file fileSize == 0";
        }
    }

    int send_data(FileMetaInfoWithContent const& data) noexcept {
        int result = -2;
        if (!connection_pool) {
            connection_pool = std::make_shared<ConnectionPool>(10, 100);
        }
        auto socket = connection_pool->GetObject();
        if (socket.has_value()) {
            auto sock = socket.value();
            ThreadPoolManager::submit_task(
                5 * 60 * 1000, [] {},
                [&result, &sock, &data] {
                    log_info << "socket: " << sock << ", send whole file filesize: " << data.file_size;
                    for (int count = 0; count < 3; ++count) {
                        int r = util::send_packet(sock, util::package{1, int(FileTransferType::All), data.serialize()});
                        if (r) {
                            try {
                                auto p = util::receive_packet(sock); // FileTransferType::TransferId
                                result = 0;
                            } catch (const std::system_error& e) {
                                log_error << "socket: " << sock << ", send whole file network transfer error";
                                result = -2;
                            }
                        } else {
                            result = -2;
                        }
                        if (result == 0) {
                            break;
                        } else if (result == -2) {
                            auto r = connection_pool->Reset(sock);
                            if (r.has_value()) {
                                sock = r.value();
                            } else {
                                connection_pool->ForceReset();
                            }
                        }
                    }
                });
            return result;
        }
    }
} // namespace util
