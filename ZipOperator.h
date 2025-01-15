#pragma once

#include <functional> // for std::function<>
// #include <span> // for std::span<>
#include <string> // for std::string

namespace util::ZipOperator {
    enum version_os_part {
        // 0 - MS-DOS and OS/2(FAT/VFAT/FAT32 file systems)
        // 1 - Amiga                     2 - OpenVMS
        // 3 - UNIX                      4 - VM/CMS
        // 5 - Atari ST                  6 - OS/2 H.P.F.S.
        // 7 - Macintosh                 8 - Z-System
        // 9 - CP/M                      10 - Windows NTFS
        // 11 - MVS (OS/390 - Z/OS)      12 - VSE
        // 13 - Acorn Risc               14 - VFAT
        // 15 - alternate MVS            16 - BeOS
        // 17 - Tandem                   18 - OS/400
        // 19 - OS X (Darwin)            20 thru 255 - unused
    };

    enum version_require {
        // 1.0 - Default value：默认值。
        // 1.1 - File is a volume label：文件是一个卷标。
        // 2.0 - File is a folder (directory)：文件是一个文件夹(目录)。
        // 2.0 - File is compressed using Deflate compression：使用Deflate压缩方法对文件进行压缩。
        // 2.0 - File is encrypted using traditional PKWARE encryption：使用传统的PKWARE加密对文件进行加密。
        // 2.1 - File is compressed using Deflate64(tm)：使用Deflate64(tm)压缩方法对文件进行压缩。
        // 2.5 - File is compressed using PKWARE DCL Implode：使用PKWARE DCL Implode压缩方法对文件进行压缩。
        // 2.7 - File is a patch data set：文件是补丁数据集。
        // 4.5 - File uses ZIP64 format extensions：文件使用ZIP64格式扩展。
        // 4.6 - File is compressed using BZIP2 compression*：使用BZIP2压缩方法对文件进行压缩。
        // 5.0 - File is encrypted using DES：使用DES对文件进行加密。
        // 5.0 - File is encrypted using 3DES：使用3DES对文件进行加密。
        // 5.0 - File is encrypted using original RC2 encryption：使用原始RC2加密对文件进行加密。
        // 5.0 - File is encrypted using RC4 encryption：使用RC4加密对文件进行加密。
        // 5.1 - File is encrypted using AES encryption：使用AES加密对文件进行加密。
        // 5.1 - File is encrypted using corrected RC2 encryption**：使用更正的RC2加密对文件进行加密。
        // 5.2 - File is encrypted using corrected RC2-64 encryption**：使用更正的RC2-64加密对文件进行加密
        // 6.1 - File is encrypted using non-OAEP key wrapping*：使用non-OAEP密钥包装对文件进行加密。
        // 6.2 - Central directory encryption：中心目录加密。
        // 6.3 - File is compressed using LZMA：使用LZMA压缩方法对文件进行压缩。
        // 6.3 - File is compressed using PPMd+：使用PPMd+压缩方法对文件进行压缩。
        // 6.3 - File is encrypted using Blowfish：使用Blowfish对文件进行加密。
        // 6.3 - File is encrypted using Twofish：使用Twofish对文件进行加密。
    };

    enum flag_bits {
        // Bit 0: 如果设置，表示文件已加密。

        // (压缩方法6 - Imploding)
        // Bit 1: 如果使用的压缩方法是类型6，Imploding。如果设置了此位，表明
        //     使用了8K滑动词典。如果清除了此位，表明使用了4K滑动词典。
        // Bit 2: 如果使用的压缩方法是类型6，Imploding。如果设置了此位，表明
        //     使用了3个Shannon-Fano树来编码滑动字典输出。如果清除了此位，
        //     则使用了2个Shannon-Fano树。

        // (压缩方法8和9 - Deflating)
        // Bit 2  Bit 1
        // 0      0    使用了正常（-en）压缩选项。
        // 0      1    使用了最大（-exx/-ex）压缩选项。
        // 1      0    使用了快速（-ef）压缩选项。
        // 1      1    使用了超快速（-es）压缩选项。

