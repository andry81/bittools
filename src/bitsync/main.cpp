#include "main.hpp"

#include "tacklelib/utility/utility.hpp"
#include "tacklelib/utility/assert.hpp"

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
    struct UserData
    {
        std::vector<uint8_t> *          baud_alphabet_start_sequence;
        std::vector<uint8_t> *          baud_alphabet_end_sequence;
        uint32_t                        bits_per_baud;
        uint32_t                        baud_mask;
        uint32_t                        baud_capacity;
        uint32_t                        last_bit_offset;
        tackle::file_handle<char>       file_out_handle;
    };

    void translate_buffer(UserData & data, uint8_t * buf, uint32_t size)
    {
        std::shared_ptr<uint8_t> write_buf{ new uint8_t[size] };

        uint32_t bit_offset = data.last_bit_offset;
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

            const uint32_t remain_mask = (0x01 << remain_bit_offset) - 1;

            write_buf.get()[byte_offset] = (buf[byte_offset] & remain_mask) | (to_baud_char << remain_bit_offset);

            bit_offset += data.bits_per_baud;
        }

        data.last_bit_offset += num_wholes * data.bits_per_baud;

        const size_t write_size = fwrite(write_buf.get(), 1, size, data.file_out_handle.get());
        const int file_write_err = ferror(data.file_out_handle.get());
        if (write_size < size) {
            utility::debug_break();
            throw std::system_error{ file_write_err, std::system_category(), data.file_out_handle.path() };
        }
    }

    void _read_file_chunk(uint8_t * buf, uint64_t size, void * user_data)
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

        UserData & data = *static_cast<UserData *>(user_data);

        const size_t read_size = uint32_t(size);

        translate_buffer(data, buf, read_size);
    }
}

int main(int argc, char* argv[])
{
    int ret = 0;

    uint32_t stream_bit_width;
    uint32_t syncseq_bit_width;
    uint32_t bits_per_baud;

    tackle::path_string in_file;
    tackle::path_string out_file_dir;

    try {
        po::options_description basic_desc("Basic options");
        basic_desc.add_options()
            ("help,h", "print usage message")

            ("stream_bit_width,w",
                po::value(&stream_bit_width)->required(), "stream width in bits")
            ("syncseq_bit_width,u",
                po::value(&syncseq_bit_width)->required(), "synchro sequence width in bits (must be less than stream width)")
            ("bits_per_baud,q",
                po::value(&bits_per_baud)->required(), "bits per baud in stream (must be <= 3)") // CAUTION: factorial => (2^3)! = 8! = 40320 ( (2^4)! = 16! = 20922789888000 - greater than 32 bits! )

            ("in_file,i",
                po::value(&in_file.str())->required(), "input file path")
            ("out_file_dir,d",
                po::value(&out_file_dir.str()), "output directory path for output files (current directory if not set)");

        po::options_description all_desc("Allowed options");
        all_desc.
            add(basic_desc);

        po::positional_options_description p;
        p.add("stream_bit_width", 1);
        p.add("syncseq_bit_width", 1);
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

        if (!stream_bit_width) {
            fprintf(stderr, "error: stream width must be positive: stream_bit_width=%u\n", stream_bit_width);
            return 255;
        }

        if (!syncseq_bit_width) {
            fprintf(stderr, "error: synchro sequence width must be positive: syncseq_bit_width=%u\n", syncseq_bit_width);
            return 255;
        }

        if (!(bits_per_baud > 0 && bits_per_baud <= 3)) {
            fprintf(stderr, "error: bits per baud must be positive and not greater than 3: bits_per_baud=%u\n", bits_per_baud);
            return 255;
        }

        if (stream_bit_width < syncseq_bit_width + bits_per_baud) {
            fprintf(stderr, "error: synchro sequence must be less than stream width: stream_bit_width=%u syncseq_bit_width=%u bits_per_baud=%u\n",
                stream_bit_width, syncseq_bit_width, bits_per_baud);
            return 255;
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

        const uint32_t baud_capacity = (0x01 << bits_per_baud);
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

                        if (baud_capacity >= 5)
                        for (uint32_t i4 = 0; i4 < baud_capacity; i4++) {
                            bool next = false;

                            for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                                if (baud_alphabet_current_sequence[j] == i4) {
                                    next = true;
                                    break;
                                }
                            }

                            if (next) {
                                continue;
                            }

                            baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i4;
                            baud_alphabet_sequence_index++;

                            for (uint32_t i5 = 0; i5 < baud_capacity; i5++) {
                                bool next = false;

                                for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                                    if (baud_alphabet_current_sequence[j] == i5) {
                                        next = true;
                                        break;
                                    }
                                }

                                if (next) {
                                    continue;
                                }

                                baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i5;
                                baud_alphabet_sequence_index++;

                                for (uint32_t i6 = 0; i6 < baud_capacity; i6++) {
                                    bool next = false;

                                    for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                                        if (baud_alphabet_current_sequence[j] == i6) {
                                            next = true;
                                            break;
                                        }
                                    }

                                    if (next) {
                                        continue;
                                    }

                                    baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i6;
                                    baud_alphabet_sequence_index++;

                                    for (uint32_t i7 = 0; i7 < baud_capacity; i7++) {
                                        bool next = false;

                                        for (uint32_t j = 0; j < baud_alphabet_sequence_index; j++) {
                                            if (baud_alphabet_current_sequence[j] == i7) {
                                                next = true;
                                                break;
                                            }
                                        }

                                        if (next) {
                                            continue;
                                        }

                                        baud_alphabet_current_sequence[baud_alphabet_sequence_index] = i7;
                                        baud_alphabet_sequence_index++;

                                        baud_alphabet_sequences.push_back(baud_alphabet_current_sequence);

                                        baud_alphabet_sequence_index--;
                                    }

                                    baud_alphabet_sequence_index--;
                                }

                                baud_alphabet_sequence_index--;
                            }

                            baud_alphabet_sequence_index--;
                        }
                        else baud_alphabet_sequences.push_back(baud_alphabet_current_sequence);

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

        UserData user_data;

        user_data.baud_alphabet_start_sequence = &baud_alphabet_start_sequence;
        user_data.bits_per_baud = bits_per_baud;
        user_data.baud_mask = baud_mask;
        user_data.baud_capacity = baud_capacity;

        for (uint32_t i = 0; i < num_baud_alphabet_sequences; i++) {
            tackle::path_string out_file{ out_file_dir / utility::get_file_name_stem(in_file_path) + "." + utility::int_to_dec(i, 2) + boost::fs::path{ in_file_path.str() }.extension().string() };

            user_data.baud_alphabet_end_sequence = &baud_alphabet_sequences[i];
            user_data.last_bit_offset = 0;

            user_data.file_out_handle = utility::open_file(out_file, "wb", utility::SharedAccess_DenyWrite);

            fseek(file_in_handle.get(), 0, SEEK_SET);
            tackle::file_reader<char>(file_in_handle, _read_file_chunk).do_read(&user_data, {}, 256);
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
