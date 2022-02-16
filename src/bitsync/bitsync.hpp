#pragma once

#include "common.hpp"

#include "autocorrelation.hpp"

#include "tacklelib/utility/utility.hpp"
#include "tacklelib/utility/assert.hpp"
#include "tacklelib/utility/math.hpp"

#include "tacklelib/tackle/file_reader.hpp"

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <fmt/format.h>

#include <string>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>


namespace tackle {
#ifdef _UNICODE
    using path_tstring              = path_wstring;
    using tag_native_path_tstring   = tag_path_wstring_bs;
#else
    using path_tstring              = path_string;
    using tag_native_path_tstring   = tag_path_string_bs;
#endif
}

namespace utility {
#ifdef _UNICODE
    using tag_tstring = utility::tag_wstring;
#else
    using tag_tstring = utility::tag_string;
#endif
}

struct Flags
{
    Flags();
    Flags(const Flags &) = default;
    Flags(Flags &&) = default;

    Flags & operator =(const Flags &) = default;
    //Flags && operator =(Flags &&) = default;

    //void merge(const Flags & flags);
    void clear();

    bool disable_calc_autocorr_mean;
    bool insert_output_syncseq_instead_fill;
};

struct Options
{
    Options();
    Options(const Options &) = default;
    Options(Options &&) = default;

    //void merge(const Options & options);
    void clear();

    Options & operator =(const Options &) = default;
    //Options && operator =(Options &&) = default;

    std::tstring            mode;
    uint32_t                stream_byte_size;
    uint64_t                stream_bit_size;
    uint32_t                stream_min_period;
    uint32_t                stream_max_period;
    uint32_t                max_periods_in_offset;
    uint32_t                syncseq_bit_size;
    uint32_t                syncseq_int32;
    uint32_t                syncseq_min_repeat;
    uint32_t                syncseq_max_repeat;
    uint32_t                bits_per_baud;
    std::tstring            gen_token;
    uint32_t                gen_input_noise_bit_block_size;     // 0-32767, 0 - don't generate
    uint32_t                gen_input_noise_block_bit_prob;     // 1-100
    uint32_t                insert_output_syncseq_first_offset;
    uint32_t                insert_output_syncseq_end_offset;   // excluding
    uint32_t                insert_output_syncseq_period;
    uint32_t                insert_output_syncseq_period_repeat;
    float                   autocorr_min;
    uint64_t                autocorr_mean_buf_max_size_mb;
    tackle::path_tstring    input_file;
    tackle::path_tstring    tee_input_file;
    tackle::path_tstring    output_file_dir;
    tackle::path_tstring    output_file;
};

enum Mode
{
    Mode_None           = 0,
    Mode_Gen            = 1,
    Mode_Sync           = 2,
    Mode_Gen_Sync       = 3,    // TODO: generate into memory instead of into files and sync with each generated file
    Mode_Pipe           = 4,
};

struct BasicData
{
    Flags *                         flags_ptr;
    Options *                       options_ptr;
    Mode                            mode;
    tackle::file_handle<TCHAR>      tee_file_in_handle;
};

struct StreamParams
{
    uint32_t                        padded_stream_byte_size;
    uint64_t                        last_bit_offset;
    uint32_t                        stream_width;
};

struct NoiseParams
{
    uint64_t                        last_gen_noise_bits_block_index;
};

struct GenData
{
    BasicData                       basic_data;

    std::vector<uint8_t> *          baud_alphabet_start_sequence;
    std::vector<uint8_t> *          baud_alphabet_end_sequence;
    uint32_t                        baud_mask;
    uint32_t                        baud_capacity;
    uint32_t                        shifted_bit_offset;
    StreamParams                    stream_params;
    NoiseParams                     noise_params;
    tackle::file_handle<TCHAR>      file_out_handle;
};

struct SyncData
{
    BasicData                       basic_data;

    uint64_t                        syncseq_bit_offset; // CAUTION: can be greater than stream width/period because of noise or synchronous sequence change in the input data!
    StreamParams                    stream_params;
    NoiseParams                     noise_params;
    AutocorrInParams                autocorr_in_params;
    AutocorrInOutParams             autocorr_io_params;
};

struct PipeData
{
    BasicData                       basic_data;

    StreamParams                    stream_params;
    NoiseParams                     noise_params;
    tackle::file_handle<TCHAR>      file_out_handle;
};

struct ReadFileChunkData
{
    Mode                            mode;
    void *                          mode_data;
};

void generate_stream(GenData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size);
void pipe_stream(PipeData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size);
void search_synchro_sequence(SyncData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size);
void read_file_chunk(uint8_t * buf, uint64_t size, void * user_data, tackle::file_reader_state & state);