        // (压缩方法14 - LZMA)
        // Bit 1: 如果使用的压缩方法是类型14，LZMA。如果设置了此位，表明使用
        //     end-of-stream(EOS)标记来标记压缩数据流的结束。如果清除了
        //     此位，则不使用EOS标记，并且必须知道提取的压缩数据大小。
        // 注意: 如果压缩方法是任何其他方式，则位1和2是不确定的。

        // Bit 3: 如果设置了此位，则本地头中的字段CRC-32，压缩大小和未压缩大小
        //     设置为零。正确的值将紧随压缩数据之后放入数据描述符中。
        // 注意：用于DOS的PKZIP版本2.04g仅在压缩方法8中识别此位，对于任何压缩方法，较新版本的PKZIP均识别该位。
        // Bit 4: 保留用于压缩方法8，以增强deflating。
        // Bit 5: 如果设置此位，则表明文件是压缩的补丁数据。
        // 注意：需要PKZIP版本2.70或更高版本。
        // Bit 6: 强加密功能。如果设置了此位，则必须将提取文件所需的版本设置为
        //     至少50，并且还必须设置位0。如果使用AES加密，则提取文件所需
        //     的版本必须至少为51。有关详细信息，请参见强加密规范。
        // Bit 7: 目前未使用。
        // Bit 8: 目前未使用。
        // Bit 9: 目前未使用。
        // Bit 10: 目前未使用。
        // Bit 11: 语言编码标志(EFS)。 如果该位置1，则该文件的文件名和注释字
        //         段必须使用UTF-8编码。(请参阅附录D)
        // Bit 12: 由PKWARE保留，用于增强压缩。
        // Bit 13: 在加密中心目录时设置，以指示本地头中的选定数据值被屏蔽以隐藏
        //         其实际值。有关详细信息，请参见描述强加密规范的部分。
        // Bit 14: 由PKWARE保留。
        // Bit 15: 由PKWARE保留。
    };

    enum compression_method_value {
        // 0 - The file is stored (no compression)：存储(无压缩)。
        // 1 - The file is Shrunk：文件缩小。
        // 2 - The file is Reduced with compression factor 1：待补充。
        // 3 - The file is Reduced with compression factor 2：待补充。
        // 4 - The file is Reduced with compression factor 3：待补充。
        // 5 - The file is Reduced with compression factor 4：待补充。
        // 6 - The file is Imploded：Imploding压缩算法。
        // 7 - Reserved for Tokenizing compression algorithm：为Tokenizing压缩算法保留。
        // 8 - The file is Deflated：Deflated压缩算法。
        // 9 - Enhanced Deflating using Deflate64(tm)：加强的压缩算法Deflate64(tm)。
        // 10 - PKWARE Data Compression Library Imploding (old IBM TERSE)：PKWARE DCL Implode压缩算法(旧的IBM TERSE压缩算法)。
        // 11 - Reserved by PKWARE：由PKWARE预留。
        // 12 - File is compressed using BZIP2 algorithm：BZIP2压缩算法。
        // 13 - Reserved by PKWARE：由PKWARE预留。
        // 14 - LZMA：LZMA压缩算法。
        // 15 - Reserved by PKWARE：由PKWARE预留。
        // 16 - IBM z/OS CMPSC Compression：IBM z/OS CMPSC压缩算法。
        // 17 - Reserved by PKWARE：由PKWARE预留。
        // 18 - File is compressed using IBM TERSE (new)：IBM TERSE压缩算法(新的)。
        // 19 - IBM LZ77 z Architecture (PFS)：IBM LZ77 z Architecture压缩算法。
        // 20 - deprecated (use method 93 for zstd)
        // 93 - Zstandard (zstd) Compression
        // 94 - MP3 Compression
        // 95 - XZ Compression
        // 96 - JPEG variant：JPEG变种压缩算法。
        // 97 - WavPack compressed data：WavPack压缩算法。
        // 98 - PPMd version I, Rev 1：PPMd压缩算法版本1。
        // 99 - AE-x encryption marker (see APPENDIX E)：AE-x加密标记(请参阅附录E)。
    };

