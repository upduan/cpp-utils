#include "base64.h"

namespace util::base64 {
    // std::string const encode(std::string_view data) {
    //     int base64Length = Base64EncodeGetRequiredLength(static_cast<int>(data.length()));
    //     LPSTR bytes = (LPSTR)malloc(base64Length);
    //     Base64Encode(reinterpret_cast<const BYTE*>(data.data()), static_cast<int>(data.length()), bytes, &base64Length);
    //     auto r = std::string(bytes, base64Length);
    //     free(bytes);
    //     return r;
    // }

    // std::string const decode(std::string_view data) {
    //     int originLength = Base64DecodeGetRequiredLength(static_cast<int>(data.length()));
    //     BYTE* bytes = (BYTE*)malloc(originLength);
    //     Base64Decode(reinterpret_cast<LPCSTR>(data.data()), static_cast<int>(data.length()), bytes, &originLength);
    //     auto r = std::string(reinterpret_cast<char*>(bytes), originLength);
    //     free(bytes);
    //     return r;
    // }

    std::string encode(std::string_view content) noexcept {
        constexpr char const* const pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        // 7 6 5 4 3 2 | 1 0 7 6 5 4 | 3 2 1 0 7 6 | 5 4 3 2 1 0
        // 5 4 3 2 1 0 | 5 4 3 2 1 0 | 5 4 3 2 1 0 | 5 4 3 2 1 0
        std::string result;
        std::uint8_t const* data = (std::uint8_t const*)content.data();
        std::size_t length = content.length();
        result.reserve((length + 2) / 3 * 4);
        auto offset = 0u;
        auto current = data;
        for (;;) {
            char r[5] = "====";

            std::size_t index = *current >> 2; // first
            r[0] = pattern[index];

            index = (*current & 0b11) << 4; // second
            ++current;
            ++offset;
            if (offset >= length) {
                r[1] = pattern[index];
                result += r;
                break;
            }
            index = index | (*current >> 4); // second
            r[1] = pattern[index];

            index = (*current & 0b1111) << 2; // third
            ++current;
            ++offset;
            if (offset >= length) {
                r[2] = pattern[index];
                result += r;
                break;
            }
            index = index | (*current >> 6); // third
            r[2] = pattern[index];

            index = *current & 0b111111; // fourth
            r[3] = pattern[index];

            result += r;

            ++current;
            ++offset;
            if (offset >= length) {
                break;
            }
        }
        return result;
    }

    std::vector<std::uint8_t> decode(std::string_view data) noexcept {
        auto convert = [](char data) noexcept {
            char chr = data;
            if (chr >= 0x41 && chr <= 0x5A) {
                chr = chr - 0x41;
            } else if (chr >= 0x61 && chr <= 0x7A) {
                chr = chr - 0x61 + 0x1A;
            } else if (chr >= 0x30 && chr <= 0x39) {
                chr = chr - 0x30 + 0x34;
            } else if (chr == 0x2B) {
                chr = 0x3E;
            } else if (chr == 0x2D) {
                chr = 0x3F;
            } else if (chr == '=') {
                chr = 0;
            }
            return chr & 0b111111;
        };
        // 0x41-0x5A A-Z 0x0-0x19
        // 0x61-0x7A a-z 0x20-0x33
        // 0x30-0x39 0-9 0x34-0x3D
        // 0x2B + 0x3E
        // 0x2F / 0x3F
        //     ^                 ^                 ^                 ^
        // 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0 | 7 6 5 4 3 2 1 0
        // 0 0 0 0 0 0 0 0 | 5 4 3 2 1 0 5 4 | 3 2 1 0 5 4 3 2 | 1 0 5 4 3 2 1 0
        //                   v           v             v             v
        std::vector<std::uint8_t> result;
        // FIXME auto lines = util::TextOps::split(data, "\\n");
        std::vector<std::string> lines = {};
        std::string stripNewline;
        if (lines.empty()) {
            stripNewline = data;
        } else {
            for (auto const& line : lines) {
                stripNewline += line;
            }
        }
        // std::replace(data.begin(), data.end(), "\\n", "");
        result.reserve(stripNewline.length() / 4 * 3);
        // Log::log("result length is %d", data.length() / 4 * 3);
        for (size_t i = 0; i < stripNewline.length(); i += 4) {
            // Log::log("23-18: %d", convert(data[i]));
            // Log::log("17-12: %d", convert(data[i + 1]));
            // Log::log("11-6: %d", convert(data[i + 2]));
            // Log::log("5-0: %d", convert(data[i + 3]));
            int v = (convert(stripNewline[i]) << 18) | (convert(stripNewline[i + 1]) << 12) | (convert(stripNewline[i + 2]) << 6) | (convert(stripNewline[i + 3]));
            // Log::log("value is %d", v); // 11,403,522
            result.push_back(std::uint8_t((v & 0xFF0000) >> 16));
            result.push_back(std::uint8_t((v & 0xFF00) >> 8));
            result.push_back(std::uint8_t((v & 0xFF) >> 0));
        }
        return result;
    }
} // namespace utils::base64

/*
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <iostream>

using namespace boost::archive::iterators;
using namespace std;

int main(int argc, char **argv) {
    typedef transform_width< binary_from_base64<remove_whitespace<string::const_iterator> >, 8, 6 > it_binary_t;
    typedef insert_linebreaks<base64_from_binary<transform_width<string::const_iterator, 6, 8> >, 72 > it_base64_t;
    string s;
    getline(cin, s, '\n');
    cout << "Your string is: '" << s << "'" << endl;

    // Encode
    unsigned int writePaddChars = (3 - s.length() % 3) % 3;
    string base64(it_base64_t(s.begin()), it_base64_t(s.end()));
    base64.append(writePaddChars, '=');

    cout << "Base64 representation: " << base64 << endl;

    // Decode
    unsigned int paddChars = count(base64.begin(), base64.end(), '=');
    std::replace(base64.begin(), base64.end(), '=', 'A'); // replace '=' by base64 encoding of '\0'
    string result(it_binary_t(base64.begin()), it_binary_t(base64.end())); // decode
    result.erase(result.end() - paddChars, result.end());  // erase padding '\0' characters
    cout << "Decoded: " << result << endl;
    return 0;
}
*/