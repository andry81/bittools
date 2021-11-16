#include "main.hpp"

#include "tacklelib/utility/utility.hpp"
#include "tacklelib/utility/assert.hpp"
#include "tacklelib/utility/math.hpp"

#include "tacklelib/tackle/file_reader.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>

#include <fmt/format.h>

#include <string>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

namespace po = boost::program_options;

namespace boost {
    namespace fs = filesystem;
}

namespace
{
    enum Mode
    {
        Mode_None           = 0,
        Mode_Gen            = 1,
        Mode_Sync           = 2,
    };

    struct BasicData
    {
        Mode                            mode;
    };

    struct GenData : BasicData
    {
        std::vector<uint8_t> *          baud_alphabet_start_sequence;
        std::vector<uint8_t> *          baud_alphabet_end_sequence;
        uint32_t                        bits_per_baud;
        uint32_t                        baud_mask;
        uint32_t                        baud_capacity;
        uint32_t                        last_bit_offset;
        tackle::file_handle<char>       file_out_handle;
    };

    struct SyncData : BasicData
    {
        uint32_t                        padded_stream_byte_size;
        uint32_t                        syncseq_bit_size;
        uint32_t                        syncseq_bytes;
        uint32_t                        syncseq_mask;
        uint32_t                        syncseq_repeat;
        uint32_t                        syncseq_bit_offset;
    };

    struct SyncSeqHits
    {
        uint32_t                first_bit_offset;
        std::vector<uint32_t>   next_bit_offsets;
    };

    void translate_buffer(GenData & data, uint8_t * buf, uint32_t size)
    {
        utility::Buffer write_buf{ size };

        uint8_t * buf_out = write_buf.get();

        uint32_t bit_offset = 0;

        const uint32_t num_wholes = (size * 8) / data.bits_per_baud;
        const uint32_t reminder = (size * 8) % data.bits_per_baud;

        for (uint32_t i = 0; i < num_wholes; i++)
        {
            const uint32_t byte_offset = bit_offset / 8;
            const uint32_t remain_bit_offset = bit_offset % 8;

            const uint8_t baud_char = (buf[byte_offset] >> remain_bit_offset) & data.baud_mask;
            uint8_t to_baud_char = baud_char;

            for (uint32_t i = 0; i < data.baud_capacity; i++) {
                if ((*data.baud_alphabet_start_sequence)[i] == baud_char) {
                    to_baud_char = (*data.baud_alphabet_end_sequence)[i];
                    break;
                }
            }

            const uint32_t remain_mask = (uint32_t(0x01) << remain_bit_offset) - 1;

            buf_out[byte_offset] = (buf_out[byte_offset] & remain_mask) | (to_baud_char << remain_bit_offset);

            bit_offset += data.bits_per_baud;
        }

        data.last_bit_offset += num_wholes * data.bits_per_baud;

        const size_t write_size = fwrite(buf_out, 1, size, data.file_out_handle.get());
        const int file_write_err = ferror(data.file_out_handle.get());
        if (write_size < size) {
            utility::debug_break();
            throw std::system_error{ file_write_err, std::system_category(), data.file_out_handle.path() };
        }
    }

