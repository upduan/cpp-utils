#include "file-transfer.h"

#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include <sys/socket.h>

#include "Log.h"
#include "connection-pool.h"
#include "md5.h"
#include "socket-client.h"
#include "thread-pool.h"

namespace util::FileTransfer {
    namespace {
        std::shared_ptr<util::ThreadPool> t_pool = nullptr;
        std::shared_ptr<util::ConnectionPool> c_pool = nullptr;
        std::any context_;

        // 1. TransferStart[or All]: Robot -- Total-Length, Version, Type, FileMetaInfo[, content(when Type is All)] --> PC
        // 2. Ack: PC -- Total-Length, Version, Type, TransferId --> Robot
        // 3. Chunk[repeat]: Robot -- Total-Length, Version, Type, TransferId, Offset, Length, Data --> PC
        // -- 4. MD5: Robot -- Total-Length, Version, Type, TransferId, MD5 --> PC
        // 5. Ack: PC -- Total-Length, Version, Type[, TransferId(when Type is not All)] --> Robot
        // -- 6. NAck: PC -- Total-Length, Version, Type[, TransferId(when Type is not All)] --> Robot

        enum class FileTransferType : int { TransferStart = 0xFFFF01, Chunk, All, Ack, MD5, NAck };

        enum class FileType : std::uint32_t { TotalJson = 6, TaskJson, Video, Jpeg, Timestamp = 10, Pose };

        // FIXME move to file transfer client
        struct Context {
            std::string big_task_id;
            std::string task_id;
        };

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
                *(int*)(buffer + offset) = that.filename.length();
                offset += sizeof(int);
                std::memcpy(buffer + offset, that.filename.data(), that.filename.length());
                offset += that.filename.length();
                return offset;
            }

            FileMetaInfo(FileType ft, std::string const& btid, std::string const& tid, std::string const& c, int fs, std::string const& fn) noexcept
                : file_type{ft}, big_task_id{btid}, task_id{tid}, camera{c}, file_size{fs}, filename{fn} {}

            FileType file_type;
            std::string big_task_id;
            std::string task_id;
            std::string camera;
            int file_size;
            std::string filename;

