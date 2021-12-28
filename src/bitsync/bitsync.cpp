#include "bitsync.hpp"

#include <algorithm>

#include <stdlib.h>
#include <time.h>


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
    stream_byte_size                = 0;
    syncseq_bit_size                = 0;
    syncseq_int32                   = 0;
    syncseq_repeat                  = 0;
    bits_per_baud                   = 0;
    gen_input_noise_bit_block_size  = 0;
    gen_input_noise_block_bit_prob  = 0;
}


void Options::clear()
{
    *this = Options{};
}

inline void translate_buffer(PipeData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    if (!!data.options_ptr->stream_byte_size) {
        state.break_ = true;
    }

    // Buffer size must be padded to 3 bytes reminder to be able to read and shift
    // the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.padded_stream_byte_size;

    if (padded_stream_byte_size > size) {
        for (uint32_t i = 0; i < padded_stream_byte_size - size; i++) {
            buf[size + i] = 0; // zeroing padding bytes
        }
    }

    uint8_t * buf_out = buf;

    uint64_t stream_bit_offset = data.last_bit_offset;

    // generate input noise
    const uint32_t gen_input_noise_bit_block_size = data.options_ptr->gen_input_noise_bit_block_size;

    if (gen_input_noise_bit_block_size) {
        uint64_t buf_bit_offset = 0;
        uint64_t buf_remain_bit_size = size * 8;

        const uint32_t gen_input_noise_block_bit_prob = data.options_ptr->gen_input_noise_block_bit_prob;

        uint64_t last_gen_noise_bits_block_index = data.last_gen_noise_bits_block_index;

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
                const uint64_t remain_bit_size_to_skip = (last_gen_noise_bits_block_index - gen_input_noise_bits_block_index) * gen_input_noise_bit_block_size;

                if (buf_remain_bit_size >= remain_bit_size_to_skip) {
                    gen_input_noise_bits_block_index += remain_bit_size_to_skip / gen_input_noise_bit_block_size;
                    buf_remain_bit_size -= remain_bit_size_to_skip;
                    stream_bit_offset += remain_block_bit_size;
                    buf_bit_offset += remain_block_bit_size;
                }
                else {
                    stream_bit_offset += buf_remain_bit_size;
                    buf_bit_offset += buf_remain_bit_size;
                    buf_remain_bit_size = 0;
                }
            }
        }

        // recalculate last bits block
        if (buf_remain_bit_size) {
            gen_input_noise_bits_block_index = stream_bit_offset / gen_input_noise_bit_block_size;
            const uint64_t gen_noise_block_bit_offset = stream_bit_offset % gen_input_noise_bit_block_size;

            remain_block_bit_size = uint32_t(gen_input_noise_bit_block_size - gen_noise_block_bit_offset);

            srand(time(NULL));
        }

        // process remaining bits block from the previous run as first bits block
        while(buf_remain_bit_size) {
            uint32_t remain_block_bit_prob = 100;

            if (gen_input_noise_block_bit_prob < 100) {
                const int remain_block_bit_prob_rand = rand();

                // rand normalization to 1-100 range
                remain_block_bit_prob = uint32_t(100) * remain_block_bit_prob_rand / (RAND_MAX + 1) + 1;
            }

            if (gen_input_noise_block_bit_prob >= remain_block_bit_prob) {
                const int remain_block_bit_offset_rand = rand();

                // rand normalization to 0-`remain_block_bit_size-1`
                const uint32_t remain_block_bit_offset = uint32_t(uint64_t(remain_block_bit_size) * remain_block_bit_offset_rand / (RAND_MAX + 1));

                if (buf_remain_bit_size >= remain_block_bit_size || remain_block_bit_offset < buf_remain_bit_size) {
                    // bit inversion
                    const uint64_t byte_offset = (stream_bit_offset + remain_block_bit_offset) / 8;
                    const uint32_t remain_bit_offset = (stream_bit_offset + remain_block_bit_offset) % 8;

                    uint32_t & buf32_out = *(uint32_t *)(buf_out + byte_offset);

                    buf32_out ^= (uint32_t(0x01) << remain_bit_offset);

                    gen_input_noise_bits_block_index++;
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

        data.last_bit_offset = stream_bit_offset;

        data.last_gen_noise_bits_block_index = last_gen_noise_bits_block_index;
    }

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

void translate_buffer(GenData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    if (!!data.options_ptr->stream_byte_size) {
        state.break_ = true;
    }

    // Buffer size must be padded to 3 bytes reminder to be able to read and shift
    // the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.padded_stream_byte_size;

    if (padded_stream_byte_size > size) {
        for (uint32_t i = 0; i < padded_stream_byte_size - size; i++) {
            buf[size + i] = 0; // zeroing padding bytes
        }
    }

    utility::Buffer write_buf{ padded_stream_byte_size };

    uint8_t * buf_out = write_buf.get();

    const uint32_t bits_per_baud = data.options_ptr->bits_per_baud;

    const uint32_t num_wholes = (size * 8) / bits_per_baud;
    //const uint32_t reminder = (size * 8) % bits_per_baud;

    uint64_t stream_bit_offset = data.last_bit_offset;

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

    data.last_bit_offset = stream_bit_offset;

    const size_t write_size = fwrite(buf_out, 1, size, data.file_out_handle.get());
    const int file_write_err = ferror(data.file_out_handle.get());
    if (write_size < size) {
        utility::debug_break();
#ifdef _UNICODE
        throw std::system_error{ file_write_err, std::system_category(), utility::convert_string_to_string(data.file_out_handle.path(), utility::tag_string{}, utility::int_identity<utility::StringConv_utf16_to_utf8>{}) };
#else
        throw std::system_error{ file_write_err, std::system_category(), data.file_out_handle.path()};
#endif
    }
}

void search_synchro_sequence(SyncData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
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

    // Buffer size must be padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift
    // the last 32-bit block as 64-bit block.
    //
    const uint32_t padded_uint32_size = (size + 3) & ~uint32_t(3);
    const uint32_t padded_stream_byte_size = data.padded_stream_byte_size;

    uint32_t * buf32 = (uint32_t *)buf;

    const uint32_t num_uint32_blocks = padded_uint32_size / 4;

    if (padded_stream_byte_size > size) {
        for (uint32_t i = 0; i < padded_stream_byte_size - size; i++) {
            buf[size + i] = 0; // zeroing padding bytes
        }
    }

    std::vector<SyncSeqHits> sync_seq_hits;

    const uint32_t syncseq_bit_size = data.options_ptr->syncseq_bit_size;
    const uint32_t syncseq_int32 = data.options_ptr->syncseq_int32;
    const uint32_t syncseq_repeat = data.options_ptr->syncseq_repeat;

    // guess
    sync_seq_hits.reserve(data.padded_stream_byte_size / syncseq_bit_size / 2);

    const uint32_t syncseq_bytes = syncseq_int32 & data.syncseq_mask;

    if (syncseq_int32) {
        // linear search
        for (uint32_t from = 0; from < num_uint32_blocks; from++) {
            const uint64_t from64 = *(uint64_t *)(buf32 + from);

            if (!from64) {
                continue;
            }

            for (uint32_t i = 0; i < 32; i++) {
                const uint32_t from_shifted = uint32_t(from64 >> i) & data.syncseq_mask;

                if (!from_shifted) {
                    continue;
                }

                const uint32_t first_bit_offset = from * 4 * 8 + i;

                if (!(from_shifted ^ syncseq_bytes)) {
                    if (sync_seq_hits.size()) {
                        auto & last_sync_seq_hits = sync_seq_hits.back();
                        last_sync_seq_hits.next_bit_offsets.push_back(first_bit_offset);
                    }
                    else {
                        sync_seq_hits.push_back(SyncSeqHits{ first_bit_offset, { first_bit_offset } });
                    }
                }
            }
        }
    }
    else {
        // quadratic search
        for (uint32_t from = 0; from < num_uint32_blocks; from++) {
            const uint64_t from64 = *(uint64_t *)(buf32 + from);

            if (!from64) {
                continue;
            }

            for (uint32_t i = 0; i < 32; i++) {
                const uint32_t from_shifted = uint32_t(from64 >> i) & data.syncseq_mask;

                if (!from_shifted) {
                    continue;
                }

                const uint32_t first_bit_offset = from * 4 * 8 + i;

                for (uint32_t to = from + 1; to < num_uint32_blocks; to++) {
                    const uint64_t to64 = *(uint64_t *)(buf32 + to);

                    if (!to64) {
                        continue;
                    }

                    for (uint32_t j = 0; j < 32; j++) {
                        const uint32_t to_shifted = uint32_t(to64 >> j) & data.syncseq_mask;

                        if (!to_shifted) {
                            continue;
                        }

                        const uint32_t next_bit_offset = to * 4 * 8 + j;

                        if (!(from_shifted ^ to_shifted)) {
                            if (sync_seq_hits.size()) {
                                auto & last_sync_seq_hits = sync_seq_hits.back();
                                if (last_sync_seq_hits.first_bit_offset != first_bit_offset) {
                                    sync_seq_hits.push_back(SyncSeqHits{ first_bit_offset, { next_bit_offset } });
                                }
                                else {
                                    last_sync_seq_hits.next_bit_offsets.push_back(next_bit_offset);
                                }
                            }
                            else {
                                sync_seq_hits.push_back(SyncSeqHits{ first_bit_offset, { next_bit_offset } });
                            }
                        }
                    }
                }

                if (sync_seq_hits.size()) {
                    auto & last_sync_seq_hits = sync_seq_hits.back();
                    if (last_sync_seq_hits.next_bit_offsets.size() < syncseq_repeat) {
                        sync_seq_hits.pop_back();
                    }
                }
            }
        }
    }

    // count hits
    size_t hits = 0;
    std::vector<uint32_t> next_bit_offsets;

    for (const auto & first_bit_hits : sync_seq_hits) {
        next_bit_offsets.clear();
        next_bit_offsets.reserve(first_bit_hits.next_bit_offsets.size());

        for (uint32_t next_bit_offset : first_bit_hits.next_bit_offsets) {
            next_bit_offsets.push_back(next_bit_offset - first_bit_hits.first_bit_offset);
        }

        // search repeats
        uint32_t prev_next_bit_offset = next_bit_offsets[0];

        const size_t num_next_bit_offsets = next_bit_offsets.size();
        size_t j = 1;
        for (size_t i = j; i < num_next_bit_offsets; i++) {
            const uint32_t next_next_bit_offset = next_bit_offsets[i];
            if (prev_next_bit_offset / j == next_next_bit_offset / (j + 1)) {
                hits++;
                if (hits >= syncseq_repeat) {
                    // found
                    data.syncseq_bit_offset = first_bit_hits.first_bit_offset;
                    return;
                }
            }
            else {
                hits = 0; // reset
            }

            if (prev_next_bit_offset) {
                j++;
            }

            prev_next_bit_offset = next_next_bit_offset;
        }
    }
}

void read_file_chunk(uint8_t * buf, uint64_t size, void * user_data, tackle::file_reader_state & state)
{
    if (sizeof(size_t) < sizeof(uint64_t)) {
        const uint64_t max_value = uint64_t(math::size_max);
        if (size > max_value) {
            throw std::runtime_error(fmt::format("size is out of buffer: size={:d}", size));
        }
    }

    BasicData & basic_data = *static_cast<BasicData *>(user_data);

    switch (basic_data.mode)
    {
    case Mode_Gen:
    {
        GenData & data = *static_cast<GenData *>(user_data);

        const size_t read_size = uint32_t(size);

        translate_buffer(data, state, buf, read_size);
    } break;

    case Mode_Sync:
    {
        SyncData & data = *static_cast<SyncData *>(user_data);

        const size_t read_size = uint32_t(size);

        search_synchro_sequence(data, state, buf, read_size);
    } break;

    case Mode_Pipe:
    {
        PipeData & data = *static_cast<PipeData *>(user_data);

        const size_t read_size = uint32_t(size);

        translate_buffer(data, state, buf, read_size);
    } break;
    }
}
