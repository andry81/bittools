#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <windows.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <limits>
#include <algorithm>

#include "std/tstring.hpp"
#include "std/tstdio.hpp"

#include "tacklelib/utility/platform.hpp"
#include "tacklelib/utility/type_traits.hpp"
#include "tacklelib/utility/addressof.hpp"
#include "tacklelib/utility/utility.hpp"
#include "tacklelib/utility/math.hpp"
#include "tacklelib/utility/locale.hpp"

using uint_t = unsigned int;

// Bitwise memory copy.
// Both buffers must be padded to 7 bytes remainder to be able to read/write the last 8-bit block as 64-bit block.
// Buffers must not overlap.
//
inline void memcpy_bitwise64(uint8_t * to_padded_int64_buf, uint64_t to_first_bit_offset, uint8_t * from_padded_int64_buf, uint64_t from_first_bit_offset, uint64_t bit_size)
{
    assert(bit_size);

    uint64_t bit_offset = 0;

    uint32_t from_byte_offset = uint32_t(from_first_bit_offset / 8);
    uint32_t to_byte_offset = uint32_t(to_first_bit_offset / 8);

    uint32_t remainder_from_bit_offset = uint32_t(from_first_bit_offset % 8);
    uint32_t remainder_to_bit_offset = uint32_t(to_first_bit_offset % 8);

    while (bit_offset < bit_size) {
        if (remainder_to_bit_offset >= remainder_from_bit_offset && (remainder_to_bit_offset || remainder_from_bit_offset)) {
            const uint64_t from_bit_block = *(uint64_t *)&from_padded_int64_buf[from_byte_offset];
            uint64_t & to_bit_block = *(uint64_t *)&to_padded_int64_buf[to_byte_offset];

            const uint32_t to_first_bit_delta_offset = remainder_to_bit_offset - remainder_from_bit_offset;
            const uint64_t to_bit_block_inversed_mask = uint64_t(~0) << remainder_to_bit_offset;

            to_bit_block = ((from_bit_block << to_first_bit_delta_offset) & to_bit_block_inversed_mask) | (to_bit_block & ~to_bit_block_inversed_mask);

            const uint32_t bit_size_copied = 64 - remainder_to_bit_offset;

            bit_offset += bit_size_copied;

            from_first_bit_offset += bit_size_copied;
            to_first_bit_offset += bit_size_copied;

            if (remainder_to_bit_offset != remainder_from_bit_offset) {
                from_byte_offset += 7;
                to_byte_offset += 8;

                remainder_from_bit_offset = 8 - to_first_bit_delta_offset;
                remainder_to_bit_offset = 0;
            }
            else {
                from_byte_offset += 8;
                to_byte_offset += 8;

                remainder_from_bit_offset = 0;
                remainder_to_bit_offset = 0;
            }
        }
        else if (remainder_to_bit_offset < remainder_from_bit_offset) {
            const uint64_t from_bit_block = *(uint64_t *)&from_padded_int64_buf[from_byte_offset];
            uint64_t & to_bit_block = *(uint64_t *)&to_padded_int64_buf[to_byte_offset];

            const uint32_t to_first_bit_delta_offset = remainder_from_bit_offset - remainder_to_bit_offset;
            const uint64_t to_bit_block_inversed_mask = uint64_t(~0) << remainder_to_bit_offset;

            to_bit_block = ((from_bit_block >> to_first_bit_delta_offset) & to_bit_block_inversed_mask) | (to_bit_block & ~to_bit_block_inversed_mask);

            const uint32_t bit_size_copied = 64 - remainder_from_bit_offset;

            bit_offset += bit_size_copied;

            from_first_bit_offset += bit_size_copied;
            to_first_bit_offset += bit_size_copied;

            from_byte_offset += 8;
            to_byte_offset += 7;

            remainder_from_bit_offset = 0;
            remainder_to_bit_offset = (8 - to_first_bit_delta_offset);
        }
        else {
            // optimization
            const uint64_t bit_size_remain = bit_size - bit_offset;
            const uint32_t byte_size_remain = uint32_t(bit_size_remain / 8);

            memcpy(to_padded_int64_buf + to_byte_offset, from_padded_int64_buf + from_byte_offset, byte_size_remain + 1);

            break;
        }

        assert(from_byte_offset == uint32_t(from_first_bit_offset / 8));
        assert(remainder_from_bit_offset == uint32_t(from_first_bit_offset % 8));

        assert(to_byte_offset == uint32_t(to_first_bit_offset / 8));
        assert(remainder_to_bit_offset == uint32_t(to_first_bit_offset % 8));
    }
}

#endif
