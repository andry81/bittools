#include "main.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>
#include <boost/preprocessor/cat.hpp>

#include "tacklelib/tackle/file_handle.hpp"
#include "tacklelib/tackle/file_reader.hpp"

#include "tacklelib/utility/platform.hpp"
#include "tacklelib/utility/utility.hpp"
#include "tacklelib/utility/assert.hpp"

#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>


namespace po = boost::program_options;

namespace boost
{
    namespace fs = filesystem;
}

namespace
{
    template <typename TO, typename FROM>
    TO _reflect_value(TO value, FROM type_value)
    {
        const size_t num_bytes = sizeof(type_value);

        auto reflected_value = TO(FROM());

        for (size_t i = 0; i < num_bytes * CHAR_BIT; i++) {
            reflected_value <<= 1;

            if (value & 0x01) {
                reflected_value |= 0x01;
            }

            value >>= 1;
        }

        return reflected_value;
    }

    void _print_table(size_t crc_byte_width, uint32_t crc_polynomial, size_t table_columns, bool reflect)
    {
        const size_t crc_bit_width = crc_byte_width * CHAR_BIT;
        const size_t crc_shifted_width = crc_bit_width - CHAR_BIT;
        const size_t crc_msb_mask = (0x01 << (crc_bit_width - 1));
        size_t crc_max_value;

        switch (crc_byte_width) {
        case 1:
            crc_max_value = uint8_t(~0U);
            break;
        case 2:
            crc_max_value = uint16_t(~0U);
            break;
        case 3:
            crc_max_value = uint32_t(~0U) & 0xFFFFFFU;
            break;
        case 4:
            crc_max_value = uint32_t(~0U);
            break;
        default:
            ASSERT_TRUE(0);
            return;
        }

        const std::string num_crc_chars_str = utility::int_to_dec(crc_byte_width * 2, 0);

        if (!reflect) {
            uint32_t reflected_crc_polynomial;

            switch (crc_byte_width) {
            case 1:
            {
                reflected_crc_polynomial = _reflect_value(crc_polynomial, uint8_t());
            } break;

            case 2:
            {
                reflected_crc_polynomial = _reflect_value(crc_polynomial, uint16_t());
            } break;

            case 3:
            {
                reflected_crc_polynomial = (_reflect_value(crc_polynomial, uint32_t()) >> CHAR_BIT);
            } break;

            case 4:
            {
                reflected_crc_polynomial = _reflect_value(crc_polynomial, uint32_t());
            } break;

            default:
                ASSERT_TRUE(0);
            }

            for (size_t b = 0; ; ) {
                if (b && (b % table_columns) == 0)
                    puts("");

                size_t v = b;
                for (int i = 8; i--; )
                    v = v & 1 ? ((v >> 1) ^ reflected_crc_polynomial) : (v >> 1);

                printf((std::string("0x%0") + num_crc_chars_str + "x").c_str(), v & crc_max_value);

                if (++b == 256)
                    break;
                printf(", ");
            }
        }
        else {
            const size_t crc_bit_width = crc_byte_width * CHAR_BIT;

            boost::variant<uint8_t, uint16_t, uint32_t> table_byte_var;
            size_t b = 0;
            size_t divident = 0;

            switch (crc_byte_width) {
            case 1:
            {
                table_byte_var = uint8_t();
            } break;

            case 2:
            {
                table_byte_var = uint16_t();
            } break;

            case 3:
            case 4:
            {
                table_byte_var = uint32_t();
            } break;

            default:
                ASSERT_TRUE(0);
            }

            static const auto & s_calc_func = [&](size_t divident, auto type_value) {
                auto & table_byte_ref = boost::get<decltype(type_value)>(table_byte_var);
                table_byte_ref = decltype(type_value)(uint32_t(divident << crc_shifted_width));
                for (uint8_t bit = 0; bit < 8; bit++)
                {
                    if ((table_byte_ref & crc_msb_mask) != 0)
                    {
                        table_byte_ref <<= 1;
                        table_byte_ref ^= crc_polynomial;
                    }
                    else
                    {
                        table_byte_ref <<= 1;
                    }
                }

                printf((std::string("0x%0") + num_crc_chars_str + "x").c_str(), table_byte_ref);
            };

            for (; divident < 256; divident++)
            {
                if (b && (b % table_columns) == 0)
                    puts("");

                switch (crc_byte_width) {
                case 1:
                {
                    s_calc_func(divident, uint8_t());
                } break;

                case 2:
                {
                    s_calc_func(divident, uint16_t());
                } break;

                case 3:
                case 4:
                {
                    s_calc_func(divident, uint32_t());
                } break;

                default:
                    ASSERT_TRUE(0);
                }

                if (++b == 256)
                    break;
                printf(", ");
            }
        }
    }
}

int main(int argc, char **argv)
{
    uint32_t crc_width;
    std::string crc_polynomial_str;
    uint32_t table_columns = 0;
    uint8_t column_separator = ',';
    bool reflect = false;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "print usage message")

            ("width,w",
                po::value(&crc_width)->required(),      "crc width (ex: 8, 16, 24, 32)")
            ("polynomial,p",
                po::value(&crc_polynomial_str)->required(), "crc from LSB to MSB polynomial")
            ("columns,c",
                po::value(&table_columns),              "table columns per row")
            ("separator,s",
                po::value(&column_separator),           "character separator between columns")
            ("reflect,r",
                po::bool_switch()->default_value(false), "reflect table/implementation (use shift from MSB to the left instead default: shift from LSB to the right)")
            ;

        po::positional_options_description p;
        p.add("width", 1);
        p.add("polynomial", 1);
        p.add("columns", 1);
        p.add("separator", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        reflect = vm["reflect"].as<bool>();

        po::notify(vm); // important, otherwise related option variables won't be initialized
    }
    catch (std::exception& e)
    {
        fprintf(stderr, "%s\n",
            (boost::format(
                BOOST_PP_CAT(BOOST_PP_CAT("error: ", __FUNCTION__), ": exception: \"%s\".")) %
                    e.what()).str().c_str());
        return -1;
    }
    catch (...)
    {
        fprintf(stderr, "%s\n",
            (boost::format(
                BOOST_PP_CAT("error: " __FUNCTION__, ": unknown exception."))).str().c_str());
        return -1;
    }

    const size_t crc_byte_width = (crc_width + CHAR_BIT - 1) / CHAR_BIT;
    const uint32_t crc_polynomial = std::stoul(crc_polynomial_str, 0, 0);

    switch(crc_byte_width) {
    case 1:
        if (!table_columns) {
            table_columns = 16;
        }
        break;
    case 2:
        if (!table_columns) {
            table_columns = 8;
        }
        break;
    case 3:
        if (!table_columns) {
            table_columns = 6;
        }
        break;
    case 4:
        if (!table_columns) {
            table_columns = 4;
        }
        break;

    default:
        fprintf(stderr, "%s\n",
            (boost::format(
                BOOST_PP_CAT("error: " __FUNCTION__, ": invalid crc width: width=%u supported=[8,16,24,32]")) %
                    crc_width).str().c_str());
        return 2;
    }

    _print_table(crc_byte_width, crc_polynomial, table_columns, reflect);

    return 0;
}