            std::size_t size() const noexcept {
                return 4 + 4 + big_task_id.length() + 4 + task_id.length() + 4 + camera.length() + 4 + 4 + filename.length();
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
            FileMetaInfoWithContent(FileType ft, std::string const& btid, std::string const& tid, std::string const& c, int fs, std::string const& fn,
                std::string_view ctt) noexcept
                : FileMetaInfo(ft, btid, tid, c, fs, fn), content{ctt} {}

            std::string_view content;

            std::size_t size() const noexcept {
                return FileMetaInfo::size() + content.length();
            }

            std::string_view serialize() const noexcept {
                log_trace << "serialize size:" << size();
                buffer_ = (char*)std::malloc(size());
                std::size_t offset = FileMetaInfo::fill_buffer(buffer_, *this);
                log_info << "content:" << content.length();
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

        constexpr std::size_t BUFFER_SIZE = 1024 * 100;

        bool string_view_to_uint64(std::string_view data, std::uint64_t& result) {
            bool result = false; // 转换失败
            if (data.empty()) {
                log_error << "Error: string_view is empty." << std::endl;
            } else {
                // 使用 std::from_chars 转换为 uint64_t
                auto [ptr, ec] = std::from_chars(data.data(), data.data() + data.size(), result);
                if (ec == std::errc()) {
                    result = true; // 转换成功
                } else if (ec == std::errc::invalid_argument) {
                    log_error << "Error: Invalid argument, not a valid integer." << std::endl;
                } else if (ec == std::errc::result_out_of_range) {
                    log_error << "Error: Integer out of range for uint64_t." << std::endl;
                }
            }
            return result;
        }

        bool need_transport(std::filesystem::path path) noexcept {
            // TODO check, use context_ if needed.
            return true;
        }

        void mark_transport_finally(std::filesystem::path path) noexcept {
            // TODO mark finally, use context_ if needed.
        }

        std::map<std::string, FileType> ext_to_types = {
            // FIXME
            {".json", FileType::TaskJson}, {".txt", FileType::TotalJson}, {".mjpeg", FileType::Video}, {".mjpg", FileType::Video}, {".jpeg", FileType::Jpeg},
            {".jpg", FileType::Jpeg}, {".ts", FileType::Timestamp}, {".txt", FileType::Pose}};

        FileType guess_type(std::filesystem::path path) noexcept {
            FileType result = FileType::TaskJson;
            if (auto it = ext_to_types.find(path.extension().string()); it != ext_to_types.end()) {
                result = it->second;
            }
            return result;
        }

        bool send_chunks(int sockfd, std::filesystem::path path, std::uint64_t transfer_id, std::uintmax_t file_size) {
            bool result = false;
            std::ifstream file;
            file.open(path.string(), std::ios::binary);
            if (file.is_open()) {
                char buffer[BUFFER_SIZE];
                std::size_t offset = 0;
                util::md5 digest;
                while (!file.eof()) {
                    auto chunk_length = std::min(BUFFER_SIZE, file_size - offset);
                    file.read(buffer, chunk_length);
                    auto read_length = file.gcount();
                    if (read_length) {
                        if (read_length == chunk_length) {
                            digest.update((std::uint8_t*)buffer, (std::uint8_t*)buffer + read_length);
                            iovec data[2];
                            auto chunk = ChunkHead{transfer_id, int(offset), int(read_length) /*, buffer*/};
                            data[0].iov_base = &chunk;
                            data[0].iov_len = ChunkHead::size();
                            data[1].iov_base = (void*)buffer;
                            data[1].iov_len = read_length;
                            auto r = util::send_data(sockfd, int(FileTransferType::Chunk), data, 2);
                            if (r == read_length) {
                                offset += read_length;
                            } else {
                                // TODO: send error
                                break;
                            }
                        } else {
                            // TODO: read error
                            break;
                        }
                    } else {
                        // TODO: read zero bytes
                        break;
                    }
                }
                if (offset == file_size) {
                    result = true;
                }
            }
            return result;
        }

        void send_file_impl(std::filesystem::path path, std::string const& camera_sn, std::uintmax_t file_size) noexcept {
            auto sockfd = c_pool->GetObject(); // get the socket, value wrap by std::optional
            if (sockfd.has_value()) {
                log_debug << "get connection from pool";
                auto c = std::any_cast<Context>(context_);
                auto f = FileMetaInfo(guess_type(path), c.big_task_id, c.task_id, camera_sn, file_size, path.filename().string());
                auto s = f.serialize();
                auto p = util::package{int(8 + s.length()), 1, int(FileTransferType::All), s};
                std::uint64_t transfer_id = 0;
                for (int i = 0; i < 3; ++i) {
                    try {
                        int r = util::send_packet(sockfd.value(), p);
                        auto r_p = util::receive_packet(sockfd.value(), 10);
                        if (string_view_to_uint64(r_p.data, transfer_id)) {
                            if (send_chunks(sockfd.value(), path, transfer_id, file_size)) {
                                mark_transport_finally(path);
                                break;
                            }
                        } else {
                            continue;
                        }
                    } catch (socket_exception& e) {
                        if (e.error == SocketError::Final) {
                            c_pool->CloseAll();
                            break;
                        } else if (e.error == SocketError::Temporary) {
                            c_pool->Reset(sockfd.value());
                        }
                    }
                }
                c_pool->RecycleObject(sockfd.value()); // release the socket
                log_debug << "recycled connection to pool";
            }
        }

        void send_whole_file_impl(std::filesystem::path path, std::string const& camera_sn, std::uintmax_t file_size) noexcept {
            auto sockfd = c_pool->GetObject(); // get the socket, value wrap by std::optional
            if (sockfd.has_value()) {
                log_debug << "get connection from pool";
                auto c = std::any_cast<Context>(context_);
                auto content = std::malloc(file_size);
                auto file = std::fopen(path.c_str(), "rb");
                std::fread(content, 1, file_size, file);
                std::fclose(file);
                auto f = FileMetaInfoWithContent(guess_type(path), c.big_task_id, c.task_id, camera_sn, file_size, path.filename().string(),
                    std::string_view((char*)content, file_size));
                std::free(content);
                auto s = f.serialize();
                auto p = util::package{int(8 + s.length()), 1, int(FileTransferType::All), s};
                for (int i = 0; i < 3; ++i) {
                    try {
                        int r = util::send_packet(sockfd.value(), p);
                        auto r_p = util::receive_packet(sockfd.value(), 10);
                        mark_transport_finally(path);
                        break;
                    } catch (socket_exception& e) {
                        if (e.error == SocketError::Final) {
                            c_pool->CloseAll();
                            break;
                        } else if (e.error == SocketError::Temporary) {
                            c_pool->Reset(sockfd.value());
                        }
                    }
                }
                // task_id = std::any_cast<std::string>(context_), whole_file_name = path.string(), file_name = path.filename().string()
                c_pool->RecycleObject(sockfd.value()); // release the socket
                log_debug << "recycled connection to pool";
            }
        }

        void send_file(std::filesystem::path path, std::string const& camera_sn) noexcept {
            auto file_size = std::filesystem::file_size(path);
            if (file_size > 2 * 1024 * 1024) {
                // TODO transfer chunked file
                send_file_impl(path, camera_sn, file_size);
            } else {
                // TODO transfer whole file
                send_whole_file_impl(path, camera_sn, file_size);
            }
        }
    } // namespace

    void initialize() noexcept {
        t_pool = std::make_shared<ThreadPool>(10);
        c_pool = std::make_shared<util::ConnectionPool>("127.0.0.1", 2222, 10, 100);
    }

    void transfer_files(std::any context, std::string const& folder, std::function<void(std::string const& file_name, int code, std::string const& message)>&& notify) noexcept {
        context_ = context;
        for (auto const& dir_entry : std::filesystem::directory_iterator{folder}) {
            if (dir_entry.is_directory()) {
                auto camera_sn = dir_entry.path().filename().string();
                for (auto const& file : std::filesystem::directory_iterator{dir_entry}) {
                    if (file.is_regular_file()) {
                        auto whole_file_path = file.path();
                        if (need_transport(whole_file_path)) {
                            t_pool->submit_task(0, nullptr, send_file, whole_file_path, camera_sn);
                        }
                    }
                }
            } else if (dir_entry.is_regular_file()) {
                auto whole_file_path = dir_entry.path();
                if (need_transport(whole_file_path)) {
                    t_pool->submit_task(0, nullptr, send_file, whole_file_path, "");
                }
            }
        }
        t_pool->wait_for_all_done();
    }

    void finalize() noexcept {
        c_pool->CloseAll();
        c_pool = nullptr;
        t_pool = nullptr;
    }
} // namespace util::FileTransfer
