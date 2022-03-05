// Author: Andrey Dibrov (andry at inbox dot ru)
//

#include "bitsync.hpp"

#include <tacklelib/utility/memory.hpp>

#include <algorithm>

#include <stdlib.h>
#include <time.h>


namespace {
    // max_bit_size     -> -1
    // half_bit_size    ->  0
    // 0                -> +1
    float _normaliza_num_bits(uint32_t num_bits, uint32_t half_bit_size, uint32_t max_bit_size)
    {
        assert(half_bit_size < max_bit_size);
        assert(max_bit_size >= num_bits);
        return half_bit_size >= num_bits ?
            float(half_bit_size - num_bits) / half_bit_size :
            -float(num_bits - half_bit_size) / (max_bit_size - half_bit_size);
    }
}

Flags g_flags       = {};
Options g_options   = {};


Flags::Flags()
{
    // raw initialization
    memset(this, 0, sizeof(*this));
};

void Flags::clear()
{
    *this = Flags{};
}

Options::Options()
{
    // by default:
    //  * all min values based on a synchro sequence bit length
    //  * all max values based on a stream bit length
    //
    stream_byte_size                    = 0;
    stream_bit_size                     = 0;
    stream_min_period                   = math::uint32_max;
    stream_max_period                   = math::uint32_max;
    max_periods_in_offset               = math::uint32_max;
    syncseq_bit_size                    = 0;
    syncseq_int32                       = 0;
    syncseq_min_repeat                  = math::uint32_max;
    syncseq_max_repeat                  = math::uint32_max;
    bits_per_baud                       = 0;
    gen_input_noise_bit_block_size      = 0;
    gen_input_noise_block_bit_prob      = 0;
    insert_output_syncseq_first_offset  = math::uint32_max;
    insert_output_syncseq_end_offset    = math::uint32_max;
    insert_output_syncseq_period        = 0;
    insert_output_syncseq_period_repeat = math::uint32_max;
    autocorr_min                        = 0;
    autocorr_mean_buf_max_size_mb       = 400; // 400 Mb is default
}


void Options::clear()
{
    *this = Options{};
}

// Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
//
inline void generate_noise(const BasicData & basic_data, StreamParams & stream_params, NoiseParams & noise_params, uint8_t * buf, uint32_t size)
{
    uint8_t * buf_out = buf;

    const uint64_t stream_bit_size = size * 8;

    const uint64_t stream_bit_start_offset = stream_params.last_bit_offset;

    uint64_t stream_bit_offset = stream_bit_start_offset;

    // generate input noise
    const uint32_t gen_input_noise_bit_block_size = basic_data.options_ptr->gen_input_noise_bit_block_size;

    if (gen_input_noise_bit_block_size) {
        uint64_t buf_bit_offset = 0;
        uint64_t buf_remain_bit_size = stream_bit_size;

        const uint32_t gen_input_noise_block_bit_prob = basic_data.options_ptr->gen_input_noise_block_bit_prob;

        uint64_t last_gen_noise_bits_block_index = noise_params.last_gen_noise_bits_block_index;

        uint64_t gen_input_noise_bits_block_index = stream_bit_offset / gen_input_noise_bit_block_size;
        const uint64_t gen_noise_block_bit_offset = stream_bit_offset % gen_input_noise_bit_block_size;

        // remaining part must be always in the last block otherwise the block index is invalid
        uint32_t remain_block_bit_size = uint32_t(gen_input_noise_bit_block_size - gen_noise_block_bit_offset);

        // skip remaining bits block from the previous run if bits block index ahead
        if (buf_remain_bit_size && gen_input_noise_bits_block_index < last_gen_noise_bits_block_index) {
            if (buf_remain_bit_size >= remain_block_bit_size) {
                buf_remain_bit_size -= remain_block_bit_size;
                stream_bit_offset += remain_block_bit_size;
                buf_bit_offset += remain_block_bit_size;
            }
            else {
                stream_bit_offset += buf_remain_bit_size;
                buf_bit_offset += buf_remain_bit_size;
                buf_remain_bit_size = 0;
            }

            gen_input_noise_bits_block_index++;

            if (buf_remain_bit_size && gen_input_noise_bits_block_index < last_gen_noise_bits_block_index) {
                const uint64_t remain_bit_blocks_to_skip = last_gen_noise_bits_block_index - gen_input_noise_bits_block_index;
                const uint64_t remain_bit_size_to_skip = remain_bit_blocks_to_skip * gen_input_noise_bit_block_size;

                if (buf_remain_bit_size >= remain_bit_size_to_skip) {
                    buf_remain_bit_size -= remain_bit_size_to_skip;
                    stream_bit_offset += remain_block_bit_size;
                    buf_bit_offset += remain_block_bit_size;

                    gen_input_noise_bits_block_index += remain_bit_blocks_to_skip;
                }
                else {
                    stream_bit_offset += buf_remain_bit_size;
                    buf_bit_offset += buf_remain_bit_size;
                    buf_remain_bit_size = 0;

                    gen_input_noise_bits_block_index += buf_remain_bit_size / gen_input_noise_bit_block_size;
                }
            }
        }

        // recalculate last bits block
        if (buf_remain_bit_size) {
            gen_input_noise_bits_block_index = stream_bit_offset / gen_input_noise_bit_block_size;
            const uint64_t gen_noise_block_bit_offset = stream_bit_offset % gen_input_noise_bit_block_size;

            remain_block_bit_size = uint32_t(gen_input_noise_bit_block_size - gen_noise_block_bit_offset);

            srand(uint_t(time(NULL)));
        }

        // process remaining bits block from the previous run as first bits block
        while (buf_remain_bit_size) {
            uint32_t remain_block_bit_prob = 100;

            if (gen_input_noise_block_bit_prob < 100) {
                const int remain_block_bit_prob_rand = rand();

                // rand normalization to 1-100 range
                remain_block_bit_prob = uint32_t(100) * remain_block_bit_prob_rand / RAND_MAX;
            }

            if (gen_input_noise_block_bit_prob >= remain_block_bit_prob) {
                const int remain_block_bit_offset_rand = rand();

                // rand normalization to 0-`remain_block_bit_size-1`
                assert(remain_block_bit_size);
                const uint32_t remain_block_bit_offset = remain_block_bit_size ?    // just in case
                    (remain_block_bit_offset_rand < RAND_MAX ?                      // out of range check
                        uint32_t(uint64_t(remain_block_bit_size) * remain_block_bit_offset_rand / RAND_MAX) :
                        remain_block_bit_size - 1) :
                    0;

                if (buf_remain_bit_size >= remain_block_bit_size || remain_block_bit_offset < buf_remain_bit_size) {
                    // bit inversion
                    const uint64_t byte_offset = (stream_bit_offset + remain_block_bit_offset) / 8;
                    const uint32_t remain_bit_offset = (stream_bit_offset + remain_block_bit_offset) % 8;

                    uint32_t & buf32_out = *(uint32_t *)(buf_out + byte_offset);

                    buf32_out ^= (uint32_t(0x01) << remain_bit_offset);

                    gen_input_noise_bits_block_index++;
                }
                else {
                    // TODO:
                    //  Remember here the bit to inverse in the remaining block to inverse it unconditionally in the next function call
                    //  instead of call to the `rand()` again.
                    //
                }
            }

            if (buf_remain_bit_size >= remain_block_bit_size) {
                buf_remain_bit_size -= remain_block_bit_size;
                stream_bit_offset += remain_block_bit_size;
                buf_bit_offset += remain_block_bit_size;

                remain_block_bit_size = gen_input_noise_bit_block_size;
            }
            else {
                buf_remain_bit_size = 0;
                stream_bit_offset += buf_remain_bit_size;
                buf_bit_offset += buf_remain_bit_size;

                remain_block_bit_size = 0;
            }
        }

        if (last_gen_noise_bits_block_index < gen_input_noise_bits_block_index) {
            last_gen_noise_bits_block_index = gen_input_noise_bits_block_index;
        }

        stream_params.last_bit_offset = stream_bit_offset;

        noise_params.last_gen_noise_bits_block_index = last_gen_noise_bits_block_index;
    }
}

