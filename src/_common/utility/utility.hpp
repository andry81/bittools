#pragma once

#include "tackle/file_handle.hpp"

#include <bitset>


#define if_break(x) if(!(x)); else switch(0) case 0: default:

#define SCOPED_TYPEDEF(type_, typedef_) typedef struct { typedef type_ type; } typedef_

namespace utility
{
    using namespace tackle;

    size_t get_file_size(FileHandle file_handle);
    bool is_files_equal(FileHandle left_file_handle, FileHandle right_file_handle);
    FileHandle recreate_file(const std::string & file_path, const char * mode, int flags, size_t size = 0, uint32_t fill_by = 0);
    FileHandle create_file(const std::string & file_path, const char * mode, int flags, size_t size = 0, uint32_t fill_by = 0);
    FileHandle open_file(const std::string & file_path, const char * mode, int flags, size_t creation_size = 0, size_t resize_if_existed = -1, uint32_t fill_by_on_creation = 0);

    template<typename T>
    std::string int_to_hex(T i, size_t padding = sizeof(T) * 2)
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(padding)
            << std::hex << i;
        return stream.str();
    }

    template<typename T>
    std::string int_to_dec(T i, size_t padding = sizeof(T) * 2)
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(padding)
            << std::dec << i;
        return stream.str();
    }

    template<typename T>
    std::string int_to_bin(T i, bool first_bit_is_lowest_bit = false) {
        std::bitset<sizeof(T) * CHAR_BIT> bs(i);
        if (!first_bit_is_lowest_bit) {
            return bs.to_string();
        }

        const std::string ret = bs.to_string();
        return std::string(ret.rbegin(), ret.rend());
    }

    inline uint8_t reverse(uint8_t byte) {
        byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
        byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
        byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        return byte;
    }
}

// to suppress compilation warning:
//  `warning C4146 : unary minus operator applied to unsigned type, result still unsigned`
inline unsigned int negate(unsigned int i)
{
    return unsigned int(-int(i));
}

inline unsigned long negate(unsigned long i)
{
    return unsigned long(-long(i));
}

inline unsigned long long negate(unsigned long long i)
{
    return unsigned long long(-long long(i));
}

template<typename T>
inline uint32_t rotl(uint32_t n, unsigned int c)
{
    static_assert(sizeof(uint32_t) >= sizeof(T), "sizeof(T) must be less or equal to the sizeof(uint32_t)");
    const uint32_t byte_mask = uint32_t(-1) >> (CHAR_BIT * (sizeof(uint32_t) - sizeof(T)));
    const uint32_t mask = (CHAR_BIT * sizeof(T) - 1);
    c &= mask;
    return byte_mask & ((n << c) | ((byte_mask & n) >> (mask & negate(c))));
}

template<typename T>
inline uint32_t rotr(uint32_t n, unsigned int c)
{
    static_assert(sizeof(uint32_t) >= sizeof(T), "sizeof(T) must be less or equal to the sizeof(uint32_t)");
    const uint32_t byte_mask = uint32_t(-1) >> (CHAR_BIT * (sizeof(uint32_t) - sizeof(T)));
    const uint32_t mask = (CHAR_BIT * sizeof(T) - 1);
    c &= mask;
    return byte_mask & (((byte_mask & n) >> c) | (n << (mask & negate(c))));
}

inline uint32_t rotl8(uint32_t n, unsigned int c)
{
    return rotl<uint8_t>(n, c);
}

inline uint32_t rotr8(uint32_t n, unsigned int c)
{
    return rotr<uint8_t>(n, c);
}

inline uint32_t rotl16(uint32_t n, unsigned int c)
{
    return rotl<uint16_t>(n, c);
}

inline uint32_t rotr16(uint32_t n, unsigned int c)
{
    return rotr<uint16_t>(n, c);
}
