#pragma once

#include "common.hpp"

#include <vector>
#include <deque>
#include <cstdlib>


struct SyncseqAutocorr
{
    float       corr_mean;      // autocorrelation mean (average) value
    uint32_t    num_corr;       // number of available correlations used to calculate mean value
    uint64_t    offset;         // stream offset
    uint32_t    period;         // stream width/period
};

struct SyncseqAutocorrStats
{
    SyncseqAutocorr autocorr;
    bool            is_true;    // true position flag
};

void generate_autocorr_complement_func_from_bitset(
    uint32_t value, uint32_t value_bit_size, std::vector<uint32_t> & out_ref, size_t offset);

uint64_t multiply_autocorr_funcs(
    const uint32_t * in0_ptr, const uint32_t * in1_ptr, size_t size);

float autocorr_value(
    const uint32_t * in0_ptr, const uint32_t * in1_ptr, size_t size, uint64_t max_in0, uint64_t max_in1);

void calculate_syncseq_autocorrelation(
    uint8_t *                               stream_buf,                         // buffer must be padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block
    uint64_t                                stream_bit_size,
    uint32_t                                stream_min_period,
    uint32_t                                stream_max_period,
    uint32_t                                syncseq_bit_size,
    uint32_t                                syncseq_bytes,
    uint32_t                                syncseq_min_repeat,                 // synchro sequence min/max repeat quantity to calculate autocorrelation mean values
    uint32_t                                syncseq_max_repeat,
    std::vector<float> &                    autocorr_values_arr,                // autocorrelation values per each stream bit within synchro sequence bit size
    uint64_t                                autocorr_mean_buf_max_size,
    std::deque<SyncseqAutocorr> *           autocorr_mean_deq_ptr);             // resulted synchro sequence offset and period variants sorted from highest autocorrelation mean (average) value

void calculate_syncseq_autocorrelation_false_positive_stats(
    const std::vector<float> &              autocorr_values_arr,                // calculated autocorrelation values in range (0; 1]
    const std::deque<SyncseqAutocorr> *     autocorr_mean_deq_ptr,              // calculated autocorrelation mean values in range (0; 1], sorted from correlation mean maximum value to minimum
    const std::vector<uint32_t> &           true_positions_index_arr,           // true positions (indexes) in the stream
    size_t &                                true_num,                           // number of true positions
    size_t                                  stat_arrs_size,                     // sizes of all output statistic arrays
    std::vector<float> &                    false_max_corr_arr,                 // false positive correlation, sorted from maximum to minimum
    std::vector<size_t> &                   false_max_index_arr,                // false positive positions
    std::vector<float> &                    true_max_corr_arr,                  // true position correlation values, sorted from minimum to maximum
    std::vector<size_t> &                   true_max_index_arr,                 // true positions
    std::vector<float> &                    false_in_true_max_corr_arr,         // false positive correlation values within true positions, true positions is zeroed for convenience, sorted from maximum to minimum
    std::vector<size_t> &                   false_in_true_max_index_arr,        // false positive positions within true positions, true positions is zeroed for convenience
    std::vector<float> &                    saved_true_in_false_max_corr_arr,   // true position correlation values shifted out of false_in_true_max_* arrays because out of space
    std::vector<size_t> &                   saved_true_in_false_max_index_arr,
    std::vector<SyncseqAutocorrStats> *     false_in_true_max_corr_mean_arr_ptr); // false positive correlation mean values within true positions, true positions is zeroed for convenience, sorted from correlation mean maximum value to minimum
