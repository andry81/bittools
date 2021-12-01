#pragma once

#include "common.hpp"

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

    //...
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
    uint32_t                syncseq_bit_size;
    uint32_t                syncseq_int32;
    uint32_t                syncseq_repeat;
    uint32_t                bits_per_baud;
    std::tstring            gen_token;
    tackle::path_tstring    input_file;
    tackle::path_tstring    output_file_dir;
};

enum Mode
{
    Mode_None           = 0,
    Mode_Gen            = 1,
    Mode_Sync           = 2,
    Mode_Gen_Sync       = 3,    // TODO: generate into memory instead of into files and sync with each generated file
};

struct BasicData
{
    Flags *                         flags_ptr;
    Options *                       options_ptr;
    Mode                            mode;
};

struct GenData : BasicData
{
    uint32_t                        padded_stream_byte_size;
    std::vector<uint8_t> *          baud_alphabet_start_sequence;
    std::vector<uint8_t> *          baud_alphabet_end_sequence;
    uint32_t                        bits_per_baud;
    uint32_t                        baud_mask;
    uint32_t                        baud_capacity;
    uint32_t                        shifted_bit_offset;
    tackle::file_handle<TCHAR>      file_out_handle;
    bool                            has_stream_byte_size;
};

struct SyncData : BasicData
{
    uint32_t                        padded_stream_byte_size;
    uint32_t                        syncseq_bit_size;
    uint32_t                        syncseq_int32;
    uint32_t                        syncseq_mask;
    uint32_t                        syncseq_repeat;
    uint32_t                        syncseq_bit_offset;
};

struct SyncSeqHits
{
    uint32_t                first_bit_offset;
    std::vector<uint32_t>   next_bit_offsets;
};

void translate_buffer(GenData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size);
void search_synchro_sequence(SyncData & data, tackle::file_reader_state & state, uint8_t * buf, uint32_t size);
void read_file_chunk(uint8_t * buf, uint64_t size, void * user_data, tackle::file_reader_state & state);
