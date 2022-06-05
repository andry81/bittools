#include "main.hpp"

#include <tacklelib/utility/preprocessor.hpp>

#include <inttypes.h>


const TCHAR * g_flags_to_parse_arr[] = {
    _T("/impl-token"), _T("/impl-mwsocm"), _T("/impl-msocmd"), _T("/impl-mwacocv"),
    _T("/corr-multiply-method"), _T("/corr-mm"), _T("/corr-mm-inverted-xor-prime1033"), _T("/corr-mm-dispersed-value-prime1033"),
    _T("/stream-byte-size"), _T("/s"),
    _T("/stream-bit-size"), _T("/si"),
    _T("/syncseq-bit-size"), _T("/q"),
    _T("/syncseq-int32"), _T("/k"),
    _T("/syncseq-min-repeat"), _T("/r"),
    _T("/syncseq-max-repeat"), _T("/rmax"),
    _T("/stream-min-period"), _T("/spmin"),
    _T("/stream-max-period"), _T("/spmax"),
    _T("/max-periods-in-offset"), _T("/max-pio"),
    _T("/max-corr-values-per-period"), _T("/max-cvpp"),
    _T("/gen-token"), _T("/g"),
    _T("/gen-input-noise"), _T("/inn"),
    _T("/insert-output-syncseq"), _T("/outss"),
    _T("/fill-output-syncseq"), _T("/outssf"),
    _T("/tee-input"),
    _T("/corr-min"),
    _T("/corr-mean-min"),
    _T("/no-zero-corr"),
    _T("/skip-calc-on-filtered-corr-value-use"), _T("/skip-calc-on-fcvu"),
    _T("/skip-max-weighted-sum-of-corr-mean-calc"), _T("/skip-mwsocm-calc"),
    _T("/use-linear-corr"),
    _T("/use-max-corr-mean"),
    _T("/sort-at-first-by-max-corr-mean"),
    _T("/return-sorted-result"),
    _T("/corr-mean-buf-max-size-mb")
};

const TCHAR * g_empty_flags_arr[] = {
    _T("")
};

inline int invalid_format_flag(const TCHAR * arg)
{
    _tprintf(_T("flag format is invalid: \"%s\"\n"), arg);
    return 255;
}

inline int invalid_format_flag_message(const TCHAR * fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    tvprintf(fmt, vl);
    va_end(vl);
    return 255;
}

template <size_t N>
inline bool is_arg_in_filter(const TCHAR * arg, const TCHAR * (& filter_arr)[N])
{
    bool is_found = false;

    utility::for_each_unroll(filter_arr, [&](const TCHAR * str) {
        if (!tstrcmp(arg, str)) {
            is_found = true;
            return false;
        }
        return true;
    });

    return is_found;
}

template <size_t N>
inline bool is_arg_equal_to(const TCHAR * arg, const TCHAR (& cmp_arg)[N])
{
    static_assert(N > 1, "cmp_arg[N] must contain at least one character");
    return !tstrncmp(arg, cmp_arg, N);
}

