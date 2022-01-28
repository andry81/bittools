// Author: Andrey Dibrov (andry at inbox dot ru)
//

#include "bitsync.hpp"

#include <algorithm>

#include <stdlib.h>
#include <time.h>

//#include <fftw3.h>


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
    stream_byte_size                = 0;
    stream_min_period               = math::uint32_max;
    stream_max_period               = math::uint32_max;
    syncseq_bit_size                = 0;
    syncseq_int32                   = 0;
    syncseq_min_repeat              = math::uint32_max;  // by default max repeats based on the stream length
    syncseq_max_repeat              = math::uint32_max;
    bits_per_baud                   = 0;
    gen_input_noise_bit_block_size  = 0;
    gen_input_noise_block_bit_prob  = 0;
    autocorr_mean_min               = 0;
    autocorr_mean_buf_max_size_mb   = 400; // 400 Mb is default
}


void Options::clear()
{
    *this = Options{};
}

inline void generate_noise(const BasicData & basic_data, StreamParams & stream_params, NoiseParams & noise_params, uint8_t * buf, uint32_t size)
{
    uint8_t * buf_out = buf;

    uint64_t stream_bit_offset = stream_params.last_bit_offset;

    // generate input noise
    const uint32_t gen_input_noise_bit_block_size = basic_data.options_ptr->gen_input_noise_bit_block_size;

    if (gen_input_noise_bit_block_size) {
        uint64_t buf_bit_offset = 0;
        uint64_t buf_remain_bit_size = size * 8;

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

void generate_stream(GenData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    if (!!data.basic_data.options_ptr->stream_byte_size) {
        state.break_ = true;
    }

    // Buffer size must be padded to 3 bytes remainder to be able to read and shift
    // the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.stream_params.padded_stream_byte_size;

    if (padded_stream_byte_size > size) {
        for (uint32_t i = 0; i < padded_stream_byte_size - size; i++) {
            buf[size + i] = 0; // zeroing padding bytes
        }
    }

    if (data.basic_data.options_ptr->gen_input_noise_bit_block_size) {
        StreamParams stream_params{ data.stream_params };
        NoiseParams noise_params{ data.noise_params };
        generate_noise(data.basic_data, stream_params, noise_params, buf, size); // CAUTION: params copy must be copied!
    }

    // write ready to process input into tee at first
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

    const uint32_t num_wholes = (size * 8) / bits_per_baud;
    //const uint32_t remainder = (size * 8) % bits_per_baud;

    uint64_t stream_bit_offset = data.stream_params.last_bit_offset;

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

inline void pipe_stream(PipeData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size)
{
    if (!!data.basic_data.options_ptr->stream_byte_size) {
        state.break_ = true;
    }

    // Buffer size must be padded to 3 bytes remainder to be able to read and shift
    // the last 8-bit block as 32-bit block.
    //
    const uint32_t padded_stream_byte_size = data.stream_params.padded_stream_byte_size;

    if (padded_stream_byte_size > size) {
        for (uint32_t i = 0; i < padded_stream_byte_size - size; i++) {
            buf[size + i] = 0; // zeroing padding bytes
        }
    }

    if (data.basic_data.options_ptr->gen_input_noise_bit_block_size) {
        generate_noise(data.basic_data, data.stream_params, data.noise_params, buf, size);
    }

    // write ready to process input into tee at first
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

    // Buffer size must be padded to a multiple of 4 bytes plus 4 bytes remainder to be able to read and shift
    // the last 32-bit block as 64-bit block.
    //
    const uint32_t padded_uint32_size = (size + 3) & ~uint32_t(3);
    const uint32_t padded_stream_byte_size = data.stream_params.padded_stream_byte_size;

    uint32_t * buf32 = (uint32_t *)buf;

    const uint32_t num_uint32_blocks = padded_uint32_size / 4;

    if (padded_stream_byte_size > size) {
        for (uint32_t i = 0; i < padded_stream_byte_size - size; i++) {
            buf[size + i] = 0; // zeroing padding bytes
        }
    }

    if (data.basic_data.options_ptr->gen_input_noise_bit_block_size) {
        StreamParams stream_params{ data.stream_params };
        NoiseParams noise_params{ data.noise_params };
        generate_noise(data.basic_data, stream_params, noise_params, buf, size); // CAUTION: params copy must be copied!
    }

    // write ready to process input into tee at first
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

    const uint32_t syncseq_bit_size = data.basic_data.options_ptr->syncseq_bit_size;
    assert(syncseq_bit_size > 1);

    //const uint32_t syncseq_half_bit_size = (syncseq_bit_size + 1) / 2; // math expected value of synchro sequence xor with random bit stream
    const uint32_t syncseq_int32 = data.basic_data.options_ptr->syncseq_int32;
    const uint32_t syncseq_min_repeat = data.basic_data.options_ptr->syncseq_min_repeat != math::uint32_max ? data.basic_data.options_ptr->syncseq_min_repeat : 0;
    const uint32_t syncseq_max_repeat = data.basic_data.options_ptr->syncseq_max_repeat;
    const uint32_t syncseq_bytes = syncseq_int32 & data.syncseq_mask;
    //const uint32_t syncseq_num_bits = math::count_bits(syncseq_bytes);

    const uint64_t stream_bit_size = size * 8;

    bool break_ = false;

    std::vector<uint32_t> stream_bits_block_delta_arr((size_t(stream_bit_size)));

    uint64_t stream_bit_offset = 0;

    // linear search
    for (uint32_t from = 0; from < num_uint32_blocks; from++) {
        const uint64_t from64 = *(uint64_t *)(buf32 + from);

        for (uint32_t i = 0; i < 32; i++) {
            const uint32_t from_shifted = uint32_t(from64 >> i) & data.syncseq_mask;

            stream_bits_block_delta_arr[size_t(stream_bit_offset)] = from_shifted;

            stream_bit_offset++;

            if (stream_bit_offset >= stream_bit_size) {
                break_ = true;
                break;
            }
        }

        if (break_) break;
    }

    // calculate autocorrelation values and autocorrelation mean values

    std::vector<float> autocorr_values_arr;
    std::deque<SyncseqAutocorr> autocorr_mean_deq;

    calculate_autocorrelation(
        stream_bit_size, g_options.stream_min_period, g_options.stream_max_period,
        syncseq_bit_size, syncseq_bytes, syncseq_min_repeat, syncseq_max_repeat,
        g_options.autocorr_mean_buf_max_size_mb * 1024 * 1024,
        stream_bits_block_delta_arr, autocorr_values_arr,
        !g_flags.disable_calc_autocorr_mean ? &autocorr_mean_deq : nullptr);

    if (!g_flags.disable_calc_autocorr_mean) {
        // sort from the correlation mean values from maximum to the minimum
        std::sort(autocorr_mean_deq.begin(), autocorr_mean_deq.end(), [&](const SyncseqAutocorr & l, const SyncseqAutocorr & r) -> bool
        {
            return l.corr_mean > r.corr_mean;
        });

        // search first maximum correlation mean value with declared repeats in a stream
        uint64_t syncseq_bit_offset = math::uint64_max;
        uint32_t syncseq_period = math::uint32_max;
        float autocorr_mean = 0;

        for (auto autocorr_mean_ref : autocorr_mean_deq) {
            if (autocorr_mean_ref.num_corr < 1 || !autocorr_mean_ref.period) { // just in case
                assert(0);
                continue;
            }

            if (autocorr_mean_ref.corr_mean < g_options.autocorr_mean_min) {
                break;
            }

            if (autocorr_mean_ref.num_corr >= syncseq_min_repeat + 1) {
                syncseq_bit_offset = autocorr_mean_ref.offset;
                syncseq_period = autocorr_mean_ref.period;
                autocorr_mean = autocorr_mean_ref.corr_mean;
                break;
            }
        }

        if (syncseq_bit_offset != math::uint64_max) {
            data.syncseq_bit_offset = syncseq_bit_offset;
            data.stream_params.stream_width = syncseq_period;
            data.autocorr_params.mean = autocorr_mean;
        }
    }
    else if (autocorr_values_arr.size()) {
        // search first maximum correlation value
        uint64_t syncseq_bit_offset = math::uint64_max;
        float corr_max_value = 0;

        for (size_t i = 0; i < autocorr_values_arr.size(); i++) {
            const float corr_value = autocorr_values_arr[i];

            if (corr_max_value < corr_value) {
                corr_max_value = corr_value;
                syncseq_bit_offset = i;
            }
        }

        if (syncseq_bit_offset != math::uint64_max) {
            if (corr_max_value >= g_options.autocorr_mean_min) {
                data.syncseq_bit_offset = syncseq_bit_offset;
            }
            data.autocorr_params.value = corr_max_value;
        }
    }

#if 1 // DO NOT REMOVE: search algorithm false positive calculation code to test algorithm stability within input noise

    // Example of a command line to run a test:
    //
    //  `bitsync.exe /autocorr-mean-min 0.81 /inn <bit-block-size> <probability-per-block> /tee-input test_input_w_noise.bin /spmin <stream-min-period> /spmax <stream-max-period> /s <stream-byte-size> /q <syncseq-bit-size> /r <synseq-repeat-value> /k <synseq-hex-value> sync <bits-per-baud> test_input.bin .`
    //
    //  For example, if syncseq-bit-size=20, then `bit-block-size` can be half of the synchro sequence - `10` with probability-per-block=100.
    //  That means the noise generator would generate from 1 to 3 of inversed bits per each synchro sequence in a bit stream.
    //

    // Example of expressions for the watch window in a debugger to represent statistical validity and certainty of the
    // correlation values output without mean search (first phase of the algorithm before the `syncseq_autocorr_mean_deq` calculation):
    //
    //  true_num                                    // quantity of found true positions, just for a self test
    //
    //  &true_max_corr_arr[0],11                    // true position correlation values, sorted from minimum to maximum
    //
    //  &false_max_corr_arr[0],3                    // false positive correlation, sorted from maximum to minimum
    //
    //  &false_in_true_max_index_arr[0],30          // false positive correlation values within true positions, true positions is zeroed for convenience, sorted from maximum to minimum
    //
    //  true_max_corr_arr[0]-false_max_corr_arr[0]  // Correlation values false positive stability factor in range [-1; +1],
    //                                              //   difference between lowest true position correlation value and highest false positive correlation value,
    //                                              //   lower is worser (less stable), higher is better (more stable).
    //                                              //

    // Example of expressions for the watch window in a debugger to represent statistical validity and certainty of the
    // correlation mean values output (second phase of the algorithm with the `syncseq_autocorr_mean_deq` calculation):
    //
    //  &autocorr_mean_deq[0],30                    // Correlation mean values together with particular number of correlation values used to calculate the mean value and
    //                                              // bit stream offset and period, sorted from correlation mean maximum value to minimum.
    //
    //  &false_in_true_max_corr_mean_arr[0],30      // false positive correlation values within true positions, sorted from correlation mean maximum value to minimum
    //

    size_t true_num;

    std::vector<uint32_t> true_positions_index_arr = { // all true known positions for the particular input
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

    calculate_autocorrelation_false_positive_stats(
        autocorr_values_arr, !g_flags.disable_calc_autocorr_mean ? &autocorr_mean_deq : nullptr,
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
