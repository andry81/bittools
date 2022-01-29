#include "main.hpp"


#define DEFAULT_SYNCSEQ_MAXIMAL_REPEAT_PERIOD   16

const TCHAR * g_flags_to_parse_arr[] = {
    _T("/stream-byte-size"), _T("/s"),
    _T("/syncseq-bit-size"), _T("/q"),
    _T("/syncseq-int32"), _T("/k"),
    _T("/syncseq-min-repeat"), _T("/r"),
    _T("/syncseq-max-repeat"), _T("/rmax"),
    _T("/stream-min-period"), _T("/spmin"),
    _T("/stream-max-period"), _T("/spmax"),
    _T("/gen-token"), _T("/g"),
    _T("/gen-input-noise"), _T("/inn"),
    _T("/tee-input"),
    _T("/autocorr-mean-min"),
    _T("/autocorr-mean-buf-max-size-mb"),
    _T("/disable-calc-autocorr-mean"),
};

const TCHAR * g_empty_flags_arr[] = {
    _T("")
};

inline int invalid_format_flag(const TCHAR * arg)
{
    _tprintf(_T("flag format is invalid: \"%s\"\n"), arg);
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
int parse_arg_to_option(int & error, const TCHAR * arg, int argc, const TCHAR * argv[], int & arg_offset, Flags & flags, Options & options, const TCHAR * (& include_filter_arr)[N], const TCHAR * (& exclude_filter_arr)[M])
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
    if (is_arg_equal_to(arg, _T("/autocorr-mean-min"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.autocorr_mean_min = utility::str_to_float(std::tstring{ arg });
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/autocorr-mean-buf-max-size-mb"))) {
        arg_offset += 1;
        if (argc >= arg_offset + 1 && (arg = argv[arg_offset])) {
            if (is_arg_in_filter(start_arg, include_filter_arr)) {
                options.autocorr_mean_buf_max_size_mb = _ttoi(arg);
                return 1;
            }
            return 0;
        }
        else error = invalid_format_flag(start_arg);
        return 2;
    }
    if (is_arg_equal_to(arg, _T("/disable-calc-autocorr-mean"))) {
        if (is_arg_in_filter(start_arg, include_filter_arr)) {
            flags.disable_calc_autocorr_mean = true;
            return 1;
        }
        return 0;
    }

    return -1;
}

namespace boost {
    namespace fs = filesystem;
}

int _tmain(int argc, const TCHAR * argv[])
{
    PWSTR cmdline_str = GetCommandLine();

    size_t arg_offset_begin = 0;

    if (argv[0][0] != _T('/')) { // arguments shift detection
        arg_offset_begin = 1;
    }

    int ret = 127; // not found

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

                    if ((parse_result = parse_arg_to_option(parse_error, arg, argc, argv, arg_offset, g_flags, g_options, g_flags_to_parse_arr, g_empty_flags_arr)) >= 0) {
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
                case Mode_Gen:
                case Mode_Sync:
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

                switch (mode) {
                case Mode_Gen:
                case Mode_Sync:
                case Mode_Gen_Sync:
                {
                    if (g_options.stream_byte_size && g_options.stream_byte_size <= 4) {
                        _ftprintf(stderr, _T("error: stream_byte_size must be greater than 4 bytes\n"));
                        return 255;
                    }

                    if (g_options.stream_byte_size >= math::uint32_max / 2) {
                        _ftprintf(stderr, _T("error: stream_byte_size is too big: stream_byte_size=%u\n"),
                            g_options.stream_byte_size);
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

                    if (g_options.stream_min_period != math::uint32_max && g_options.stream_min_period >= g_options.stream_byte_size * 8) {
                        _ftprintf(stderr, _T("error: stream_min_period must be less than stream_byte_size: stream_min_period=%u stream_byte_size=%u\n"),
                            g_options.stream_min_period, g_options.stream_byte_size);
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

                    if (!g_options.syncseq_int32) {
                        _ftprintf(stderr, _T("error: syncseq_bytes must not 0\n"));
                        return 255;
                    }

                    if (g_options.syncseq_min_repeat != math::uint32_max && !g_options.syncseq_min_repeat) {
                        _ftprintf(stderr, _T("error: syncseq_min_repeat must be positive\n"));
                        return 255;
                    }

                    if (g_options.syncseq_max_repeat != math::uint32_max && !g_options.syncseq_max_repeat) {
                        _ftprintf(stderr, _T("error: syncseq_max_repeat must be positive\n"));
                        return 255;
                    }

                    if (g_options.syncseq_max_repeat != math::uint32_max && g_options.syncseq_max_repeat < g_options.syncseq_min_repeat) {
                        _ftprintf(stderr, _T("error: syncseq_min_repeat must be not greater than syncseq_max_repeat: syncseq_min_repeat=%u syncseq_max_repeat=%u\n"),
                            g_options.syncseq_min_repeat, g_options.syncseq_max_repeat);
                        return 255;
                    }

                    if (g_options.syncseq_max_repeat == math::uint32_max) {
                        if (g_options.syncseq_min_repeat != math::uint32_max && g_options.syncseq_min_repeat > DEFAULT_SYNCSEQ_MAXIMAL_REPEAT_PERIOD) {
                            g_options.syncseq_max_repeat = g_options.syncseq_min_repeat;
                        }
                        else {
                            g_options.syncseq_max_repeat = DEFAULT_SYNCSEQ_MAXIMAL_REPEAT_PERIOD;
                        }
                    }

                    if (g_options.syncseq_bit_size < g_options.bits_per_baud || g_options.syncseq_bit_size < 2) {
                        _ftprintf(stderr, _T("error: syncseq_bit_size must be greater or equal to bits_per_baud and be at least 2: syncseq_bit_size=%u bits_per_baud=%u\n"),
                            g_options.syncseq_bit_size, g_options.bits_per_baud);
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

                if (g_options.autocorr_mean_min < 0 || g_options.autocorr_mean_min > 1.0) {
                    _ftprintf(stderr, _T("error: autocorr_mean_min must be in range [0; 1]: autocorr_mean_min=%f\n"), g_options.autocorr_mean_min);
                    return 255;
                }

                if (!g_options.autocorr_mean_buf_max_size_mb) {
                    _ftprintf(stderr, _T("error: autocorr_mean_buf_max_size_mb must be positive.\n"));
                    return 255;
                }

                if (g_options.input_file.empty() || !utility::is_regular_file(g_options.input_file, false)) {
                    _ftprintf(stderr, _T("error: input file is not found: \"%s\"\n"), g_options.input_file.c_str());
                    return 255;
                }

                tackle::file_handle<TCHAR> tee_file_in_handle;

                if (!g_options.tee_input_file.empty()) {
                    tee_file_in_handle = utility::open_file(g_options.tee_input_file, _T("wb"), utility::SharedAccess_DenyWrite, 0, 0);
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

                    if (g_options.output_file_dir != _T(".") && !utility::is_directory_path(g_options.output_file_dir, false)) {
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

                    if (utility::is_regular_file(g_options.output_file, false) && utility::is_same_file(g_options.input_file, g_options.output_file, false)) {
                        _ftprintf(stderr, _T("error: output file must not be input file: \"%s\"\n"), g_options.output_file.c_str());
                        return 255;
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

                const tackle::file_handle<TCHAR> file_in_handle = utility::open_file(g_options.input_file, _T("rb"), utility::SharedAccess_DenyWrite);

                switch (mode) {
                case Mode_Gen:
                {
                    if (!g_options.stream_byte_size) {
                        g_options.stream_byte_size = uint32_t((std::min)(utility::get_file_size(file_in_handle), uint64_t(math::uint32_max)));
                    }

                    // Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
                    //
                    const uint32_t padded_stream_byte_size = g_options.stream_byte_size != math::uint32_max ?
                        g_options.stream_byte_size + 3 : math::uint32_max;

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

                            gen_data.file_out_handle = utility::recreate_file(out_file, _T("wb"), utility::SharedAccess_DenyWrite);

                            fseek(file_in_handle.get(), 0, SEEK_SET);
                            tackle::file_reader<TCHAR>(file_in_handle, read_file_chunk).do_read(&read_file_chunk_data, { g_options.stream_byte_size }, padded_stream_byte_size);

                            fmt::print(
                                _T("#{:d}-{:d}: `{:s}`:\n"), j, i, out_file.c_str());

                            for (uint32_t k = 0; k < baud_capacity; k++) {
                                const uint32_t from_baud = baud_alphabet_start_sequence[k];
                                const uint32_t to_baud = (*gen_data.baud_alphabet_end_sequence)[k];
                                if (from_baud != to_baud) {
                                    fmt::print(
                                        _T("  {0:#{2}b} -> {1:#{2}b}\n"), from_baud, to_baud, g_options.bits_per_baud);
                                }
                            }

                            ret = 0;
                        }
                    }
                } break;

                case Mode_Sync:
                {
                    if (!g_options.stream_byte_size) {
                        g_options.stream_byte_size = uint32_t((std::min)(utility::get_file_size(file_in_handle), uint64_t(math::uint32_max)));
                    }

                    // Buffer must be padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block.
                    // Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
                    //
                    const uint32_t padded_stream_byte_size = g_options.stream_byte_size != math::uint32_max ?
                        ((g_options.stream_byte_size + 3) & ~uint32_t(3)) + 4 : math::uint32_max;

                    SyncData sync_data{
                        BasicData{ &g_flags, &g_options, mode, tee_file_in_handle },
                        math::uint32_max,
                        StreamParams{ padded_stream_byte_size, 0, 0 },
                        NoiseParams{},
                        AutocorrParams{}
                    };

                    ReadFileChunkData read_file_chunk_data{ mode, &sync_data };

                    fseek(file_in_handle.get(), 0, SEEK_SET); // just in case
                    tackle::file_reader<TCHAR>(file_in_handle, read_file_chunk).do_read(&read_file_chunk_data, { g_options.stream_byte_size }, padded_stream_byte_size);

                    if (sync_data.syncseq_bit_offset != math::uint32_max) {
                        ret = 0;
                        fmt::print(
                            "Synchro sequence offset: {:d}\n"   // CAUTION: can be greater than stream width/period because of noise or synchronous sequence change in the input data!
                            "Stream width/period:     {:d}\n"
                            "Autocorrelation value:   {:#06f}\n"
                            "Autocorrelation mean:    {:#06f}\n",
                            sync_data.syncseq_bit_offset,
                            sync_data.stream_params.stream_width,
                            sync_data.autocorr_params.value,
                            sync_data.autocorr_params.mean);
                    }
                } break;

                case Mode_Pipe:
                {
                    if (!g_options.stream_byte_size) {
                        g_options.stream_byte_size = uint32_t((std::min)(utility::get_file_size(file_in_handle), uint64_t(math::uint32_max)));
                    }

                    // Buffer must be padded to 3 bytes remainder to be able to read and shift the last 8-bit block as 32-bit block.
                    //
                    const uint32_t padded_stream_byte_size = g_options.stream_byte_size != math::uint32_max ?
                        g_options.stream_byte_size + 3 : math::uint32_max;

                    PipeData pipe_data{
                        BasicData{ &g_flags, &g_options, mode, tee_file_in_handle },
                        StreamParams{ padded_stream_byte_size, 0, 0 },
                        NoiseParams{},
                        utility::recreate_file(g_options.output_file, _T("wb"), utility::SharedAccess_DenyWrite)
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
