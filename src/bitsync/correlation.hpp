#pragma once

#include "common.hpp"

#include <vector>
#include <deque>
#include <cstdlib>

struct Impl
{
    enum impl_token
    {
        impl_unknown                                = -1,

        // major time complexity: N * N * ln(N)
        impl_max_weighted_sum_of_corr_mean          = 1,

        // major time complexity: N * N * ln(N)
        impl_min_sum_of_corr_mean_deviat            = 2,

        // major time complexity: N * N
        impl_max_weighted_autocorr_of_corr_values   = 3
    };

    enum corr_multiply_method
    {
        corr_muliply_unknown                        = -1,

        corr_muliply_inverted_xor_prime1033         = 1,

        corr_muliply_dispersed_value_prime1033      = 2
    };
};

struct CorrInParams
{
    Impl::impl_token                impl_token;
    Impl::corr_multiply_method      corr_mm;
    uint64_t                        stream_bit_size;
    uint32_t                        syncseq_bit_size;
    float                           corr_min;
    float                           corr_mean_min;
    uint32_t                        min_period;
    uint32_t                        max_period;
    uint32_t                        period_min_repeat;              // period min/max repeat quantity to calculate correlation mean values
    uint32_t                        period_max_repeat;
    uint32_t                        max_periods_in_offset;          // -1 = no limit, 0 = 1 period excluding first bit of 2d period, 1 = 1 period including first bit of 2d period, >1 = N periods including first bit of N+1 period
    uint32_t                        max_corr_values_per_period;
    size_t                          max_corr_mean_bytes;
    bool                            use_linear_corr;
    bool                            skip_calc_on_filtered_corr_value_use;
    bool                            skip_max_weighted_sum_of_corr_mean_calc;
    bool                            sort_at_first_by_max_corr_mean;
    bool                            return_sorted_result;
};

// 0 - unused
struct CorrInOutParams // input/output parameters
{
    CorrInOutParams() = default;
    CorrInOutParams(uint32_t syncseq_int32_)
    {
        *this = CorrInOutParams{}; // reuse initialization by value

        syncseq_int32 = syncseq_int32_;
    }
    CorrInOutParams(const CorrInOutParams &) = default;
    CorrInOutParams(CorrInOutParams &&) = default;

    CorrInOutParams & operator =(const CorrInOutParams &) = default;
    CorrInOutParams & operator =(CorrInOutParams &&) = default;

    uint32_t                        syncseq_int32;
};

struct CalcTimePhase
{
    std::tstring                    phase_name;
    double                          calc_time_dur_sec;
    float                           calc_time_all_dur_fract; // [0 - 1.0]
};

struct CorrOutParams // output only parameters
{
    CorrOutParams() = default;

    CorrOutParams(const CorrOutParams &) = default;
    CorrOutParams(CorrOutParams &&) = default;

    CorrOutParams & operator =(const CorrOutParams &) = default;
    CorrOutParams & operator =(CorrOutParams &&) = default;

    std::vector<CalcTimePhase>      calc_time_phases;

    float                           used_corr_value;
    float                           min_corr_value;
    float                           max_corr_value;
    float                           used_corr_mean;
    float                           min_corr_mean;
    float                           max_corr_mean;
    float                           min_corr_mean_deviat;
    float                           max_corr_mean_deviat;
    uint32_t                        min_period;
    uint32_t                        max_period;
    uint32_t                        period_used_repeat;

    size_t                          num_corr_values_calc;           // number of all correlation values calculated for a bit stream excluding filtered values by correlation minimum
    size_t                          num_corr_values_iterated;       // number of iteration over all correlation values
    size_t                          num_corr_means_calc;            // number of all correlation mean values calculated (if enabled) for all offsets and all periods in each offset excluding filtered values by correlation mean minimum
    size_t                          num_corr_means_iterated;        // number of iteration over all correlation mean values
    size_t                          used_corr_mean_bytes;           // all containers bytes used to store all correlation mean values, represents overall memory usage
    size_t                          accum_corr_mean_bytes;          // Container bytes used to store accumulated correlation mean values.
                                                                    // If this value hit the maximum, then correlation mean values calculation algorithm does a quit.
    bool                            input_inconsistency;            // indicates input inconsistency
    bool                            accum_corr_mean_calc;           // Indicates correlation mean values calculation.
    bool                            accum_corr_mean_quit;           // Indicates correlation mean values calculation algorithm early quit.
};

