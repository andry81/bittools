
#include "utility/crc.hpp"
#include "utility/crc_tables.hpp"
#include "utility/assert.hpp"
#include "utility/utility.hpp"

namespace
{
    template <typename T>
    FORCE_INLINE T _t_crc(const T * table, T crc, const void * buf, size_t size, T crc_init, T xor_in, T xor_out, bool input_reflected, bool result_reflected)
    {
        const uint8_t * p = (const uint8_t *)buf;

        if (!crc && crc_init) crc = crc_init;

        crc ^= xor_in;

        while (size--) {
            const uint8_t buf_byte = input_reflected ? utility::reverse(*p++) : *p++;
            crc = table[(crc ^ buf_byte) & 0xFF] ^ (crc >> 8);
        }

        if (result_reflected) {
            crc = utility::reverse(crc);
        }

        return crc ^ xor_out;
    }
}

namespace utility
{
    // NOTE:
    // 1. Online generator: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
    // 2. Understanding:    http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
    // 3. RFC1662 (HDLC FCS implementation): https://tools.ietf.org/html/rfc1662

    extern FORCE_INLINE uint16_t crc16(uint16_t polynomial, uint16_t crc, const void * buf, size_t size, uint16_t crc_init,
        uint16_t xor_in, uint16_t xor_out, bool input_reflected, bool result_reflected)
    {
        switch (polynomial) {
        case 0x1021: return _t_crc<uint16_t>(g_crc16_1021, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        case 0x8005: return _t_crc<uint16_t>(g_crc16_8005, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        case 0xC867: return _t_crc<uint16_t>(g_crc16_C867, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        default: ASSERT_TRUE(0); // not implemented
        }

        return 0;
    }

    extern FORCE_INLINE uint32_t crc32(uint32_t polynomial, uint32_t crc, const void * buf, size_t size, uint32_t crc_init,
        uint32_t xor_in, uint32_t xor_out, bool input_reflected, bool result_reflected)
    {
        switch (polynomial) {
        case 0x04C11DB7: return _t_crc<uint32_t>(g_crc32_04C11DB7, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        case 0x1EDC6F41: return _t_crc<uint32_t>(g_crc32_1EDC6F41, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        case 0xA833982B: return _t_crc<uint32_t>(g_crc32_A833982B, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        case 0x814141AB: return _t_crc<uint32_t>(g_crc32_814141AB, crc, buf, size, crc_init, xor_in, xor_out, input_reflected, result_reflected);
        default: ASSERT_TRUE(0); // not implemented
        }

        return 0;
    }
}
