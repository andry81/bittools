#pragma once

#include "common.hpp"

#include <vector>
#include <deque>
#include <cstdlib>

struct AutocorrInParams
{
    uint64_t                        stream_bit_size;
    uint32_t                        syncseq_bit_size;
    float                           corr_min_value;                 // used for both the autocorrelation min value and the autocorrelation min mean value
    uint32_t                        period_min_repeat;              // period min/max repeat quantity to calculate autocorrelation mean values
    uint32_t                        period_max_repeat;
    size_t                          max_corr_mean_bytes;
};

// 0 - unused
struct AutocorrInOutParams // input/output parameters
{
    AutocorrInOutParams() = default;
    AutocorrInOutParams(uint32_t syncseq_int32_, uint32_t min_period_, uint32_t max_period_)
    {
        *this = AutocorrInOutParams{}; // reuse initialization by value

        syncseq_int32 = syncseq_int32_;
        min_period = min_period_;
        max_period = max_period_;
    }
    AutocorrInOutParams(const AutocorrInOutParams &) = default;
    AutocorrInOutParams(AutocorrInOutParams &&) = default;

    AutocorrInOutParams & operator =(const AutocorrInOutParams &) = default;
    AutocorrInOutParams & operator =(AutocorrInOutParams &&) = default;

    uint32_t                        syncseq_int32;
    float                           used_corr_value;
    float                           min_corr_value;
    float                           max_corr_value;
    float                           used_corr_mean;
    float                           min_corr_mean;
    float                           max_corr_mean;
    uint32_t                        min_period;
    uint32_t                        max_period;
    uint32_t                        period_used_repeat;
    size_t                          num_corr_means_calc;            // number of all autocorrelation mean values calculated for all offsets and all periods in each offset
    size_t                          num_corr_values_iterated;       // number of iteration over all autocorrelation values, represents resulted O(n) time complexity for input conditions
    size_t                          used_corr_mean_bytes;           // all containers bytes used to store all autocorrelation mean values, represents overall memory usage
    size_t                          accum_corr_mean_bytes;          // Container bytes used to store accumulated autocorrelation mean values.
                                                                    // Represents resulted O(n) memory complexity for input conditions.
                                                                    // If this value hit the maximum, then autocorrelation mean values calculation algorithm does a quit.
    bool                            accum_corr_mean_quit;           // Indicates autocorrelation mean values calculation algorithm early quit.
};

struct SyncseqAutocorr
{
    uint32_t                        offset;                         // stream offset
    uint32_t                        period;                         // stream width/period
    uint32_t                        num_corr;                       // number of available correlations used to calculate mean value
    float                           corr_mean;                      // autocorrelation mean (average) value             (second phase calculation)
    //float                           corr_mean_weight;               // autocorrelation mean (average) values weight     (third phase calculation)
    float                           corr_mean_sum;                  // autocorrelation mean (average) values sum        (third phase calculation)
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
    const AutocorrInParams &                autocorr_in_params,
    AutocorrInOutParams &                   autocorr_io_params,
    uint8_t *                               stream_buf,                         // buffer must be padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block
    std::vector<float> &                    autocorr_values_arr,                // autocorrelation values per each stream bit within synchro sequence bit size
    std::deque<SyncseqAutocorr> *           autocorr_max_mean_deq_ptr);         // resulted synchro sequence offset and period variants sorted from highest autocorrelation max mean (max average for different offsets with the same period) value

void calculate_syncseq_autocorrelation_false_positive_stats(
    const std::vector<float> &              autocorr_values_arr,                // calculated autocorrelation values in range (0; 1]
    const std::deque<SyncseqAutocorr> *     autocorr_max_mean_deq_ptr,          // calculated autocorrelation max mean values in range (0; 1], sorted from correlation mean maximum value to minimum
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