struct SyncseqCorr
{
    uint32_t                        offset;                         // stream offset
    uint32_t                        period;                         // stream width/period
    uint32_t                        num_corr;                       // number of available correlations used to calculate correlation value
    float                           corr_value;                     // correlation value
};

struct SyncseqCorrMean
{
    uint32_t                        offset;                         // stream offset
    uint32_t                        period;                         // stream width/period
    uint32_t                        num_corr;                       // number of available correlations used to calculate mean value
    float                           corr_mean;                      // correlation mean (average) value
    float                           corr_mean_sum;                  // correlation mean (average) values sum
};

struct SyncseqCorrMeanDeviat
{
    uint32_t                        offset;                         // stream offset
    uint32_t                        period;                         // stream width/period
    uint32_t                        num_corr;                       // number of available correlations used to calculate mean value
    float                           corr_mean;                      // correlation mean (average) value
    float                           corr_mean_deviat_sum;           // correlation deviation from mean (average) values sum
};

struct SyncseqCorrMeanStats
{
    SyncseqCorrMean                 corr;
    bool                            is_true;    // true position flag
};

struct SyncseqCorrMeanDeviatStats
{
    SyncseqCorrMeanDeviat           corr;
    bool                            is_true;    // true position flag
};

float multiply_bits(Impl::corr_multiply_method mm, uint32_t in0_value, uint32_t in1_value, size_t size);

float calculate_corr_value(Impl::corr_multiply_method mm, uint32_t in0_value, uint32_t in1_value, size_t size, float max_in0, float max_in1, bool make_linear_corr);

void calculate_syncseq_correlation(
    const CorrInParams &                    corr_in_params,
    CorrInOutParams &                       corr_io_params,
    CorrOutParams &                         corr_out_params,
    uint8_t *                               stream_buf,                         // buffer must be padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block
    std::vector<float> &                    corr_values_arr,                    // correlation values per stream bit
    std::vector<SyncseqCorr> &              corr_autocorr_arr,                  // resulted synchro sequence offset and period variants sorted at first for correlation max values (for min offset/period at second/third if enabled)
    std::deque<SyncseqCorrMean> &           corr_max_weighted_mean_sum_deq,     // resulted synchro sequence offset and period variants sorted at first for correlation max weighted mean sum (for min offset/period at second/third if enabled)
    std::deque<SyncseqCorrMeanDeviat> &     corr_min_mean_deviat_sum_deq);      // resulted synchro sequence offset and period variants sorted at first for correlation min mean deviation sum (for min offset/period at second/third if enabled)

void calculate_syncseq_correlation_false_positive_stats(
    const std::vector<float> &              corr_values_arr,                    // calculated correlation values in range (0; 1]
    const std::deque<SyncseqCorrMean> *     corr_max_weighted_mean_sum_deq_ptr, // calculated correlation max weighted mean sum
    const std::deque<SyncseqCorrMeanDeviat> * corr_min_mean_deviat_sum_deq_ptr, // calculated correlation min mean deviation sum
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
    std::vector<SyncseqCorrMeanStats> *     false_in_true_corr_max_weighted_mean_sum_arr_ptr,   // false positive correlation values within true positions, true positions is zeroed for convenience, sorted from correlation mean weighted sum maximum value to minimum
    std::vector<SyncseqCorrMeanDeviatStats> * false_in_true_corr_min_mean_deviat_sum_arr_ptr);  // false positive correlation values within true positions, true positions is zeroed for convenience, sorted from correlation mean deviation sum minimum value to maximum