// return:
//  -1 - argument is not detected (not known)
//   0 - argument is detected and is not in inclusion filter
//   1 - argument is detected and is in inclusion filter
//   2 - argument is detected but not checked on inclusion because of invalid format
//   3 - argument is excluded without any checks
//
template <size_t M, size_t N>
int parse_arg_to_option(
    int & error, const TCHAR * arg, int argc, const TCHAR * argv[], int & arg_offset, Flags & flags, Options & options,
    const TCHAR * (& include_filter_arr)[N], const TCHAR * (& exclude_filter_arr)[M],
    std::vector<std::tstring> & mod_flags)
{
    // intercept here specific global variables accidental usage instead of local variables
    static struct {} g_options;
    static struct {} g_flags;

    error = 0;

    const TCHAR * start_arg = arg;
    const TCHAR * arg_suffix = nullptr;

    if (ptrdiff_t(utility::addressof(exclude_filter_arr)) != ptrdiff_t(utility::addressof(g_empty_flags_arr))) {
        if (is_arg_in_filter(arg, exclude_filter_arr)) {
            return 3;
        }
    }

    if (is_arg_equal_to(arg, _T("/impl-token"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                if (options.impl_token != Impl::impl_unknown) {
                    return invalid_format_flag_message(_T("`/impl-token` option is mixed with `/impl-opt*`\n"));
                }

                if (is_arg_equal_to(arg, _T("max-weighted-sum-of-corr-mean")) || is_arg_equal_to(arg, _T("max-input-noise-resistence"))) {
                    options.impl_token = Impl::impl_max_weighted_sum_of_corr_mean;
                    return 1;
                }
                else if (is_arg_equal_to(arg, _T("min-sum-of-corr-mean-deviat"))) {
                    options.impl_token = Impl::impl_min_sum_of_corr_mean_deviat;
                    return 1;
                }
                else if (is_arg_equal_to(arg, _T("max-weighted-autocorr-of-corr-values")) || is_arg_equal_to(arg, _T("max-input-performance"))) {
                    options.impl_token = Impl::impl_max_weighted_autocorr_of_corr_values;
                    return 1;
                }
                else {
                    error = invalid_format_flag(start_arg);
                    return 2;
                }
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/impl-mwsocm"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            if (options.impl_token != Impl::impl_unknown) {
                return invalid_format_flag_message(_T("`/impl-token` option is mixed with `/impl-opt*`\n"));
            }
            options.impl_token = Impl::impl_max_weighted_sum_of_corr_mean;
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/impl-msocmd"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            if (options.impl_token != Impl::impl_unknown) {
                return invalid_format_flag_message(_T("`/impl-token` option is mixed with `/impl-opt*`\n"));
            }
            options.impl_token = Impl::impl_min_sum_of_corr_mean_deviat;
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/impl-mwacocv"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            if (options.impl_token != Impl::impl_unknown) {
                return invalid_format_flag_message(_T("`/impl-token` option is mixed with `/impl-opt*`\n"));
            }
            options.impl_token = Impl::impl_max_weighted_autocorr_of_corr_values;
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/corr-multiply-method")) || is_arg_equal_to(arg, _T("/corr-mm"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                if (options.corr_mm != Impl::corr_muliply_unknown) {
                    return invalid_format_flag_message(_T("`/corr-multiply-method` option is mixed with another `/corr-multiply-method`\n"));
                }

                if (is_arg_equal_to(arg, _T("inverted-xor-prime1033"))) {
                    options.corr_mm = Impl::corr_muliply_inverted_xor_prime1033;
                    return 1;
                }
                else if (is_arg_equal_to(arg, _T("dispersed-value-prime1033"))) {
                    options.corr_mm = Impl::corr_muliply_dispersed_value_prime1033;
                    return 1;
                }
                else {
                    error = invalid_format_flag(start_arg);
                    return 2;
                }
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/corr-mm-inverted-xor-prime1033"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            if (options.corr_mm != Impl::corr_muliply_unknown) {
                return invalid_format_flag_message(_T("`/corr-multiply-method` option is mixed with another `/corr-multiply-method`\n"));
            }
            options.corr_mm = Impl::corr_muliply_inverted_xor_prime1033;
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/corr-mm-dispersed-value-prime1033"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            if (options.corr_mm != Impl::corr_muliply_unknown) {
                return invalid_format_flag_message(_T("`/corr-multiply-method` option is mixed with another `/corr-multiply-method`\n"));
            }
            options.corr_mm = Impl::corr_muliply_dispersed_value_prime1033;
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/stream-byte-size")) || is_arg_equal_to(arg, _T("/s"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.stream_byte_size = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/stream-bit-size")) || is_arg_equal_to(arg, _T("/si"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.stream_bit_size = _ttoi64(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/stream-min-period")) || is_arg_equal_to(arg, _T("/spmin"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.stream_min_period = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/stream-max-period")) || is_arg_equal_to(arg, _T("/spmax"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.stream_max_period = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/max-periods-in-offset")) || is_arg_equal_to(arg, _T("/max-pio"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.max_periods_in_offset = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/max-corr-values-per-period")) || is_arg_equal_to(arg, _T("/max-cvpp"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.max_corr_values_per_period = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/syncseq-bit-size")) || is_arg_equal_to(arg, _T("/q"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.syncseq_bit_size = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/syncseq-int32")) || is_arg_equal_to(arg, _T("/k"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.syncseq_int32 = utility::str_to_number<decltype(options.syncseq_int32)>(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/syncseq-min-repeat")) || is_arg_equal_to(arg, _T("/r"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.syncseq_min_repeat = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/syncseq-max-repeat")) || is_arg_equal_to(arg, _T("/rmax"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.syncseq_max_repeat = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/gen-token")) || is_arg_equal_to(arg, _T("/g"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.gen_token = arg;
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/gen-input-noise")) || is_arg_equal_to(arg, _T("/inn"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.gen_input_noise_bit_block_size = _ttoi(arg);

                arg_offset += 1;
                if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
                    options.gen_input_noise_block_bit_prob = _ttoi(arg);
                    return 1;
                }
                else error = invalid_format_flag(start_arg);
                return 2;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/insert-output-syncseq")) || is_arg_equal_to(arg, _T("/outss"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                tsscanf(arg, _T("%u:%u"), &options.insert_output_syncseq_first_offset, &options.insert_output_syncseq_end_offset);

                arg_offset += 1;
                if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
                    tsscanf(arg, _T("%u:%u"), &options.insert_output_syncseq_period, &options.insert_output_syncseq_period_repeat);
                    flags.insert_output_syncseq_instead_fill = true;
                    return 1;
                }
                else error = invalid_format_flag(start_arg);
                return 2;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/fill-output-syncseq")) || is_arg_equal_to(arg, _T("/outssf"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                if (!flags.insert_output_syncseq_instead_fill) {
                    tsscanf(arg, _T("%u:%u"), &options.insert_output_syncseq_first_offset, &options.insert_output_syncseq_end_offset);
                }

                arg_offset += 1;
                if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
                    if (!flags.insert_output_syncseq_instead_fill) {
                        tsscanf(arg, _T("%u:%u"), &options.insert_output_syncseq_period, &options.insert_output_syncseq_period_repeat);
                    }

                    return 1;
                }
                else error = invalid_format_flag(start_arg);
                return 2;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/tee-input"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.tee_input_file = arg;
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/corr-min"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.corr_min = utility::str_to_float(std::tstring{ arg });
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/corr-mean-min"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.corr_mean_min = utility::str_to_float(std::tstring{ arg });
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/no-zero-corr"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.no_zero_corr = true;
            mod_flags.push_back(arg);
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/use-linear-corr"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.use_linear_corr = true;
            mod_flags.push_back(arg);
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/skip-calc-on-filtered-corr-value-use")) || is_arg_equal_to(arg, _T("/skip-calc-on-fcvu"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.skip_calc_on_filtered_corr_value_use = true;
            mod_flags.push_back(arg);
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/skip-max-weighted-sum-of-corr-mean-calc")) || is_arg_equal_to(arg, _T("/skip-mwsocm-calc")) || is_arg_equal_to(arg, _T("/use-max-corr-mean"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.skip_max_weighted_sum_of_corr_mean_calc = true;
            mod_flags.push_back(arg);
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/sort-at-first-by-max-corr-mean"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.sort_at_first_by_max_corr_mean = true;
            mod_flags.push_back(arg);
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/return-sorted-result"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.return_sorted_result = true;
            mod_flags.push_back(arg);
            return 1;
        }
        return 0;
    }
    if (is_arg_equal_to(arg, _T("/corr-mean-buf-max-size-mb"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.corr_mean_buf_max_size_mb = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }

    return -1;
}

namespace boost {
    namespace fs = filesystem;
}

int _tmain(int argc, const TCHAR * argv[])
{
    //// user limits
    //const constexpr size_t min_period = 2;
    //const constexpr size_t max_period = 15;

    //assert(max_period >= min_period);

    //struct Corr
    //{
    //    int period;
    //    float corr;
    //};
    //float in_arr[] = { 3, 4, 1.1, 8, 3, 1.1, 7, 4, 1.1, 6, 8, 1.1, 2, 6, 1.1, 7 };

    //const constexpr size_t in_arr_size = sizeof(in_arr) / sizeof(in_arr[0]);

    //assert(min_period < in_arr_size);
    //assert(max_period < in_arr_size);

    //Corr out_arr[in_arr_size - 1];
    //float in_arr_square[in_arr_size];

    //// calibration
    //auto num_offset_shifts = max_period + 1;

    //num_offset_shifts = (std::max)(num_offset_shifts, min_period + 1);

    //if (in_arr_size < min_period + num_offset_shifts) {
    //    // recalculate
    //    num_offset_shifts = in_arr_size - min_period;
    //}

    //const auto num_autocorr_values = min_period + num_offset_shifts;

    //for (size_t i = 0; i < in_arr_size; i++) {
    //    in_arr_square[i] = in_arr[i] * in_arr[i];
    //}

    //for (size_t i = 0, offset_shift = min_period; max_period >= offset_shift && num_offset_shifts >= min_period; i++, offset_shift++, num_offset_shifts--) {
    //    auto & autocorr = out_arr[i];

    //    autocorr.period = offset_shift;

    //    float corr_numerator_value = 0;

    //    float corr_denominator_first_value = 0;
    //    float corr_denominator_second_value = 0;

    //    for (size_t j = 0; j < num_offset_shifts; j++) {
    //        const float corr_value = in_arr[j] * in_arr[j + offset_shift];

    //        corr_denominator_first_value += in_arr_square[j];
    //        corr_denominator_second_value += in_arr_square[j + offset_shift];

    //        corr_numerator_value += corr_value * corr_value;
    //    }

    //    autocorr.corr = std::sqrt(corr_numerator_value * num_autocorr_values * num_autocorr_values / (std::max)(corr_denominator_first_value, corr_denominator_second_value));
    //}

    //auto begin_it = &out_arr[0];
    //auto end_it = &out_arr[in_arr_size - 1];

    //std::sort(begin_it, end_it, [&](const Corr & l, const Corr & r) -> bool
    //{
    //    return l.corr > r.corr;
    //});

    //return 0;

    PWSTR cmdline_str = GetCommandLine();

    size_t arg_offset_begin = 0;

    if (argv[0][0] != _T('/')) { // arguments shift detection
        arg_offset_begin = 1;
    }

    int ret = 127; // not found

    // algorithm modification flags
    std::vector<std::tstring> mod_flags;

    mod_flags.reserve(16);

    // NOTE:
    //  lambda to bypass msvc error: `error C2712: Cannot use __try in functions that require object unwinding`
    //
    return ret = [&]() -> int { __try {
        return ret = [&]() -> int {

            if (!argc || !argv[0]) {
                return 255;
            }

            int arg_offset = arg_offset_begin;

            const TCHAR * arg;
            int parse_error = 255;
            int parse_result;

            bool print_help = false;

            try {
                // read flags
                while (argc >= arg_offset + 1)
                {
                    arg = argv[arg_offset];
                    if (!arg) return invalid_format_flag(arg);

                    if (tstrncmp(arg, _T("/"), 1)) {
                        break;
                    }

                    if (!tstrncmp(arg, _T("//"), 3)) {
                        arg_offset += 1;
                        break;
                    }

                    if (!tstrncmp(arg, _T("/?"), 3)) {
                        arg_offset += 1;

                        print_help = true;

                        break;
                    }

                    if ((parse_result = parse_arg_to_option(parse_error, arg, argc, argv, arg_offset, g_flags, g_options, g_flags_to_parse_arr, g_empty_flags_arr, mod_flags)) >= 0) {
                        if (!parse_result && !parse_error) {
                            parse_error = invalid_format_flag(arg);
                        }
                        if (parse_error != 0) {
                            return parse_error;
                        }
                    }
                    else return invalid_format_flag(arg);

                    arg_offset += 1;
                }

                if (print_help) {
                    tfputs(
#include "help_inl.hpp"
                        , stderr);

                    return 1;
                }

                if (argc >= arg_offset + 1 && (arg = argv[arg_offset]) && tstrlen(arg)) {
                    g_options.mode = arg;
                }

                arg_offset += 1;

                Mode mode = Mode_None;

                if (g_options.mode == _T("gen")) {
                    mode = Mode_Gen;
                }
                else if (g_options.mode == _T("sync")) {
                    mode = Mode_Sync;
                }
                else if (g_options.mode == _T("pipe")) {
                    mode = Mode_Pipe;
                }

                if (mode == Mode_None) {
                    _ftprintf(stderr, _T("error: mode is not known: mode=%s\n"), g_options.mode.c_str());
                    return 255;
                }

                switch (mode) {
                case Mode_Sync:
                {
                    if (g_options.impl_token == Impl::impl_unknown) {
                        _ftprintf(stderr, _T("error: the implementation (`/impl-*` options) must be selected explicitly for the mode: mode=%s\n"), g_options.mode.c_str());
                        return 255;
                    }
                } break;
                }

                switch (mode) {
                case Mode_Gen:
                case Mode_Gen_Sync:
                {
                    if (argc >= arg_offset + 1 && (arg = argv[arg_offset]) && tstrlen(arg)) {
                        g_options.bits_per_baud = _ttoi(arg);
                    }

                    arg_offset += 1;

                    if (!(g_options.bits_per_baud > 0 && g_options.bits_per_baud <= 2)) {
                        _ftprintf(stderr, _T("error: bits per baud must be positive and not greater than 2: bits_per_baud=%u\n"), g_options.bits_per_baud);
                        return 255;
                    }
                } break;
                }

                if (argc >= arg_offset + 1 && (arg = argv[arg_offset]) && tstrlen(arg)) {
                    g_options.input_file = arg;
                }

                arg_offset += 1;

                switch (mode) {
                case Mode_Gen:
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    if (argc >= arg_offset + 1 && (arg = argv[arg_offset]) && tstrlen(arg)) {
                        g_options.output_file_dir = arg;
                    }

                    arg_offset += 1;
                } break;

                case Mode_Pipe:
                {
                    if (argc >= arg_offset + 1 && (arg = argv[arg_offset]) && tstrlen(arg)) {
                        g_options.output_file = arg;
                    }

                    arg_offset += 1;
                } break;
                }

                if (g_options.stream_byte_size && g_options.stream_bit_size) {
                    _ftprintf(stderr, _T("error: stream_byte_size and stream_bit_size options are mixed.\n"));
                    return 255;
                }

                switch (mode) {
                case Mode_Gen:
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    // safety check by 2GB maximum
                    if (g_options.stream_byte_size >= math::uint32_max / 2) {
                        _ftprintf(stderr, _T("error: stream_byte_size is too big: stream_byte_size=%u\n"),
                            g_options.stream_byte_size);
                        return 255;
                    }

                    // safety check by 2GB maximum
                    if (g_options.stream_bit_size >= uint64_t(math::uint32_max) * 8 / 2) {
                        _ftprintf(stderr, _T("error: stream_bit_size is too big: stream_bit_size=%") _T(PRIu64) _T("\n"),
                            g_options.stream_bit_size);
                        return 255;
                    }
                } break;
                }

                switch (mode) {
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    if (g_options.stream_min_period != math::uint32_max && !g_options.stream_min_period) {
                        _ftprintf(stderr, _T("error: stream_min_period must be positive\n"));
                        return 255;
                    }

                    if (!g_options.stream_max_period) {
                        _ftprintf(stderr, _T("error: stream_max_period must be positive\n"));
                        return 255;
                    }

                    if (g_options.stream_min_period != math::uint32_max && g_options.stream_max_period < g_options.stream_min_period) {
                        _ftprintf(stderr, _T("error: stream_min_period must be not greater than stream_max_period: stream_min_period=%u stream_max_period=%u\n"),
                            g_options.stream_min_period, g_options.stream_max_period);
                        return 255;
                    }

                    if (!g_options.syncseq_bit_size) {
                        _ftprintf(stderr, _T("error: syncseq_bit_size must be positive\n"));
                        return 255;
                    }

                    if (32 < g_options.syncseq_bit_size) {
                        _ftprintf(stderr, _T("error: syncseq_bit_size must be less or equal to 32 bits\n"));
                        return 255;
                    }

                    if (g_options.stream_min_period != math::uint32_max && g_options.syncseq_bit_size >= g_options.stream_min_period) {
                        _ftprintf(stderr, _T("error: stream_min_period must be greater than syncseq_bit_size: syncseq_bit_size=%u stream_min_period=%u\n"),
                            g_options.syncseq_bit_size, g_options.stream_min_period);
                        return 255;
                    }

                    if (g_options.stream_max_period != math::uint32_max && g_options.syncseq_bit_size >= g_options.stream_max_period) {
                        _ftprintf(stderr, _T("error: stream_max_period must be greater than syncseq_bit_size: syncseq_bit_size=%u stream_max_period=%u\n"),
                            g_options.syncseq_bit_size, g_options.stream_max_period);
                        return 255;
                    }

                    //if (!g_options.syncseq_int32) {
                    //    _ftprintf(stderr, _T("error: syncseq_bytes must not 0\n"));
                    //    return 255;
                    //}

                    if (g_options.syncseq_min_repeat != math::uint32_max && !g_options.syncseq_min_repeat) {
                        _ftprintf(stderr, _T("error: syncseq_min_repeat must be positive\n"));
                        return 255;
                    }

                    if (g_options.syncseq_max_repeat != math::uint32_max && !g_options.syncseq_max_repeat) {
                        _ftprintf(stderr, _T("error: syncseq_max_repeat must be positive\n"));
                        return 255;
                    }

                    // /r vs /rmax
                    if (g_options.syncseq_min_repeat != math::uint32_max && g_options.syncseq_max_repeat != math::uint32_max && g_options.syncseq_max_repeat < g_options.syncseq_min_repeat) {
                        _ftprintf(stderr, _T("error: syncseq_min_repeat must be not greater than syncseq_max_repeat: syncseq_min_repeat=%u syncseq_max_repeat=%u\n"),
                            g_options.syncseq_min_repeat, g_options.syncseq_max_repeat);
                        return 255;
                    }

                    if (g_options.max_corr_values_per_period != math::uint32_max && !g_options.max_corr_values_per_period) {
                        _ftprintf(stderr, _T("error: max_corr_values_per_period must be positive\n"));
                        return 255;
                    }
                } break;
                }

                if (g_options.gen_input_noise_bit_block_size && g_options.gen_input_noise_bit_block_size > 32767) {
                    _ftprintf(stderr, _T("error: gen_input_noise_bit_block_size must be between 1-32767: \"%u\"\n"), g_options.gen_input_noise_bit_block_size);
                    return 255;
                }

                if (g_options.gen_input_noise_bit_block_size && (!g_options.gen_input_noise_block_bit_prob || g_options.gen_input_noise_block_bit_prob > 100)) {
                    _ftprintf(stderr, _T("error: gen_input_noise_block_bit_prob must be between 1-100: \"%u\"\n"), g_options.gen_input_noise_block_bit_prob);
                    return 255;
                }

                if (g_options.corr_min != math::float_max && (g_options.corr_min < 0 || g_options.corr_min > 1.0)) {
                    _ftprintf(stderr, _T("error: corr_min must be in range [0; 1]: corr_min=%f\n"), g_options.corr_min);
                    return 255;
                }

                if (g_options.corr_mean_min != math::float_max && (g_options.corr_mean_min < 0 || g_options.corr_mean_min > 1.0)) {
                    _ftprintf(stderr, _T("error: corr_mean_min must be in range [0; 1]: corr_mean_min=%f\n"), g_options.corr_mean_min);
                    return 255;
                }

                if (!g_options.corr_mean_buf_max_size_mb) {
                    _ftprintf(stderr, _T("error: corr_mean_buf_max_size_mb must be positive.\n"));
                    return 255;
                }

                if (g_options.input_file.empty() || !utility::is_regular_file(g_options.input_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, false)) {
                    _ftprintf(stderr, _T("error: input file is not found: \"%s\"\n"), g_options.input_file.c_str());
                    return 255;
                }

                tackle::file_handle<TCHAR> tee_file_in_handle;

                if (!g_options.tee_input_file.empty()) {
                    tee_file_in_handle = utility::open_file(g_options.tee_input_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, _T("wb"), utility::SharedAccess_DenyWrite, 0, 0);
                    if (!tee_file_in_handle.get()) {
                        _ftprintf(stderr, _T("error: could not open output tee file for the input: \"%s\"\n"), g_options.tee_input_file.c_str());
                        return 255;
                    }
                }

                switch (mode) {
                case Mode_Gen:
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    if (g_options.output_file_dir.empty()) {
                        g_options.output_file_dir = utility::get_current_path(false, tackle::tag_native_path_tstring{});
                    }

                    if (g_options.output_file_dir != _T(".") && !utility::is_directory_path(g_options.output_file_dir, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16) <wchar_t>{}, false)) {
                        _ftprintf(stderr, _T("error: output directory path is not a directory: path=\"%s\"\n"), g_options.output_file_dir.c_str());
                        return 255;
                    }
                } break;

                case Mode_Pipe:
                {
                    if (g_options.output_file.empty()) {
                        _ftprintf(stderr, _T("error: output file is not defined\n"));
                        return 255;
                    }

                    if (utility::is_regular_file(g_options.output_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, false) &&
                        utility::is_same_file(g_options.input_file, g_options.output_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, false)) {
                        _ftprintf(stderr, _T("error: output file must not be input file: \"%s\"\n"), g_options.output_file.c_str());
                        return 255;
                    }
                } break;
                }

                const tackle::file_handle<TCHAR> file_in_handle =
                    utility::open_file(g_options.input_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, _T("rb"), utility::SharedAccess_DenyWrite);

                if (g_options.stream_bit_size) {
                    g_options.stream_byte_size = uint32_t((g_options.stream_bit_size + 7) / 8);
                }

                const uint64_t stream_byte_size = uint32_t((std::min)(utility::get_file_size(file_in_handle), uint64_t(math::uint32_max))); // CAUTION: read only first 4GB
                if (!g_options.stream_byte_size || stream_byte_size < g_options.stream_byte_size) {
                    g_options.stream_byte_size = uint32_t(stream_byte_size);
                    g_options.stream_bit_size = uint32_t(g_options.stream_byte_size) * 8;
                }

                switch (mode) {
                case Mode_Gen:
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    if (g_options.stream_bit_size && g_options.stream_bit_size <= 32) {
                        _ftprintf(stderr, _T("error: stream_bit_size must be greater than 32 bits\n"));
                        return 255;
                    }
                } break;
                }

                switch (mode) {
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    // /spmin vs /si
                    if (g_options.stream_min_period != math::uint32_max && g_options.stream_min_period >= g_options.stream_bit_size) {
                        _ftprintf(stderr, _T("error: stream_min_period must be less than stream_byte_size: stream_min_period=%u stream_bit_size=%") _T(PRIu64) _T("\n"),
                            g_options.stream_min_period, g_options.stream_bit_size);
                        return 255;
                    }

                    // /spmax vs /si
                    if (g_options.stream_max_period != math::uint32_max && g_options.stream_max_period >= g_options.stream_bit_size) {
                        _ftprintf(stderr, _T("error: stream_max_period must be less than stream_byte_size: stream_max_period=%u stream_bit_size=%") _T(PRIu64) _T("\n"),
                            g_options.stream_max_period, g_options.stream_bit_size);
                        return 255;
                    }

                    // /r + /spmin vs /si
                    if (g_options.syncseq_min_repeat != math::uint32_max && g_options.stream_min_period != math::uint32_max && g_options.syncseq_min_repeat * g_options.stream_min_period >= g_options.stream_bit_size) {
                        _ftprintf(stderr, _T("error: syncseq_min_repeat * stream_min_period must be less than stream_bit_size: syncseq_min_repeat=%u stream_min_period=%u stream_bit_size=%") _T(PRIu64) _T("\n"),
                            g_options.syncseq_min_repeat, g_options.stream_min_period, g_options.stream_bit_size);
                        return 255;
                    }
                } break;
                }

                // update options defaults
                g_options.update_impl_token_defaults();
                g_options.update_corr_mm_defaults();
                g_options.update_corr_min_defaults(g_flags);

                //if (!g_options.is_corr_mm_default()) {
                //    mod_flags.push_back(std::tstring{ _T("/corr-mm-") } + g_options.corr_mm_token_str);
                //}

                if (g_options.syncseq_min_repeat == math::uint32_max) {
                    g_options.syncseq_min_repeat = 0;
                }

                if (g_options.syncseq_max_repeat == math::uint32_max) {
                    if (g_options.syncseq_min_repeat != math::uint32_max && g_options.syncseq_min_repeat > DEFAULT_SYNCSEQ_MAXIMAL_REPEAT_PERIOD) {
                        g_options.syncseq_max_repeat = g_options.syncseq_min_repeat;
                    }
                    else {
                        g_options.syncseq_max_repeat = DEFAULT_SYNCSEQ_MAXIMAL_REPEAT_PERIOD;
                    }
                }

                if (g_options.stream_min_period == math::uint32_max) {
                    g_options.stream_min_period = 0;
                }

                if (g_options.max_corr_values_per_period == math::uint32_max) {
                    g_options.max_corr_values_per_period = DEFAULT_MAX_CORR_VALUES_PER_PERIOD;
                }

                switch (mode) {
                case Mode_Gen:
                case Mode_Pipe:
                {
                    if (g_options.insert_output_syncseq_first_offset != math::uint32_max) {
                        if (g_options.insert_output_syncseq_first_offset >= g_options.stream_byte_size * 8) {
                            _ftprintf(stderr, _T("error: insert_output_syncseq_first_offset must be less than bit stream length: stream_bit_size=%u insert_output_syncseq_first_offset=%u\n"),
                                g_options.stream_byte_size * 8, g_options.insert_output_syncseq_first_offset);
                            return 255;
                        }

                        if (!g_options.insert_output_syncseq_period) {
                            _ftprintf(stderr, _T("error: insert_output_syncseq_period must be positive\n"));
                            return 255;
                        }

                        if (g_options.insert_output_syncseq_end_offset != math::uint32_max) {
                            if (g_options.insert_output_syncseq_first_offset >= g_options.insert_output_syncseq_end_offset) {
                                _ftprintf(stderr, _T("error: insert_output_syncseq_first_offset must be less than insert_output_syncseq_end_offset: insert_output_syncseq_first_offset=%u insert_output_syncseq_end_offset=%u\n"),
                                    g_options.insert_output_syncseq_first_offset, g_options.insert_output_syncseq_end_offset);
                                return 255;
                            }
                        }
                    }
                } break;
                }

                // parse gen token
                uint32_t gen_bit_shift = math::uint32_max;
                uint32_t gen_combination_variant = math::uint32_max;

                if (!g_options.gen_token.empty()) {
                    tsscanf(g_options.gen_token.c_str(), _T("%u-%u"), &gen_bit_shift, &gen_combination_variant);
                }

                const uint32_t baud_capacity = (uint32_t(0x01) << g_options.bits_per_baud);
                const uint32_t baud_mask = baud_capacity - 1;

                std::vector<uint8_t> baud_alphabet_current_sequence;
                std::vector<std::vector<uint8_t> > baud_alphabet_sequences;

                std::vector<uint8_t> baud_alphabet_start_sequence;

                uint32_t num_baud_alphabet_sequences = 1;

                switch (mode) {
                case Mode_Gen:
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    // CAUTION: factorial => (2^3)! = 8! = 40320 ( (2^4)! = 16! = 20922789888000 - greater than 32 bits! )
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

                    baud_alphabet_start_sequence.resize(baud_capacity);

                    for (uint32_t i = 0; i < baud_capacity; i++) {
                        baud_alphabet_start_sequence[i] = i;
                    }

                    if (g_options.output_file_dir.empty()) {
                        g_options.output_file_dir = utility::get_parent_path(g_options.input_file);
                    }
                } break;
                }

                switch (mode) {
                case Mode_Gen:
                {
                    // Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
                    //
                    const uint32_t padded_stream_byte_size = g_options.stream_byte_size + 3;

                    GenData gen_data{
                        BasicData{ &g_flags, &g_options, mode, tee_file_in_handle },
                        &baud_alphabet_start_sequence,
                        nullptr,
                        baud_mask,
                        baud_capacity,
                        0,
                        StreamParams{ padded_stream_byte_size, 0, 0 },
                        NoiseParams{}
                    };

                    ReadFileChunkData read_file_chunk_data{ mode, &gen_data };

                    // CAUTION:
                    //  We must additionally shift file on `bits_per_baud - 1` times!
                    //
                    for (uint32_t j = 0; j < g_options.bits_per_baud; j++) {
                        if (gen_bit_shift != math::uint32_max && gen_bit_shift != j) {
                            continue;
                        }

                        for (uint32_t i = 1; i < num_baud_alphabet_sequences; i++) {
                            if (gen_combination_variant != math::uint32_max && gen_combination_variant != i) {
                                continue;
                            }

                            tackle::path_tstring out_file{ g_options.output_file_dir / utility::get_file_name_stem(g_options.input_file) + _T(".") +
                                utility::int_to_dec(j, 1, utility::tag_tstring{}) + _T("-") + utility::int_to_dec(i, 2, utility::tag_tstring{}) +
                                boost::fs::path{ g_options.input_file.str() }.extension().tstring() };

                            gen_data.baud_alphabet_end_sequence = &baud_alphabet_sequences[i];
                            gen_data.stream_params.last_bit_offset = 0;
                            gen_data.shifted_bit_offset = j;

                            gen_data.file_out_handle = utility::recreate_file(out_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, _T("wb"), utility::SharedAccess_DenyWrite);

                            fseek(file_in_handle.get(), 0, SEEK_SET);
                            tackle::file_reader<TCHAR>(file_in_handle, read_file_chunk).do_read(&read_file_chunk_data, { g_options.stream_byte_size }, padded_stream_byte_size);

                            fmt::print(
                                _T("#{:d}-{:d}: `{:s}`:\n"), j, i, out_file.c_str());

                            for (uint32_t k = 0; k < baud_capacity; k++) {
                                const uint32_t from_baud = baud_alphabet_start_sequence[k];
                                const uint32_t to_baud = (*gen_data.baud_alphabet_end_sequence)[k];
                                if (from_baud != to_baud) {
                                    fmt::print(
                                        _T("  {0:#0{2}b} -> {1:#0{2}b}\n"), from_baud, to_baud, 2 + g_options.bits_per_baud);
                                }
                            }

                            ret = 0;
                        }
                    }
                } break;

                case Mode_Sync:
                {
                    // Buffer must be padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block.
                    // Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
                    //
                    const uint32_t padded_stream_byte_size = ((g_options.stream_byte_size + 3) & ~uint32_t(3)) + 4;

                    SyncData sync_data{
                        BasicData{ &g_flags, &g_options, mode, tee_file_in_handle },
                        math::uint32_max,
                        StreamParams{ padded_stream_byte_size, 0, 0 },
                        NoiseParams{},
                        CorrInParams{
                            g_options.impl_token,
                            g_options.corr_mm,
                            uint64_t(g_options.stream_byte_size) * 8,
                            g_options.syncseq_bit_size,
                            g_options.corr_min,
                            g_options.corr_mean_min,
                            g_options.stream_min_period,
                            g_options.stream_max_period,
                            g_options.syncseq_min_repeat,
                            g_options.syncseq_max_repeat,
                            g_options.max_periods_in_offset,
                            g_options.max_corr_values_per_period,
                            size_t(g_options.corr_mean_buf_max_size_mb * 1024 * 1024), // 4GB max
                            g_flags.no_zero_corr,
                            g_flags.use_linear_corr,
                            g_flags.skip_calc_on_filtered_corr_value_use,
                            g_flags.skip_max_weighted_sum_of_corr_mean_calc,
                            g_flags.sort_at_first_by_max_corr_mean,
                            g_flags.return_sorted_result
                        },
                        CorrInOutParams{
                            g_options.syncseq_int32         // CAUTION: bit length can be greater than `syncseq_bit_size`
                        },
                        CorrOutParams{}
                    };

                    ReadFileChunkData read_file_chunk_data{ mode, &sync_data };

                    fseek(file_in_handle.get(), 0, SEEK_SET); // just in case
                    tackle::file_reader<TCHAR>(file_in_handle, read_file_chunk).do_read(&read_file_chunk_data, { g_options.stream_byte_size }, padded_stream_byte_size);

                    std::tstring offset_suffix_msg_str;

                    if (sync_data.corr_out_params.input_inconsistency) {
                        offset_suffix_msg_str += _T(" [INPUT INCONSISTENCY]");
                    }
                    if (sync_data.corr_out_params.accum_corr_mean_quit) {
                        offset_suffix_msg_str += _T(" [UNCERTAIN]");
                    }

                    const std::tstring offset_prefix_warn_str = !offset_suffix_msg_str.empty() ? _T("[!] ") : _T("    ");

                    const CONSTEXPR size_t indent_size = UTILITY_CONSTEXPR_STRING_LENGTH("                             ");

                    // mean parameters
                    const std::tstring mean_params =
                        sync_data.corr_out_params.accum_corr_mean_calc ?
                            fmt::format(
                                _T("corr mean min/used/max:        {:s} / {:#06f} / {:#06f}\n"),
                                sync_data.corr_out_params.min_corr_mean != math::float_max ?
                                    fmt::format(_T("{:#06f}"), sync_data.corr_out_params.min_corr_mean) :
                                    _T("-"),
                                sync_data.corr_out_params.used_corr_mean,
                                sync_data.corr_out_params.max_corr_mean) :
                            _T("");

                    // mean deviation parameters
                    const std::tstring mean_deviat_params =
                        sync_data.corr_in_params.impl_token == Impl::impl_min_sum_of_corr_mean_deviat ?
                            fmt::format(
                                _T("corr mean deviat min/max:      {:#06f} / {:#06f}\n"),
                                sync_data.corr_out_params.min_corr_mean_deviat,
                                sync_data.corr_out_params.max_corr_mean_deviat) :
                            _T("");

                    // mean memory parameters
                    const std::tstring mean_mem_params =
                        sync_data.corr_out_params.accum_corr_mean_calc ?
                            fmt::format(
                                _T(
                                    "num corr means calc/iter:      {:d} / {:d}\n"
                                    "corr means mem accum/max:      {:d} / {:d} Kb\n"
                                    "corr means mem used:           {:d} Kb\n"
                                ),
                                sync_data.corr_out_params.num_corr_means_calc, sync_data.corr_out_params.num_corr_means_iterated,
                                (sync_data.corr_out_params.accum_corr_mean_bytes + 1023) / 1024, sync_data.corr_in_params.max_corr_mean_bytes / 1024,
                                (sync_data.corr_out_params.used_corr_mean_bytes + 1023) / 1024) :
                            _T("");

                    // modification flag params
                    std::tstring mod_flag_params;

                    for (const auto & mod_flag : mod_flags) {
                        mod_flag_params += fmt::format(_T("  {:>{}s}{:s}\n"), _T(""), indent_size, mod_flag);
                    }

                    // calculate time params
                    std::tstring calc_time_params;

                    for (const auto & calc_time_phase : sync_data.corr_out_params.calc_time_phases) {
                        calc_time_params += fmt::format(
                            _T("  {:<{}s}{:#3d} | {:.3f}\n"),
                            calc_time_phase.phase_name + _T(":"), indent_size,
                            uint32_t(calc_time_phase.calc_time_all_dur_fract * 100), // rounding to lowest
                            calc_time_phase.calc_time_dur_sec);
                    }

                    fmt::print(
                        _T(
                            "impl token:                    {:d} / {:s}\n"
                            "corr multiply method:          {:d} / {:s}\n"
                            "syncseq length/value:          {:d} / {:#010x}\n"
                            "offset:                    {:s}{:s}{:s}\n"                     // CAUTION: can be greater than stream width/period because of noise or synchronous sequence change in the input data!
                            "period (width):                {:s}\n"
                            "stream bit length:             {:d}\n"
                            "max periods {{in}} offset:       {:d}\n"
                            "period {{in}} min/max:           {:d} / {:s}\n"
                            "period {{io}} min/max:           {:d} / {:d}\n"
                            "period min/used/max repeat:    {:d} / {:d} / {:d}\n"
                            "user corr value/mean min:      {:#06f} / {:#06f}\n"
                            "corr value min/max:            {:s} / {:#06f}\n"
                            "{:s}"                                                          // used when correlation mean values calculation algorithm is enabled
                            "{:s}"                                                          // used when correlation mean deviation values calculation algorithm is enabled
                            "max corr values per period:    {:d}\n"
                            "num corr values calc/iter:     {:d} / {:d}\n"
                            "{:s}"                                                          // used when correlation mean values calculation algorithm is enabled
                            "input noise pttn bits/prob:    {:s}\n"
                            "{:s}{:s}"                                                      // used when algorithm modification flags is used
                            "calc phase times {{% | sec}}:\n"
                            "{:s}\n"
                        ),
                        sync_data.corr_in_params.impl_token, !g_options.impl_token_str.empty() ? g_options.impl_token_str : std::tstring{ _T("-") },
                        sync_data.corr_in_params.corr_mm, !g_options.corr_mm_token_str.empty() ? g_options.corr_mm_token_str : std::tstring{ _T("-") },
                        g_options.syncseq_bit_size, sync_data.corr_io_params.syncseq_int32,
                        offset_prefix_warn_str,
                        sync_data.syncseq_bit_offset != math::uint32_max ?
                            std::to_tstring(sync_data.syncseq_bit_offset) :
                            _T("-"),
                        offset_suffix_msg_str,
                        sync_data.stream_params.stream_width != math::uint32_max ?
                            std::to_tstring(sync_data.stream_params.stream_width) :
                            _T("-"),
                        g_options.stream_byte_size * 8,
                        sync_data.corr_in_params.max_periods_in_offset,
                        sync_data.corr_in_params.min_period,
                        sync_data.corr_in_params.max_period != math::uint32_max ?
                            std::to_tstring(sync_data.corr_in_params.max_period) :
                            _T("-"),
                        sync_data.corr_out_params.min_period, sync_data.corr_out_params.max_period,
                        sync_data.corr_in_params.period_min_repeat, sync_data.corr_out_params.period_used_repeat, sync_data.corr_in_params.period_max_repeat,
                        sync_data.corr_in_params.corr_min, sync_data.corr_in_params.corr_mean_min,
                        sync_data.corr_out_params.min_corr_value != math::float_max ?
                            fmt::format(_T("{:#06f}"), sync_data.corr_out_params.min_corr_value) :
                            _T("-"),
                        sync_data.corr_out_params.max_corr_value,
                        mean_params,
                        mean_deviat_params,
                        sync_data.corr_in_params.max_corr_values_per_period,
                        sync_data.corr_out_params.num_corr_values_calc,
                        sync_data.corr_out_params.num_corr_values_iterated,
                        mean_mem_params,
                        g_options.gen_input_noise_bit_block_size ?
                            fmt::format(_T("{:d} / {:#03d} %"), g_options.gen_input_noise_bit_block_size, g_options.gen_input_noise_block_bit_prob) :
                            _T("-"),
                        !mod_flag_params.empty() ?
                            _T("modification flags:\n") :
                            _T(""),
                        mod_flag_params,
                        calc_time_params);

                    ret = 0;
                } break;

                case Mode_Pipe:
                {
                    // Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
                    //
                    const uint32_t padded_stream_byte_size = g_options.stream_byte_size + 3;

                    PipeData pipe_data{
                        BasicData{ &g_flags, &g_options, mode, tee_file_in_handle },
                        StreamParams{ padded_stream_byte_size, 0, 0 },
                        NoiseParams{},
                        utility::recreate_file(g_options.output_file, IF_UNICODE(std::codecvt_utf8, std::codecvt_utf16)<wchar_t>{}, _T("wb"), utility::SharedAccess_DenyWrite)
                    };

                    ReadFileChunkData read_file_chunk_data{ mode, &pipe_data };

                    fseek(file_in_handle.get(), 0, SEEK_SET); // just in case
                    tackle::file_reader<TCHAR>(file_in_handle, read_file_chunk).do_read(&read_file_chunk_data, { g_options.stream_byte_size }, padded_stream_byte_size);
                } break;
                }
            }
            catch (const std::exception & ex)
            {
                _ftprintf(stderr, _T("%s: error: %hs\n"), argv[0], ex.what());
                ret = 255;
            }
            catch (...)
            {
                _ftprintf(stderr, _T("%s: error: unknown exception\n"), argv[0]);
                ret = 255;
            }

            return ret;
        }();
    }
    __finally {
        [&]() {
            //...
        }();
    }
    }();
}