    enum extra_field_header_id {
        // 0x0001 - Zip64 extended information extra field：ZIP64扩展信息扩展字段。
        // 0x0007 - AV Info：AV信息。
        // 0x0008 - Reserved for extended language encoding data (PFS)(see APPENDIX D)：保留用于扩展语言编码数据(PFS)（请参阅附录D）
        // 0x0009 - OS/2
        // 0x000A - NTFS
        // 0x000C - OpenVMS
        // 0x000D - UNIX
        // 0x000E - Reserved for file stream and fork descriptors：保留用于文件流和派生描述符。
        // 0x000F - Patch Descriptor：补丁描述符。
        // 0x0014 - PKCS#7 Store for X.509 Certificates：X.509证书的PKCS#7存储。
        // 0x0015 - X.509 Certificate ID and Signature for individual file：X.509证书ID和单个文件的签名。
        // 0x0016 - X.509 Certificate ID for Central Directory：中心目录的X.509证书ID。
        // 0x0017 - Strong Encryption Header：强加密头。
        // 0x0018 - Record Management Controls：记录管理控制。
        // 0x0019 - PKCS#7 Encryption Recipient Certificate List：PKCS#7加密对象证书列表。
        // 0x0020 - Reserved for Timestamp record：为Timestamp记录保留。
        // 0x0021 - Policy Decryption Key Record：策略解密密钥记录。
        // 0x0022 - Smartcrypt Key Provider Record：Smartcrypt密钥提供者记录。
        // 0x0023 - Smartcrypt Policy Key Data Record：Smartcrypt策略密钥数据记录。
        // 0x0065 - IBM S/390 (Z390), AS/400 (I400) attributes - uncompressed：IBM S/390 (Z390), AS/400 (I400)属性 - 未压缩。
        // 0x0066 - Reserved for IBM S/390 (Z390), AS/400 (I400) attributes - compressed：为IBM S/390 (Z390), AS/400 (I400)属性保留 - 压缩。
        // 0x4690 - POSZIP 4690 (reserved)：POSZIP 4690(保留)。
    };

    /*
Value	Size(byte)	Description
0x0001	2	此“扩展”块类型的标签
Size	2	“扩展”块的大小
Original Size	8	原始未压缩文件大小
Compressed Size	8	压缩数据的大小
Relative Header Offset	8	本地头记录的偏移量
Disk Start Number	4	该文件开始所在的磁盘号
    */

    /*
Value	Size(byte)	Description
0x0009	2	此“扩展”块类型的标签
TSize	2	以下数据块的大小
BSize	4	未压缩的块大小
CType	2	压缩类型
EACRC	4	解压缩块的CRC值
(var)	N	压缩块
    */

    /*
Value	Size(byte)	Description
0x000A	2	此“扩展”块类型的标签
TSize	2	总“扩展”块的大小
Reserved	4	保留以备将来使用
Tag1	2	NTFS属性＃1的标签
Size1	2	属性＃1的大小，以字节为单位
(var)	Size1	属性＃1数据
…	…	…
TagN	2	NTFS属性＃N的标签
SizeN	2	属性＃N的大小，以字节为单位
(var)	SizeN	属性＃N数据
    */

    /*
    only one tag define for NTFS
Value	Size(byte)	Description
0x0001	2	属性＃1的标签
Size1	2	属性＃1的大小，以字节为单位
Mtime	8	文件最后修改时间
Atime	8	文件最后访问时间
Ctime	8	文件创建时间
    */