    void search_synchro_sequence(SyncData & data, tackle::file_reader_state & state,  uint8_t * buf, uint32_t size)
    {
        // first time read size must be greater than 32 bits
        //
        if (!state.read_index && 32 >= size) {
            return;
        }

        // Search synchro sequence in `write_buf` by sequential 32-bit blocks compare as shifted 64-bit blocks.
        // Formula for all comparisons (K) of N 32-bit blocks in a stream:
        //  If N-even => K = ((N + 1) * N / 2 - 1) * 32
        //  If N-odd  => K = ((N + 1) / 2 * N - 1) * 32
        // Buffer size must be padded to a multiple of 8 bytes to be able to read and shift the last 64-bit block.
        //
        const uint32_t padded_uint64_size = data.padded_stream_byte_size;

        uint32_t * buf32 = (uint32_t *)buf;
        uint64_t * buf64 = (uint64_t *)buf;

        if (padded_uint64_size > size) {
            const uint32_t last_uint64_offset = (padded_uint64_size / 8) - 1;
            buf64[last_uint64_offset] &= ((uint64_t(0x1) << (padded_uint64_size - size)) - 1); // zeroing padding bytes
        }

        const uint32_t padded_uint32_size = (size + 3) & ~uint32_t(3);

        const uint32_t num_uint32_blocks = padded_uint32_size / 4;

        std::vector<SyncSeqHits> sync_seq_hits;

        // guess
        sync_seq_hits.reserve(data.padded_stream_byte_size / data.syncseq_bit_size / 2);

        const uint32_t syncseq_bytes = data.syncseq_bytes & data.syncseq_mask;

        if (data.syncseq_bytes) {
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
                        if (last_sync_seq_hits.next_bit_offsets.size() < data.syncseq_repeat) {
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
                    if (hits >= data.syncseq_repeat) {
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

    void _read_file_chunk(uint8_t * buf, uint64_t size, void * user_data, tackle::file_reader_state & state)
    {
        if (sizeof(size_t) < sizeof(uint64_t)) {
            const uint64_t max_value = uint64_t((std::numeric_limits<size_t>::max)());
            if (size > max_value) {
                throw std::runtime_error(
                    (boost::format(
                        BOOST_PP_CAT(__FUNCTION__, ": size is out of buffer: size=%llu")) %
                        size).str());
            }
        }

        BasicData & basic_data = *static_cast<BasicData *>(user_data);

        switch (basic_data.mode)
        {
        case Mode_Gen:
        {
            GenData & data = *static_cast<GenData *>(user_data);

            const size_t read_size = uint32_t(size);

            translate_buffer(data, buf, read_size);
        } break;

        case Mode_Sync:
        {
            SyncData & data = *static_cast<SyncData *>(user_data);

            const size_t read_size = uint32_t(size);

            search_synchro_sequence(data, state, buf, read_size);
        } break;
        }
   }
}

int main(int argc, char* argv[])
{
    int ret = 0;

    std::string mode_str;
    Mode mode = Mode_None;

    uint32_t bits_per_baud = 0;
    uint32_t stream_byte_size = 0;
    uint32_t syncseq_bit_size = 0;
    uint32_t syncseq_repeat = 0;

    uint32_t syncseq_bytes = 0;
    std::string syncseq_bytes_str;

    tackle::path_string in_file;
    tackle::path_string out_file_dir;

    try {
        po::options_description basic_desc("Basic options");
        basic_desc.add_options()
            ("help,h", "print usage message")

            ("mode",
                po::value(&mode_str)->required(), "mode: gen | sync")
            ("bits_per_baud",
                po::value(&bits_per_baud)->required(), "bits per baud in stream (must be <= 2)") // CAUTION: factorial => (2^3)! = 8! = 40320 ( (2^4)! = 16! = 20922789888000 - greater than 32 bits! )
            ("stream_byte_size,s",
                po::value(&stream_byte_size), "stream byte size to search for synchro sequence (must be greater than 32 bits)")
            ("syncseq_size,q",
                po::value(&syncseq_bit_size), "synchro sequence size in bits (must be less or equal to 32 bits)")
            ("syncseq_bytes,k",
                po::value(&syncseq_bytes_str), "synchro sequence bytes (must be not 0)")
            ("syncseq_repeat,r",
                po::value(&syncseq_repeat), "synchro sequence minimal repeat quantity")

            ("in_file,i",
                po::value(&in_file.str())->required(), "input file path")
            ("out_file_dir,d",
                po::value(&out_file_dir.str()), "output directory path for output files (current directory if not set)");

        po::options_description all_desc("Allowed options");
        all_desc.
            add(basic_desc);

        po::positional_options_description p;
        p.add("mode", 1);
        p.add("bits_per_baud", 1);
        p.add("in_file", 1);
        p.add("out_file_dir", 1);

        po::variables_map vm;

        po::store(po::command_line_parser(argc, argv).options(all_desc).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout <<
                basic_desc <<
                "\n";
            return 1;
        }

        po::notify(vm); // important, otherwise related option variables won't be initialized

        if (mode_str == "gen") {
            mode = Mode_Gen;
        }
        else if (mode_str == "sync") {
            mode = Mode_Sync;
        }

        if (mode == Mode_None) {
            fprintf(stderr, "error: mode is not known: mode=%s\n", mode_str.c_str());
            return 255;
        }

        if (!(bits_per_baud > 0 && bits_per_baud <= 2)) {
            fprintf(stderr, "error: bits per baud must be positive and not greater than 2: bits_per_baud=%u\n", bits_per_baud);
            return 255;
        }

        if (mode == Mode_Sync) {
            if (stream_byte_size <= 32) {
                fprintf(stderr, "error: stream_byte_size must be greater than 32 bits\n");
                return 255;
            }

            if (!syncseq_bit_size) {
                fprintf(stderr, "error: syncseq_bit_size must be positive\n");
                return 255;
            }

            if (32 < syncseq_bit_size) {
                fprintf(stderr, "error: syncseq_bit_size must be less or equal to 32 bits\n");
                return 255;
            }

            if (syncseq_bit_size < bits_per_baud) {
                fprintf(stderr, "error: syncseq_bit_size must be greater or equal to bits_per_baud\n");
                return 255;
            }

            if (!syncseq_bytes_str.empty()) {
                syncseq_bytes = utility::str_to_number<decltype(syncseq_bytes)>(syncseq_bytes_str);

                if (!syncseq_bytes) {
                    fprintf(stderr, "error: syncseq_bytes must not 0\n");
                    return 255;
                }
            }

            if (!syncseq_repeat) {
                fprintf(stderr, "error: syncseq_repeat must be positive\n");
                return 255;
            }
        }

        if (in_file.empty() || !utility::is_regular_file(in_file, false)) {
            fprintf(stderr, "error: input file is not found: \"%s\"\n", in_file.c_str());
            return 256;
        }

        if (out_file_dir.empty()) {
            out_file_dir = utility::get_current_path(false, tackle::tag_native_path_string{});
        }

        if (out_file_dir != "." && !utility::is_directory_path(out_file_dir, false)) {
            std::cerr <<
                (boost::format("error: %s: output directory path is not a directory: path=\"%s\"") %
                    UTILITY_PP_FUNC % out_file_dir).str() << "\n";
            return 255;
        }

        const uint32_t baud_capacity = (uint32_t(0x01) << bits_per_baud);
        const uint32_t baud_mask = baud_capacity - 1;

        std::vector<uint8_t> baud_alphabet_current_sequence;
        std::vector<std::vector<uint8_t> > baud_alphabet_sequences;

        // CAUTION: factorial => (2^3)! = 8! = 40320 ( (2^4)! = 16! = 20922789888000 - greater than 32 bits! )
        uint32_t num_baud_alphabet_sequences = 1;
        for (uint32_t i = 1; i <= baud_capacity; i++) {
            num_baud_alphabet_sequences *= i; 
        }

        baud_alphabet_current_sequence.resize(baud_capacity);
        baud_alphabet_sequences.reserve(num_baud_alphabet_sequences);

        uint32_t baud_alphabet_sequence_index = 0;

        for (uint32_t i0 = 0; i0 < baud_capacity; i0++) {
            baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i0;
            baud_alphabet_sequence_index++;

            for (uint32_t i1 = 0; i1 < baud_capacity; i1++) {
                bool next = false;

                for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                    if (baud_alphabet_current_sequence[j] == i1) {
                        next = true;
                        break;
                    }
                }

                if (next) {
                    continue;
                }

                baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i1;
                baud_alphabet_sequence_index++;

                if (baud_capacity >= 3)
                for (uint32_t i2 = 0; i2 < baud_capacity; i2++) {
                    bool next = false;

                    for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                        if (baud_alphabet_current_sequence[j] == i2) {
                            next = true;
                            break;
                        }
                    }

                    if (next) {
                        continue;
                    }

                    baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i2;
                    baud_alphabet_sequence_index++;

                    for (uint32_t i3 = 0; i3 < baud_capacity; i3++) {
                        bool next = false;

                        for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                            if (baud_alphabet_current_sequence[j] == i3) {
                                next = true;
                                break;
                            }
                        }

                        if (next) {
                            continue;
                        }

                        baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i3;
                        baud_alphabet_sequence_index++;

                        baud_alphabet_sequences.push_back(baud_alphabet_current_sequence);

                        baud_alphabet_sequence_index--;
                    }

                    baud_alphabet_sequence_index--;
                }
                else baud_alphabet_sequences.push_back(baud_alphabet_current_sequence);

                baud_alphabet_sequence_index--;
            }

            baud_alphabet_sequence_index--;
        }

        std::vector<uint8_t> baud_alphabet_start_sequence;

        baud_alphabet_start_sequence.resize(baud_capacity);

        for (uint32_t i = 0; i < baud_capacity; i++) {
            baud_alphabet_start_sequence[i] = i;
        }

        tackle::file_handle<char> file_in_handle = utility::open_file(in_file, "rb", utility::SharedAccess_DenyWrite);

        tackle::path_string in_file_path = in_file;

        if (out_file_dir.empty()) {
            out_file_dir = utility::get_parent_path(in_file_path);
        }

        switch (mode) {
        case Mode_Gen:
        {
            GenData gen_data;

            gen_data.mode = mode;
            gen_data.baud_alphabet_start_sequence = &baud_alphabet_start_sequence;
            gen_data.bits_per_baud = bits_per_baud;
            gen_data.baud_mask = baud_mask;
            gen_data.baud_capacity = baud_capacity;

            for (uint32_t i = 1; i < num_baud_alphabet_sequences; i++) {
                tackle::path_string out_file{ out_file_dir / utility::get_file_name_stem(in_file_path) + "." + utility::int_to_dec(i, 2) + boost::fs::path{ in_file_path.str() }.extension().string() };

                gen_data.baud_alphabet_end_sequence = &baud_alphabet_sequences[i];
                gen_data.last_bit_offset = 0;

                gen_data.file_out_handle = utility::open_file(out_file, "wb", utility::SharedAccess_DenyWrite);

                fseek(file_in_handle.get(), 0, SEEK_SET);
                tackle::file_reader<char>(file_in_handle, _read_file_chunk).do_read(&gen_data, {}, 256);

                fmt::print(
                    "#{:d}: `{:s}`:\n", i, out_file.c_str());

                for (uint32_t j = 0; j < baud_capacity; j++) {
                    const uint32_t from_baud = baud_alphabet_start_sequence[j];
                    const uint32_t to_baud = (*gen_data.baud_alphabet_end_sequence)[j];
                    if (from_baud != to_baud) {
                        fmt::print(
                            "  {0:#{2}b} -> {1:#{2}b}\n", from_baud, to_baud, bits_per_baud);
                    }
                }
            }
        } break;

        case Mode_Sync:
        {
            const uint32_t syncseq_capacity = (uint32_t(0x01) << syncseq_bit_size);
            const uint32_t syncseq_mask = syncseq_capacity - 1;

            // Buffer size must be padded to a multiple of 8 bytes to be able to read and shift the last 64-bit block.
            //
            const uint32_t padded_stream_byte_size = (stream_byte_size + 7) & ~uint32_t(7);

            SyncData sync_data;

            sync_data.mode = mode;
            sync_data.padded_stream_byte_size = padded_stream_byte_size;
            sync_data.syncseq_bit_size = syncseq_bit_size;
            sync_data.syncseq_bytes = syncseq_bytes;
            sync_data.syncseq_mask = syncseq_mask;
            sync_data.syncseq_repeat = syncseq_repeat;
            sync_data.syncseq_bit_offset = math::uint32_max;

            fseek(file_in_handle.get(), 0, SEEK_SET);
            tackle::file_reader<char>(file_in_handle, _read_file_chunk).do_read(&sync_data, {}, padded_stream_byte_size);

            if (sync_data.syncseq_bit_offset != math::uint32_max) {
                fmt::print(
                    "Synchro sequence offset: {:d}\n", sync_data.syncseq_bit_offset);
            }
        } break;
        }

        [&]() {}();
    }
    catch (const std::exception & ex)
    {
        fprintf(stderr, "%s: %s: error: %s\n", __argv[0], UTILITY_PP_FUNC, ex.what());
        ret = -1;
    }
    catch (...)
    {
        fprintf(stderr, "%s: %s: error: unknown exception\n", __argv[0], UTILITY_PP_FUNC);
        ret = -1;
    }

    return ret;
}
