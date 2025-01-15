#include "ZipOperator.h"

#include <filesystem>

#include <ioapi.h>
#include <unzip.h>
#include <zip.h>

namespace util::ZipOperator {
    void unzip(std::string const& zip, std::string const& dir, std::function<void(int code, std::string const& message)>&& notify) noexcept {
        if (auto zipHandle = unzOpen(zip.c_str()); zipHandle) {
            unz_global_info globalInfo;
            if (unzGetGlobalInfo(zipHandle, &globalInfo) == UNZ_OK) {
                char notifyBuffer[256]{0};
                std::sprintf(notifyBuffer, "%d", globalInfo.number_entry);
                notify(1, notifyBuffer);
                if (!std::filesystem::exists(dir)) {
                    std::filesystem::create_directory(dir);
                }
                constexpr int const MAX_PATH = 260;
                char fileName[MAX_PATH]{};
                char extraField[MAX_PATH]{};
                char comment[MAX_PATH]{};
                for (auto i = 0ul; i < globalInfo.number_entry; ++i) {
                    unz_file_info fileInfo;
                    if (unzGetCurrentFileInfo(zipHandle, &fileInfo, fileName, MAX_PATH, extraField, MAX_PATH, comment, MAX_PATH) == UNZ_OK) {
                        std::string entryName = fileName;
                        if (fileInfo.external_fa == 0x10 || (entryName.rfind('/') == entryName.length() - 1)) {
                            std::filesystem::create_directory(dir + "/" + fileName);
                            notify(2, fileName);
                        } else {
                            if (unzOpenCurrentFile(zipHandle) == UNZ_OK) {
                                std::string filePath = dir + "/" + fileName;
                                if (std::filesystem::exists(filePath)) {
                                    notify(-7, "file already exists");
                                } else {
                                    FILE* entryFile = std::fopen(filePath.c_str(), "wb+");
                                    if (entryFile) {
                                        auto fileContentSize = fileInfo.uncompressed_size;
                                        if (auto readBuffer = (char*)std::malloc(fileContentSize); readBuffer) {
                                            while (true) {
                                                std::memset(readBuffer, 0, fileContentSize);
                                                auto readSize = unzReadCurrentFile(zipHandle, readBuffer, fileContentSize);
                                                if (readSize > 0) {
                                                    if (readSize != fileContentSize) {
                                                        notify(-9, "read current file failed");
                                                    }
                                                    auto writeSize = std::fwrite(readBuffer, 1, readSize, entryFile);
                                                    if (writeSize != readSize) {
                                                        notify(-10, "write current file failed");
                                                    }
                                                }
                                                unzCloseCurrentFile(zipHandle);
                                                std::fclose(entryFile);
                                                if (readSize < 0) {
                                                    notify(-9, "read current file failed");
                                                } else if (readSize == 0) {
                                                    notify(3, fileName);
                                                } else {
                                                    notify(3, fileName);
                                                }
                                                break;
                                            }
                                            std::free(readBuffer);
                                        } else {
                                            break;
                                        }
                                        std::fclose(entryFile);
                                    } else {
                                        notify(-8, "create read buffer failed");
                                    }
                                }
                            } else {
                                notify(-5, "open current file failed");
                            }
                        }
                        if (auto r = unzGoToNextFile(zipHandle); r == UNZ_OK) {
                        } else {
                            if (r == UNZ_END_OF_LIST_OF_FILE) {
                                notify(4, "all entry is processed");
                                break;
                            } else {
                                notify(-4, "move to next file failed");
                            }
                        }
                    } else {
                        notify(-3, "get current file info failed");
                        break;
                    }
                }
                notify(4, "all entry is processed");
            } else {
                notify(-2, "get global info failed");
            }
            unzClose(zipHandle);
        } else {
            notify(-1, "open zip file failed");
        }
    }

    namespace {
        std::uint32_t getFileCrc(std::string const& fileName) noexcept {
            std::uint32_t result = 0;
            if (auto file = std::fopen(fileName.c_str(), "rb"); file) {
                constexpr int bufferSize = 1024 * 16;
                std::uint8_t buffer[bufferSize]{};
                std::size_t readSize = 0;
                do {
                    readSize = std::fread(buffer, 1, bufferSize, file);
                    result = crc32_z(result, buffer, readSize);
                } while (readSize > 0);
                std::fclose(file);
            }
        }

        int isLargeFile(std::string const& fileName) noexcept {
            int result = 0;
            if (FILE* file = std::fopen(fileName.c_str(), "rb"); file) {
                std::fseek(file, 0, SEEK_END);
                ZPOS64_T pos = (ZPOS64_T)std::ftell(file);
                if (pos >= 0xffffffff) {
                    result = 1;
                }
                fclose(file);
            }
            return result;
        }
    } // namespace

    void zip(std::string const& dir, std::string const& zip, std::function<void(int code, std::string const& message)>&& notify) noexcept {
        if (std::filesystem::exists(dir)) {
            if (auto zipHandle = zipOpen64(zip.c_str(), 0); zipHandle) {
                for (auto const& dir_entry : std::filesystem::directory_iterator{dir}) {
                    if (std::filesystem::is_directory(dir_entry)) {
                        // create directory to zip
                    } else {
                        zip_fileinfo zi;
                        if (auto err = zipOpenNewFileInZip3_64(zipHandle, entryName, &zi, NULL, 0, NULL, 0, NULL /* comment*/, (opt_compress_level != 0) ? Z_DEFLATED : 0,
                                opt_compress_level, 0,
                                /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
                                -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, password, getFileCrc(entryName), isLargeFile(entryName));
                            err == ZIP_OK) {
                            do {
                                // read file
                                err = zipWriteInFileInZip(zipHandle, fileContent, (unsigned)contentSize);
                            } while (true);
                            err = zipCloseFileInZip(zipHandle);
                        } else {
                            notify(-4, "create file entry failed");
                        }
                    }
                }
                auto err = zipClose(zipHandle, nullptr);
                if (err != ZIP_OK) {
                    notify(-3, "close zip file failed");
                }
            } else {
                notify(-2, "create zip file failed");
            }
        } else {
            notify(-1, "dir not exists");
        }
    }
} // namespace util::ZipOperator
