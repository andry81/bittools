#include "main.hpp"

#include "utility/utility.hpp"
#include "utility/assert.hpp"

#include "tackle/file_reader.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

namespace po = boost::program_options;
using namespace utility;

namespace boost {
    namespace fs = filesystem;
}

namespace
{
    struct UserData
    {
        std::vector<uint8_t> xor_value;
        FileHandle file_out_handle;
    };

    void xor_buffer(uint8_t * buf, uint32_t size, const std::vector<uint8_t> & xor_value = { 0xff, 0xff, 0xff, 0xff })
    {
        ASSERT_TRUE(buf && size);
        uint32_t buf_offset = 0;
        const size_t xor_value_size = xor_value.size();
        const uint32_t num_wholes = size / xor_value_size;
        for (uint32_t i = 0; i < num_wholes; i++)
        {
            switch (xor_value_size) {
            case 64:
                *(uint64_t*)(buf + buf_offset) = *(uint64_t*)&xor_value[0];
                break;
            case 32:
                *(uint32_t*)(buf + buf_offset) = *(uint32_t*)&xor_value[0];
                break;
            default:
                for (size_t i = 0; i < xor_value_size; i++) {
                    (buf + buf_offset)[i] ^= xor_value[i];
                }
            }
            buf_offset += xor_value_size;
        }

        const uint32_t reminder = size % xor_value_size;
        if (reminder) {
            for (size_t i = 0; i < reminder; i++) {
                (buf + buf_offset)[i] ^= xor_value[i];
            }
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

        const UserData & data = *static_cast<UserData *>(user_data);

        const size_t read_size = uint32_t(size);

        xor_buffer(buf, read_size, data.xor_value);

        const size_t write_size = fwrite(buf, 1, read_size, data.file_out_handle.get());
        const int file_write_err = ferror(data.file_out_handle.get());
        if (write_size < read_size) {
            utility::debug_break();
            throw std::system_error{ file_write_err, std::system_category(), data.file_out_handle.path() };
        }
    }
}

int main(int argc, char* argv[])
{
    try {
        std::string in_file;
        std::string out_file;
        std::string num_xor_bits_str;

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "print usage message")
            ("input,i",
                po::value(&in_file), "input file and output file prefix if output file is not set explicitly")
            ("output,o",
                po::value(&out_file), "output file")
            ("xor_bits,b",
                po::value(&num_xor_bits_str), "number of first bits in the file to XOR with")
        ;

        po::positional_options_description p;
        p.add("input", 1);
        p.add("xor_bits", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm); // important, otherwise related option variables won't be initialized

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        if (!boost::fs::exists(in_file)) {
            fprintf(stderr, "error: input file is not found: \"%s\"\n", in_file.c_str());
            return 1;
        }

        if (in_file == out_file) {
            fprintf(stderr, "error: output file should not be input\n");
            return 2;
        }

        FileHandle file_in_handle = open_file(in_file, "rb", _SH_DENYWR);

        boost::fs::path in_file_path = boost::fs::path(in_file);

        if (out_file.empty()) {
            const std::string & out_parent_path = in_file_path.parent_path().string();
            out_file = out_parent_path + (!out_parent_path.empty() ? "/" : "") + in_file_path.stem().string() + "_xor" + in_file_path.extension().string();
        }

        FileHandle file_out_handle = open_file(out_file, "wb", _SH_DENYWR);

        typedef std::shared_ptr<uint8_t> ReadBufSharedPtr;

        std::vector<uint8_t> xor_value = { 0xff, 0xff, 0xff, 0xff };
        uint32_t bit_size = 32;
        if (!num_xor_bits_str.empty()) {
            bit_size = std::stoul(num_xor_bits_str, 0, 0);
        }
        if (bit_size > 1024 * 1024) {
            bit_size = 1024 * 1024 * CHAR_BIT;
        }

        uint32_t next_read_size = (bit_size + CHAR_BIT - 1) / CHAR_BIT;
        size_t read_size = 0;
        size_t write_size = 0;

        // read the xor value from the file beginning
        xor_value.resize(next_read_size);
        read_size = fread(&xor_value[0], 1, next_read_size, file_in_handle.get());
        const int file_read_err = ferror(file_in_handle.get());
        if (read_size < next_read_size) {
            utility::debug_break();
            throw std::system_error{ file_read_err, std::system_category(), file_in_handle.path() };
        }

        write_size = fwrite(&xor_value[0], 1, read_size, file_out_handle.get());
        const int file_write_err = ferror(file_out_handle.get());
        if (write_size < read_size) {
            utility::debug_break();
            throw std::system_error{ file_write_err, std::system_category(), file_out_handle.path() };
        }

        UserData user_data;
        user_data.xor_value = xor_value;
        user_data.file_out_handle = file_out_handle;
        tackle::FileReader(file_in_handle, _read_file_chunk).do_read(&user_data, {}, next_read_size);
    }
    catch (std::exception & e) {
        std::cerr << e.what() << "\n";
        return -1;
    }

    return 0;
}