inline void write_syncseq(
    const tackle::file_handle<TCHAR> & file_out_handle, uint8_t * buf_in, size_t size, uint32_t syncseq_int32, uint32_t syncseq_bit_size,
    uint32_t first_offset, uint32_t end_offset, uint32_t period, uint32_t period_repeat, bool insert_instead_fill)
{
    assert(size);
    assert(syncseq_bit_size);
    assert(period);

    const uint64_t stream_bit_size = size * 8;
    assert(first_offset < stream_bit_size);

    const uint64_t syncseq_mask64 = uint32_t(~(~uint64_t(0) << syncseq_bit_size));
    const uint64_t syncseq_bytes = syncseq_int32 & uint32_t(syncseq_mask64);

    uint64_t inserted_stream_bit_size;
    uint32_t inserted_stream_byte_size;

    if (insert_instead_fill && syncseq_bit_size < period) {
        inserted_stream_bit_size = stream_bit_size + ((stream_bit_size - first_offset - 1) / period + 1) * syncseq_bit_size;
        inserted_stream_byte_size = uint32_t((inserted_stream_bit_size + 7) / 8);
    }
    // no data between overlapped synchro sequence, nothing to increase
    else {
        inserted_stream_bit_size = stream_bit_size;
        inserted_stream_byte_size = size;
    }

    // Output buffer must be padded to 7 bytes remainder to be able to read and shift the last 8-bit block as 64-bit block.
    // Each byte can be splitted maximum by a 32-bit block, which needs at least 40-bit block bit arithmetic.
    //
    const uint64_t padded_inserted_stream_byte_size = inserted_stream_byte_size + 7;

    utility::Buffer inserted_write_buf{ size_t(padded_inserted_stream_byte_size) };

    uint8_t * buf_out = inserted_write_buf.get();

    // zeroing last byte remainder + padding bytes
    assert(inserted_stream_byte_size);
    for (uint64_t i = inserted_stream_byte_size - 1; i < padded_inserted_stream_byte_size; i++) {
        buf_out[i] = 0;
    }

    uint32_t byte_size_before_offset = size_t(first_offset + 7) / 8; // including the reminder

    if (byte_size_before_offset > 8) {
        memcpy(buf_out, buf_in, byte_size_before_offset);
    }
    // optimization
    else if (byte_size_before_offset) {
        *(uint64_t *)buf_out = *(uint64_t *)buf_in;
    }

    uint64_t from_first_bit_offset = first_offset;
    uint64_t from_end_bit_offset = (std::min)(uint64_t(end_offset), stream_bit_size);

    uint64_t to_bit_offset = first_offset;

    uint32_t period_repeat_count = 0;

    uint64_t syncseq_bit_size_to_copy;
    uint64_t period_remainder_bit_size_to_copy;

    if (syncseq_bit_size < period) {
        const uint64_t from_bit_step = period - syncseq_bit_size;

        while (from_first_bit_offset < from_end_bit_offset && to_bit_offset < inserted_stream_bit_size) {
            syncseq_bit_size_to_copy = (std::min)(inserted_stream_bit_size - to_bit_offset, uint64_t(syncseq_bit_size));    // `to` limit
            syncseq_bit_size_to_copy = (std::min)(from_end_bit_offset - from_first_bit_offset, syncseq_bit_size_to_copy);   // `from` limit

            utility::memcpy_bitwise64(buf_out, to_bit_offset, (uint8_t*)&syncseq_bytes, 0, syncseq_bit_size_to_copy);

            to_bit_offset += syncseq_bit_size_to_copy;

            if (insert_instead_fill) {
                if (to_bit_offset >= inserted_stream_bit_size) {
                    break;
                }
            }
            else {
                from_first_bit_offset += syncseq_bit_size_to_copy;

                if (from_first_bit_offset >= from_end_bit_offset) {
                    break;
                }
            }

            period_remainder_bit_size_to_copy = (std::min)(inserted_stream_bit_size - to_bit_offset, from_bit_step);                        // `to` limit
            period_remainder_bit_size_to_copy = (std::min)(from_end_bit_offset - from_first_bit_offset, period_remainder_bit_size_to_copy); // `from` limit

            if (period_remainder_bit_size_to_copy) {
                utility::memcpy_bitwise64(buf_out, to_bit_offset, buf_in, from_first_bit_offset, period_remainder_bit_size_to_copy);
            }

            from_first_bit_offset += period_remainder_bit_size_to_copy;
            to_bit_offset += period_remainder_bit_size_to_copy;

            period_repeat_count++;

            if (period_repeat < period_repeat_count) {
                break;
            }
        }
    }
    else {
        while (from_first_bit_offset < from_end_bit_offset && to_bit_offset < inserted_stream_bit_size) {
            syncseq_bit_size_to_copy = (std::min)(inserted_stream_bit_size - to_bit_offset, uint64_t(syncseq_bit_size));    // `to` limit
            syncseq_bit_size_to_copy = (std::min)(from_end_bit_offset - from_first_bit_offset, syncseq_bit_size_to_copy);   // `from` limit

            if (syncseq_bit_size_to_copy) {
                utility::memcpy_bitwise64(buf_out, to_bit_offset, (uint8_t*)&syncseq_bytes, 0, uint32_t(syncseq_bit_size_to_copy));
            }

            const uint64_t from_bit_step = (std::min)(uint64_t(period), syncseq_bit_size_to_copy);

            from_first_bit_offset += from_bit_step;
            to_bit_offset += from_bit_step;

            period_repeat_count++;

            if (period_repeat < period_repeat_count) {
                break;
            }
        }
    }

    if (from_first_bit_offset < stream_bit_size && to_bit_offset < inserted_stream_bit_size) {
        period_remainder_bit_size_to_copy = inserted_stream_bit_size - to_bit_offset;                                               // `to` limit
        period_remainder_bit_size_to_copy = (std::min)(stream_bit_size - from_first_bit_offset, period_remainder_bit_size_to_copy); // `from` limit

        if (period_remainder_bit_size_to_copy) {
            utility::memcpy_bitwise64(buf_out, to_bit_offset, buf_in, from_first_bit_offset, period_remainder_bit_size_to_copy);
        }
    }

    const size_t write_size = fwrite(buf_out, 1, inserted_stream_byte_size, file_out_handle.get());
    const int file_write_err = ferror(file_out_handle.get());
    if (write_size < size) {
        utility::debug_break();
#ifdef _UNICODE
        throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(file_out_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
        throw std::system_error{ file_write_err, std::system_category(), file_out_handle.path() };
#endif
    }
}

// Buffer be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
//
void generate_stream(GenData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    if (!!data.basic_data.options_ptr->stream_byte_size) {
        state.break_ = true;
    }

    // Buffer is already padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.stream_params.padded_stream_byte_size;

    // zeroing padding bytes
    for (uint32_t i = size; i < padded_stream_byte_size; i++) {
        buf[i] = 0;
    }

    if (data.basic_data.options_ptr->gen_input_noise_bit_block_size) {
        StreamParams stream_params{ data.stream_params };
        NoiseParams noise_params{ data.noise_params };
        generate_noise(data.basic_data, stream_params, noise_params, buf, size); // CAUTION: params copy must be copied!
    }

    // tee preprocessed input
    if (data.basic_data.tee_file_in_handle.get()) {
        const size_t write_size = fwrite(buf, 1, size, data.basic_data.tee_file_in_handle.get());
        const int file_write_err = ferror(data.basic_data.tee_file_in_handle.get());
        if (write_size < size) {
            utility::debug_break();
#ifdef _UNICODE
            throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(data.basic_data.tee_file_in_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
            throw std::system_error{ file_write_err, std::system_category(), data.basic_data.tee_file_in_handle.path() };
#endif
        }
    }

    utility::Buffer write_buf{ padded_stream_byte_size };

    uint8_t * buf_out = write_buf.get();

    const uint32_t bits_per_baud = data.basic_data.options_ptr->bits_per_baud;

    const uint64_t stream_bit_size = size * 8;

    const uint32_t num_wholes = uint32_t(stream_bit_size / bits_per_baud);
    //const uint32_t remainder = uint32_t(stream_bit_size % bits_per_baud);

    const uint64_t stream_bit_start_offset = data.stream_params.last_bit_offset;

    uint64_t stream_bit_offset = stream_bit_start_offset;

    for (uint32_t i = 0; i < num_wholes; i++)
    {
        const uint64_t byte_offset = stream_bit_offset / 8;
        const uint32_t remain_bit_offset = (stream_bit_offset % 8) + data.shifted_bit_offset;

        uint32_t buf32 = *(uint32_t *)(buf + byte_offset);

        const uint8_t baud_char = uint8_t(buf32 >> remain_bit_offset) & data.baud_mask;
        uint8_t to_baud_char = baud_char;

        for (uint32_t i = 0; i < data.baud_capacity; i++) {
            if ((*data.baud_alphabet_start_sequence)[i] == baud_char) {
                to_baud_char = (*data.baud_alphabet_end_sequence)[i];
                break;
            }
        }

        uint32_t & buf32_out = *(uint32_t *)(buf_out + byte_offset);

        buf32_out = (buf32_out & ~(uint32_t(data.baud_mask) << remain_bit_offset)) | (uint32_t(to_baud_char) << remain_bit_offset);

        stream_bit_offset += bits_per_baud;
    }

    data.stream_params.last_bit_offset = stream_bit_offset;

    uint64_t syncseq_first_offset = 0;

    if (g_options.insert_output_syncseq_period) {
        syncseq_first_offset = data.basic_data.options_ptr->insert_output_syncseq_first_offset;
        const uint32_t syncseq_end_offset = data.basic_data.options_ptr->insert_output_syncseq_end_offset;
        const uint32_t syncseq_period = data.basic_data.options_ptr->insert_output_syncseq_period;
        const uint32_t syncseq_period_repeat = data.basic_data.options_ptr->insert_output_syncseq_period_repeat;

        if (syncseq_first_offset >= stream_bit_start_offset) {
            syncseq_first_offset -= stream_bit_start_offset;
        }
        else {
            syncseq_first_offset = syncseq_period - (stream_bit_start_offset - syncseq_first_offset) % syncseq_period;
        }

        if (syncseq_first_offset < stream_bit_size) {
            write_syncseq(data.file_out_handle, buf_out, size,
                data.basic_data.options_ptr->syncseq_int32,
                data.basic_data.options_ptr->syncseq_bit_size,
                uint32_t(syncseq_first_offset), syncseq_end_offset,
                syncseq_period, syncseq_period_repeat,
                data.basic_data.flags_ptr->insert_output_syncseq_instead_fill);
        }
    }

    if (!g_options.insert_output_syncseq_period || syncseq_first_offset >= stream_bit_size) {
        const size_t write_size = fwrite(buf_out, 1, size, data.file_out_handle.get());
        const int file_write_err = ferror(data.file_out_handle.get());

        if (write_size < size) {
            utility::debug_break();
#ifdef _UNICODE
            throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(data.file_out_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
            throw std::system_error{ file_write_err, std::system_category(), data.file_out_handle.path() };
#endif
        }
    }
}

// Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
//
inline void pipe_stream(PipeData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    if (!!data.basic_data.options_ptr->stream_byte_size) {
        state.break_ = true;
    }

    const uint64_t stream_bit_size = size * 8;

    const uint64_t stream_bit_start_offset = data.stream_params.last_bit_offset;

    // Buffer is already padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.stream_params.padded_stream_byte_size;

    // zeroing padding bytes
    for (uint32_t i = size; i < padded_stream_byte_size; i++) {
        buf[i] = 0;
    }

    if (data.basic_data.options_ptr->gen_input_noise_bit_block_size) {
        generate_noise(data.basic_data, data.stream_params, data.noise_params, buf, size);
    }

    // tee preprocessed input
    if (data.basic_data.tee_file_in_handle.get()) {
        const size_t write_size = fwrite(buf, 1, size, data.basic_data.tee_file_in_handle.get());
        const int file_write_err = ferror(data.basic_data.tee_file_in_handle.get());
        if (write_size < size) {
            utility::debug_break();
#ifdef _UNICODE
            throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(data.basic_data.tee_file_in_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
            throw std::system_error{ file_write_err, std::system_category(), data.basic_data.tee_file_in_handle.path() };
#endif
        }
    }

    uint64_t syncseq_first_offset = 0;

    if (g_options.insert_output_syncseq_period) {
        syncseq_first_offset = data.basic_data.options_ptr->insert_output_syncseq_first_offset;
        const uint32_t syncseq_end_offset = data.basic_data.options_ptr->insert_output_syncseq_end_offset;
        const uint32_t syncseq_period = data.basic_data.options_ptr->insert_output_syncseq_period;
        const uint32_t syncseq_period_repeat = data.basic_data.options_ptr->insert_output_syncseq_period_repeat;

        if (syncseq_first_offset >= stream_bit_start_offset) {
            syncseq_first_offset -= stream_bit_start_offset;
        }
        else {
            syncseq_first_offset = syncseq_period - (stream_bit_start_offset - syncseq_first_offset) % syncseq_period;
        }

        if (syncseq_first_offset < stream_bit_size) {
            write_syncseq(data.file_out_handle, buf, size,
                data.basic_data.options_ptr->syncseq_int32,
                data.basic_data.options_ptr->syncseq_bit_size,
                uint32_t(syncseq_first_offset), syncseq_end_offset,
                syncseq_period, syncseq_period_repeat,
                data.basic_data.flags_ptr->insert_output_syncseq_instead_fill);
        }
    }

    if (!g_options.insert_output_syncseq_period || syncseq_first_offset >= stream_bit_size) {
        const size_t write_size = fwrite(buf, 1, size, data.file_out_handle.get());
        const int file_write_err = ferror(data.file_out_handle.get());
        if (write_size < size) {
            utility::debug_break();
#ifdef _UNICODE
            throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(data.file_out_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
            throw std::system_error{ file_write_err, std::system_category(), data.file_out_handle.path() };
#endif
        }
    }
}

// Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
//
void search_synchro_sequence(SyncData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    // not found
    data.syncseq_bit_offset = math::uint32_max;

    // first time read size must be greater than 32 bits
    //
    if (!state.read_index && 32 >= size) {
        return;
    }

    // single read only
    state.break_ = true;

    // Search synchro sequence in `write_buf` by sequential 32-bit blocks compare as shifted 64-bit blocks.
    // Formula for all comparisons (K) of N 32-bit blocks in a stream:
    //  If N-even => K = ((N + 1) * N / 2 - 1) * 32
    //  If N-odd  => K = ((N + 1) / 2 * N - 1) * 32

    // Buffer is already padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.stream_params.padded_stream_byte_size;

    // zeroing padding bytes
    for (uint32_t i = size; i < padded_stream_byte_size; i++) {
        buf[i] = 0;
    }

    if (data.basic_data.options_ptr->gen_input_noise_bit_block_size) {
        StreamParams stream_params{ data.stream_params };
        NoiseParams noise_params{ data.noise_params };
        generate_noise(data.basic_data, stream_params, noise_params, buf, size); // CAUTION: params copy must be copied!
    }

    // tee preprocessed input
    if (data.basic_data.tee_file_in_handle.get()) {
        const size_t write_size = fwrite(buf, 1, size, data.basic_data.tee_file_in_handle.get());
        const int file_write_err = ferror(data.basic_data.tee_file_in_handle.get());
        if (write_size < size) {
            utility::debug_break();
#ifdef _UNICODE
            throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(data.basic_data.tee_file_in_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
            throw std::system_error{ file_write_err, std::system_category(), data.basic_data.tee_file_in_handle.path() };
#endif
        }
    }

    // calculate synchro sequence autocorrelation values and autocorrelation mean values

    std::vector<float> autocorr_values_arr;
    std::deque<SyncseqAutocorr> autocorr_max_mean_deq;

    calculate_syncseq_autocorrelation(
        data.autocorr_in_params, data.autocorr_io_params,
        buf,
        autocorr_values_arr,
        !g_flags.disable_calc_autocorr_mean ? &autocorr_max_mean_deq : nullptr);

    if (!g_flags.disable_calc_autocorr_mean) {
        if (!autocorr_max_mean_deq.empty()) {
            for (const auto & autocorr_max_mean_ref : autocorr_max_mean_deq) {
                // use only first periodic value
                if (autocorr_max_mean_ref.period) {
                    data.syncseq_bit_offset = autocorr_max_mean_ref.offset;
                    data.stream_params.stream_width = autocorr_max_mean_ref.period;
                    data.autocorr_io_params.used_corr_mean = autocorr_max_mean_ref.corr_mean;
                    data.autocorr_io_params.period_used_repeat = autocorr_max_mean_ref.num_corr - 1;
                    break;
                }
            }
        }
        else {
            data.stream_params.stream_width = math::uint32_max; // print other calculated values
        }

        float max_corr_value = 0;

        for (size_t i = 0; i < autocorr_values_arr.size(); i++) {
            const float corr_value = autocorr_values_arr[i];

            if (max_corr_value < corr_value) {
                max_corr_value = corr_value;
            }
        }

        data.autocorr_io_params.max_corr_value = max_corr_value;
    }
    else if (autocorr_values_arr.size()) {
        data.stream_params.stream_width = math::uint32_max; // print other calculated values

        // search first maximum correlation value
        uint64_t syncseq_bit_offset = math::uint64_max;

        float used_corr_value = 0;
        float max_corr_value = 0;

        for (size_t i = 0; i < autocorr_values_arr.size(); i++) {
            const float corr_value = autocorr_values_arr[i];

            if (max_corr_value < corr_value) {
                max_corr_value = corr_value;
                syncseq_bit_offset = i;
                used_corr_value = corr_value;
            }
        }

        if (syncseq_bit_offset != math::uint64_max) {
            if (max_corr_value >= g_options.autocorr_min) {
                data.syncseq_bit_offset = syncseq_bit_offset;
                data.autocorr_io_params.used_corr_value = used_corr_value;
            }
        }
    }

#ifdef _DEBUG // DO NOT REMOVE: search algorithm false positive calculation code to test algorithm stability within input noise

    // Example of a command line to run a test:
    //
    //  `bitsync.exe /autocorr-min 0.81 /inn <bit-block-size> <probability-per-block> /tee-input test_input_w_noise.bin /spmin <stream-min-period> /spmax <stream-max-period> /s <stream-byte-size> /q <syncseq-bit-size> /r <syncseq-repeat-value> /k <syncseq-hex-value> sync <bits-per-baud> test_input.bin .`
    //
    //  For example, if syncseq-bit-size=20, then `bit-block-size` can be half of the synchro sequence - `10` with probability-per-block=100.
    //  That means the noise generator would generate from 1 to 3 of inversed bits per each synchro sequence in a bit stream.
    //

    // Example of expressions for the watch window in a debugger to represent statistical validity and certainty of the
    // correlation values output without mean search (first phase of the algorithm before the `autocorr_max_mean_deq` calculation):
    //
    //  true_num                                    // quantity of found true positions, just for a self test
    //
    //  &true_max_corr_arr[0],11                    // true position correlation values, sorted from minimum to maximum
    //
    //  &false_max_corr_arr[0],3                    // false positive correlation, sorted from maximum to minimum
    //
    //  &false_in_true_max_index_arr[0],30          // false positive correlation values within true positions, true positions is zeroed for convenience, sorted from maximum to minimum
    //
    //  true_max_corr_arr[0]-false_max_corr_arr[0]  // Correlation values false positive stability factor (spread) in range [-1; +1],
    //                                              //   difference between the lowest true position correlation value and the highest false positive correlation value,
    //                                              //   lower is worser (less stable), higher is better (more stable).
    //                                              //

    // Example of expressions for the watch window in a debugger to represent statistical validity and certainty of the
    // correlation mean values output (second phase of the algorithm with the `autocorr_max_mean_deq` calculation):
    //
    //  &autocorr_max_mean_deq[0],30                // Correlation mean values together with particular number of correlation values used to calculate a mean value and
    //                                              // bit stream offset and period, sorted from correlation mean maximum value to minimum.
    //
    //  &false_in_true_max_corr_mean_arr[0],30      // false positive correlation values within true positions, sorted from correlation mean maximum value to minimum
    //

    size_t true_num;

    // all true known positions (offsets) of the synchro sequence in the current input
    //
    std::vector<uint32_t> true_positions_index_arr = {
        907,
        3871,
        6835,
        9799,
        12763,
        15727,
        18691,
        21655,
        24619,
        27583,
        30547,
    };

    std::vector<float> false_max_corr_arr;
    std::vector<size_t> false_max_index_arr;
    std::vector<float> true_max_corr_arr;
    std::vector<size_t> true_max_index_arr;
    std::vector<float> false_in_true_max_corr_arr;
    std::vector<size_t> false_in_true_max_index_arr;
    std::vector<float> saved_true_in_false_max_corr_arr;
    std::vector<size_t> saved_true_in_false_max_index_arr;
    std::vector<SyncseqAutocorrStats> false_in_true_max_corr_mean_arr;

    calculate_syncseq_autocorrelation_false_positive_stats(
        autocorr_values_arr, !g_flags.disable_calc_autocorr_mean ? &autocorr_max_mean_deq : nullptr,
        true_positions_index_arr,
        true_num, 30,
        false_max_corr_arr, false_max_index_arr, true_max_corr_arr, true_max_index_arr,
        false_in_true_max_corr_arr, false_in_true_max_index_arr,
        saved_true_in_false_max_corr_arr, saved_true_in_false_max_index_arr,
        !g_flags.disable_calc_autocorr_mean ? &false_in_true_max_corr_mean_arr : nullptr);

    Sleep(0); // DO NOT REMOVE: for a debugger break point
#endif
}

void read_file_chunk(uint8_t * buf, uint64_t size, void * user_data, tackle::file_reader_state & state)
{
    if (sizeof(size_t) < sizeof(uint64_t)) {
        const uint64_t max_value = uint64_t(math::size_max);
        if (size > max_value) {
            throw std::runtime_error(fmt::format("size is out of buffer: size={:d}", size));
        }
    }

    ReadFileChunkData & read_file_chunk_data = *static_cast<ReadFileChunkData *>(user_data);

    switch (read_file_chunk_data.mode)
    {
    case Mode_Gen:
    {
        GenData & data = *static_cast<GenData *>(read_file_chunk_data.mode_data);

        const size_t read_size = uint32_t(size);

        generate_stream(data, state, buf, read_size);
    } break;

    case Mode_Sync:
    {
        SyncData & data = *static_cast<SyncData *>(read_file_chunk_data.mode_data);

        const size_t read_size = uint32_t(size);

        search_synchro_sequence(data, state, buf, read_size);
    } break;

    case Mode_Pipe:
    {
        PipeData & data = *static_cast<PipeData *>(read_file_chunk_data.mode_data);

        const size_t read_size = uint32_t(size);

        pipe_stream(data, state, buf, read_size);
    } break;
    }
}