    struct LocalHeader {
        std::uint8_t signature[4]; // 0x50 0x4B 0x03 0x04
        std::uint8_t version[2];
        std::uint8_t flag[2];
        std::uint8_t compression_method[2];
        std::uint8_t last_modify_time[2];
        std::uint8_t last_modify_date[2];
        std::uint8_t crc[4]; // 0xDEBB20E3
        std::uint8_t compress_size[4];
        std::uint8_t origin_size[4];
        std::uint8_t file_name_length[2];
        std::uint8_t extra_field_length[2];
        std::uint8_t file_name[];
        std::uint8_t extra_field[]; // block(id: 16bit, length: 16bit) ...
    };
    struct EncryptionHeader {};
    // using FileData = std::span<std::uint8_t>;
    struct DataDescription {
        std::uint8_t signature[4]; // 0x50 0x4B 0x07 0x08 option
        std::uint8_t crc[4];
        std::uint8_t compress_size[4];
        std::uint8_t origin_size[4];
    };
    // ... all below
    struct ArchiveDecryptionHeader {};
    struct ArchiveExtraDataRecord {
        std::uint8_t signature[4]; // 0x50 0x4B 0x06 0x08
        std::uint8_t extra_field_length[4];
        std::uint8_t extra_field[];
    };
    struct CenterDirectoryHeader {
        std::uint8_t signature[4]; // 0x50 0x4B 0x01 0x02
        std::uint8_t version[2];
        std::uint8_t extract_version[2];
        std::uint8_t flag[2];
        std::uint8_t compression_method[2];
        std::uint8_t last_modify_time[2];
        std::uint8_t last_modify_date[2];
        std::uint8_t crc[4];
        std::uint8_t compress_size[4];
        std::uint8_t origin_size[4];
        std::uint8_t file_name_length[2];
        std::uint8_t extra_field_length[2];
        std::uint8_t file_comment_length[2];
        std::uint8_t disk_number_start[2];
        std::uint8_t internal_file_attribute[2];
        std::uint8_t external_file_attribute[4];
        std::uint8_t relative_offset_of_local_header[4];
        std::uint8_t file_name[];
        std::uint8_t extra_field[];
        std::uint8_t file_comment[];
    };
    // ... one
    struct DigitalSignature {
        std::uint8_t signature[4]; // 0x50 0x4B 0x05 0x05
        std::uint8_t length[2];
        std::uint8_t data[];
    };
    struct Zip64EndOfCenterDirectoryRecord {
        std::uint8_t signature[4]; // 0x50 0x4B 0x06 0x06
        std::uint8_t size[8];
        std::uint8_t version[2];
        std::uint8_t extract_version[2];
        std::uint8_t disk_number[4];
        std::uint8_t center_directory_start_disk_number[4];
        std::uint8_t entry_count[8];
        std::uint8_t total_entry_count[8];
        std::uint8_t center_directory_length[8];
        std::uint8_t center_direcotry_offset[8];
        std::uint8_t extra_fields[]; // (headerId: 2Bytes, dataSize: 4Bytes, data: ...) ...
    };
    struct Zip64EndOfCenterDirectoryLocator {
        std::uint8_t signature[4]; // 0x50 0x4B 0x06 0x07
        std::uint8_t center_directory_start_disk_number[4];
        std::uint8_t relative_offset_end_entry[8];
        std::uint8_t disk_count[4];
    };
    struct EndOfCenterDirectoryRecord {
        std::uint8_t signature[4]; // 0x50 0x4B 0x05 0x06
        std::uint8_t disk_number[2];
        std::uint8_t center_directory_start_disk_number[2];
        std::uint8_t entry_count[2];
        std::uint8_t total_entry_count[2];
        std::uint8_t center_directory_length[4];
        std::uint8_t center_direcotry_offset[4];
        std::uint8_t comment_length[2];
        std::uint8_t comment[];
    };

    void unzip(std::string const& zip, std::string const& dir, std::function<void(int code, std::string const& message)>&& notify) noexcept;

    void zip(std::string const& dir, std::string const& zip, std::function<void(int code, std::string const& message)>&& notify) noexcept;
} // namespace util::ZipOperator
