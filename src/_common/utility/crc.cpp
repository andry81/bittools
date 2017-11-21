
#include "utility/crc.hpp"
#include "utility/crc_tables.hpp"


namespace
{
    template <typename T>
    FORCE_INLINE T _t_crc(const T * table, T crc, const void * buf, size_t size, T crc_init, T xor_in, T xor_out)
    {
        const uint8_t * p = (const uint8_t *)buf;

        if (!crc && crc_init) crc = crc_init;

        crc ^= xor_in;

        while (size--)
            crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

        return crc ^ xor_out;
    }
}

namespace utility
{
    // NOTE:
    // 1. Online generator: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
    // 2. Understanding:    http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
    // 3. RFC1662 (HDLC FCS implementation): https://tools.ietf.org/html/rfc1662

    extern FORCE_INLINE std::pair<uint16_t, uint16_t> crc16_1021(uint16_t crc, const void * buf, size_t size, uint16_t crc_init, uint16_t xor_in, uint16_t xor_out)
    {
        return std::pair<uint16_t, uint16_t>(0x1021, _t_crc<uint16_t>(g_crc16_1021, crc, buf, size, crc_init, xor_in, xor_out));
    }

    extern FORCE_INLINE std::pair<uint16_t, uint16_t> crc16_C867(uint16_t crc, const void * buf, size_t size, uint16_t crc_init, uint16_t xor_in, uint16_t xor_out)
    {
        return std::pair<uint16_t, uint16_t>(0xC867, _t_crc<uint16_t>(g_crc16_C867, crc, buf, size, crc_init, xor_in, xor_out));
    }

    extern FORCE_INLINE std::pair<uint16_t, uint16_t> crc16_8005(uint16_t crc, const void * buf, size_t size, uint16_t crc_init, uint16_t xor_in, uint16_t xor_out)
    {
        return std::pair<uint16_t, uint16_t>(0x8005, _t_crc<uint16_t>(g_crc16_8005, crc, buf, size, crc_init, xor_in, xor_out));
    }

    extern FORCE_INLINE std::pair<uint32_t, uint32_t> crc32_04C11DB7(uint32_t crc, const void * buf, size_t size, uint32_t crc_init, uint32_t xor_in, uint32_t xor_out)
    {
        return std::pair<uint32_t, uint32_t>(0x04C11DB7, _t_crc<uint32_t>(g_crc32_04C11DB7, crc, buf, size, crc_init, xor_in, xor_out));
    }

    extern FORCE_INLINE std::pair<uint32_t, uint32_t> crc32_1EDC6F41(uint32_t crc, const void * buf, size_t size, uint32_t crc_init, uint32_t xor_in, uint32_t xor_out)
    {
        return std::pair<uint32_t, uint32_t>(0x1EDC6F41, _t_crc<uint32_t>(g_crc32_1EDC6F41, crc, buf, size, crc_init, xor_in, xor_out));
    }

    extern FORCE_INLINE std::pair<uint32_t, uint32_t> crc32_A833982B(uint32_t crc, const void * buf, size_t size, uint32_t crc_init, uint32_t xor_in, uint32_t xor_out)
    {
        return std::pair<uint32_t, uint32_t>(0xA833982B, _t_crc<uint32_t>(g_crc32_A833982B, crc, buf, size, crc_init, xor_in, xor_out));
    }

    extern FORCE_INLINE std::pair<uint32_t, uint32_t> crc32_814141AB(uint32_t crc, const void * buf, size_t size, uint32_t crc_init, uint32_t xor_in, uint32_t xor_out)
    {
        return std::pair<uint32_t, uint32_t>(0x814141AB, _t_crc<uint32_t>(g_crc32_814141AB, crc, buf, size, crc_init, xor_in, xor_out));
    }
}
