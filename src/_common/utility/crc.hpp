#pragma once

#include "utility/platform.hpp"

#include <cstdint>

#include <utility>


namespace utility
{
    // crc-16 (CCIT ZERO, AUG CCITT, CCITT FALSE, GENIBUS, MCRF4XX, RIELLO, TMS37157, A, KERMIT, X.25, XMODEM)
    //
    //polynomial: x**0 + x**5 + x**12 + x**16
    //            (1)0001000000100001       = 0x1021
    //               1000010000001000(1)    = 0x8408
    std::pair<uint16_t, uint16_t> crc16_1021(uint16_t crc, const void * buf, size_t size, uint16_t crc_init = uint16_t(~0U), uint16_t xor_in = 0U, uint16_t xor_out = uint16_t(~0U));

    // crc-16 (CDMA2000)
    std::pair<uint16_t, uint16_t> crc16_C867(uint16_t crc, const void * buf, size_t size, uint16_t crc_init = uint16_t(~0U), uint16_t xor_in = 0, uint16_t xor_out = 0U);

    // crc-16 (ARC, BUYPASS, DDS 110, MAXIM, USB, MODBUS)
    std::pair<uint16_t, uint16_t> crc16_8005(uint16_t crc, const void * buf, size_t size, uint16_t crc_init = 0U, uint16_t xor_in = 0, uint16_t xor_out = 0U);

    // crc-32 (BZIP2, MPEG2, POSIX, JAMCRC)
    //
    //polynomial: x**0 + x**1 + x**2 + x**4 + x**5* +x**7 + x**8 + x**10 + x**11 + x**12 + x**16* +x**22 + x**23 + x**26 + x**32
    //            (1)00000100110000010001110110110111       = 0x04C11DB7
    //               11101101101110001000001100100000(1)    = 0xEDB88320
    std::pair<uint32_t, uint32_t> crc32_04C11DB7(uint32_t crc, const void * buf, size_t size, uint32_t crc_init = uint32_t(~0U), uint16_t xor_in = 0U, uint32_t xor_out = uint32_t(~0U));

    // crc-32 (C)
    std::pair<uint32_t, uint32_t> crc32_1EDC6F41(uint32_t crc, const void * buf, size_t size, uint32_t crc_init = uint32_t(~0U), uint16_t xor_in = 0U, uint32_t xor_out = uint32_t(~0U));

    // crc-32 (D)
    std::pair<uint32_t, uint32_t> crc32_A833982B(uint32_t crc, const void * buf, size_t size, uint32_t crc_init = uint32_t(~0U), uint16_t xor_in = 0U, uint32_t xor_out = uint32_t(~0U));

    // crc-32 (Q)
    std::pair<uint32_t, uint32_t> crc32_814141AB(uint32_t crc, const void * buf, size_t size, uint32_t crc_init = 0U, uint16_t xor_in = 0U, uint32_t xor_out = 0U);
}
