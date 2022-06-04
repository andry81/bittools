// Author: Andrey Dibrov (andry at inbox dot ru)
//

#include "correlation.hpp"

#include <boost/utility/binary.hpp>
#include <boost/scope_exit.hpp>

#include <tacklelib/utility/preprocessor.hpp>
#include <tacklelib/utility/preprocessor/if_break.hpp>
#include <tacklelib/utility/math.hpp>

#include <limits>
#include <cmath>
#include <chrono>


// The correlation function algorithm for a synchro sequence in a bit stream as nondeterministic not differentiable signal.
// Finds the synchro sequence most weighted offset and period in a bit stream.
//

// NOTE:
//  The "autocorrelation" function is a specialized version of a correlation function which does contain a multiplication of a signal function to itself or
//  has a comparison of lagged version of a signal function to itself.
//  Here is and after we do multiply a signal function to another function which is may or may not be a part of a signal function, so this is basically is
//  not an autocorrelation but just a correlation.
//

// THEORY:

//  Auto correlation function default range:
//
//      0 - minimal correlation value.
//     +1 - maximal positive correlation value.
//     -1 - maximal negative correlation value.
//

//  The general approach, conditions and properties:
//
//  1. Because our bit stream sequence does represent a nondeterministic not differentiable signal as a set of discrete values with undefined range,
//     then we must represent a bit stream together with a synchro sequence as a set of natural values greater or equal of 1 to build a function
//     with natural values (no need to generate real values from a bit set) to multiply functions.
//     In other words we have to generate more non bit values for comparison than the bit stream length instead of bit values from a bit stream.
//
//     But because the generator function already tested for stable results and can be reduced, then we can try to leave the length of a complement
//     function by the same size as the entire bit stream length.
//
//     The reason why we didn't do that does lay in the complement function generator, which still can generate quite different complement functions
//     even if an input bit set is shifted on 1 bit. And the difference is dependent on the index in a bit stream window, not the index from the bit
//     stream beginning.
//
//     So we still have to allocate `N * M` instead of `N` memory for correlation values storage, where N - length of a bit stream, M - length of
//     a complement function.
//
//     For the synchro sequence bit set we can allocate only `M`.
//     For this reason a complement function generator is exist as a standalone implementation.
//
//  2. Both functions basically will not infinitely increase or decrease, otherwise the different parts of the same function
//     can be relatively equal if min/max of both is normalized to the same range, but is different if not normalized.
//     Because of that we can compare without maximum and minimum normalization and can avoid a function rescale or resample.
//
//  3. The function multiplication to the exactly same function must gain the maximal correlation result from the range [0; 1], i.e. 1.
//     To achieve that we multiply one function to another and divide by a possible maximum.
//     For example, if absolute maximum one of the functions is greater, then we just divide by greater absolute maximum to normalize a
//     correlation value to range <= 1.
//
//  4. We must avoid a function [-inf; 0] range for natural numbers and [-inf; 1] for real numbers because:
//
//       Multiplication to a negative value can reduce the sum instead of increase it, so we may gain a lesser value related to a maximum.
//       In real numbers quantity a multiplication of a value from the range (0; 1) with a value from the range [1; +inf] may gain a lesser
//       absolute value.
//       Multiplication by 0 gets the lesser absolute value too.
//
//     To avoid reduction of the sum, we always y-shift all functions on +1 independently to a maximum and minimum to shift all values into
//     [1; +inf] range.
//
//  5. Because we don't need an inversed or maximal negative correlation range - `[-1; 0)` (an inversed synchro sequence does not need to be
//     found), then we search solutions only in the positive numbers quantity - `[1; +inf]`.
//
//  6. Because multiplication of functions does gain a square factor or quadratic value, then we may take the square root to return back to linear value.
//

//  The general formula of 2 functions multiplication:
//
//    fa(x) * fb(x) = sqrt( g(fa, fb, x) / max( g(fa, fa, x), g(fb, fb, x) ) )
//      , where:
//        g(fa, fb, i) = fa(i) * fb(i) = fa(0) * fb(0) + fa(1) * fb(1) + ... + fa(n-1) * fb(n-1)
//        i = [0; n - 1]
//          , where:
//            n               - length of a function,
//            f(i)            - a function value in range [1; +inf]
//            fa(x) * fb(x)   - normalized linear correlation value in range (0; 1]
//
//  The formula is based on the theorem:
//
//      fa * fb <= max(fa * fa, fb * fb), for any fa and fb with positive values in range [1; +inf]
//
//  Proof:
//
//    There is can be 2 variants of functions in the positive quarter: a not intersected and an intersected.
//
//  f(x)                      f(x)                      
//    ^  A                      ^                       
//    |  o--o        A          |  A           B        
//    |      \   -o--o          |X o--o     o--o X      
//    |  B    \ /               |      \   /            
//    |  o-    o                |       \ /             
//    |    \                    |        o              
//    |     o--o-               |       / \             
//    |          \   B          |  B   /   \   A        
//    |           o--o          |Y o--o     o--o Y      
//  0 +--+--+--+--+--+--- x   0 +--+--+--+--+--+--- x   
//    0  1  2  3  4  5          0  1  2  3  4  5        
//
//    For not intersected variant:
//
//      fa * fb = Ya(1) * Yb(1) + ... + Ya(n) * Yb(n)
//      fa * fa = Ya(1) * Ya(1) + ... + Ya(n) * Ya(n)
//      fb * fb = Yb(1) * Yb(1) + ... + Yb(n) * Yb(n)
//
//      , where: Ya(k) * Ya(k) >= Ya(k) * Yb(k) >= Yb(k) * Yb(k), k=[0; +inf]
//
//        then: fa * fa >= fa * fb >= fb * fb
//
//        that means: fa * fb <= max(fa * fa, fb * fb)
//
//    For intersected variant:
//
//      fa * fb = Ya(1) * Yb(1) + Ya(2) * Yb(2) + Ya(3) * Yb(3) + Ya(4) * Yb(4) + Ya(5) * Yb(5)
//      fa * fa = Ya(1) * Ya(1) + Ya(2) * Ya(2) + Ya(3) * Ya(3) + Ya(4) * Ya(4) + Ya(5) * Ya(5)
//      fb * fb = Yb(1) * Yb(1) + Yb(2) * Yb(2) + Yb(3) * Yb(3) + Yb(4) * Yb(4) + Yb(5) * Yb(5)
//
//      But because 2 functions splits each other in only one point, then we can rewrite multiplication fa * fb as fx * fy:
//
//      fa * fb = fa[0, k] * fb[0, k] + fa[k + 1, n] * fb[k + 1, n], k < n, k - intersection point
//
//      Or
//
//      fa * fb = fa(1) * fb(1) + fa(2) * fb(2) + fa(3) * fb(3) + fb(4) * fa(4) + fb(5) * fa(5), k = 3
//
//      , where: fa(k) * fb(k) = fb(k) * fa(k)
//
//        then: fa * fb = fa[0, k] * fb[0, k] + fb[k + 1, n] * fa[k + 1, n] = fx * fy
//
//      But because not intersected variant is already proved:
//
//        fa * fb <= max(fx * fx, fy * fy)
//
//      To continue the proof, we must split the intersected functions multiplication into segments, where
//      only the intersected part must be proved. All others segments can be reduced to a not intersected functions.
//
//      f(x)                    
//        ^   A                 
//        | p o        B        
//        |   .\      -o q      
//        |   . \    / .        
//        |   .   \X/  .        
//        |   a    o   b        
//        |   .   / \  .        
//        |   . /    \ .        
//        | C o       \.        
//        |   .        o D      
//        |   .        .        
//        |   c        d        
//        |   .        .        
//      0 +---+--------+--- x   
//        0                     
//
//      If the equation `fp * fq <= max(fp * fp, fq * fq)` is true
//      and triangles AXC and BXD are similar and rely on common angle and
//      parallel opposite edges, then the function multiplication can be reduced
//      to the equation is based on similar triangles:
//
//      fp * fq = (a + c) * c + (b + d) * d
//      fp * fp = (a + c) ^ 2 + d ^ 2
//      fq * fq = (b + d) ^ 2 + c ^ 2
//      a > 0, b > 0, c >= 1, d >= 1
//
//      then:
//
//        `(a + c) * c + (b + d) * d <= max((a + c) ^ 2 + d ^ 2, (b + d) ^ 2 + c ^ 2)` - must has a solution
//
//      To solve this we can use `Wolfram Alpha` online:
//
//        * `(a + c) * c + (b + d) * d <= max((a + c) ^ 2 + d ^ 2, (b + d) ^ 2 + c ^ 2), a > 0, b > 0, c >= 1, d >= 1`:
//
//          https://www.wolframalpha.com/input?i=%28a+%2B+c%29+*+c+%2B+%28b+%2B+d%29+*+d+%3C%3D+max%28%28a+%2B+c%29+%5E+2+%2B+d+%5E+2%2C+%28b+%2B+d%29+%5E+2+%2B+c+%5E+2%29%2C+a+%3E+0%2C+b+%3E+0%2C+c+%3E%3D+1%2C+d+%3E%3D+1
//
//          Solution:
//
//            a c + b d + c^2 + d^2 <=
//              1/2 a^2 sgn(-c^2 + (a + c)^2 + d^2 - (b + d)^2) + a^2/2 + 1/2 b^2 sgn(c^2 - (a + c)^2 - d^2 + (b + d)^2) + a c sgn(-c^2 + (a + c)^2 + d^2 - (b + d)^2) +
//              1/2 c^2 sgn(-c^2 + (a + c)^2 + d^2 - (b + d)^2) + 1/2 d^2 sgn(-c^2 + (a + c)^2 + d^2 - (b + d)^2) + 1/2 c^2 sgn(c^2 - (a + c)^2 - d^2 + (b + d)^2) +
//              1/2 d^2 sgn(c^2 - (a + c)^2 - d^2 + (b + d)^2) + b d sgn(c^2 - (a + c)^2 - d^2 + (b + d)^2) + a c + b^2/2 + b d + c^2 + d^2,
//            a > 0, b > 0, c >= 1, d >= 1
//
//            , where:
//
//              a, b, c, d  - real numbers
//              sgn(x)      - sign of x
//
//      Result:
//
//        A solution exist for complete set of intervals: `a > 0, b > 0, c >= 1, d >= 1`.
//        That means the equation is proved:              `fa * fb <= max(fa * fa, fb * fb)` - has complete solution and so is true.
//

// EXAMPLES:
//
// NOTE:
//  All functions below is already y-shifted on +1.
//
//
//  f1(x)                         f2(x)                         f3(x)                         f4(x)                    
//    ^                             ^                             ^                             ^                      
//    |                             |                             |                             |                      
//    |                             |                             |                           7 +           o          
//    |                             |                             |                             |          / \         
//    |                             |                             |                             |         /   \        
//  4 +        o                  4 +           o               4 +        o--o               4 +  o--o--o     o--o    
//    |       / \                   |          / \                |       /    \                |                      
//    |      /   \                  |         /   \               |      /      \               |                      
//  1 +  o--o     o--o--o         1 +  o--o--o     o--o         1 +  o--o        o--o         1 +                      
//  0 +--+--+--+--+--+--+-- x     0 +--+--+--+--+--+--+-- x     0 +--+--+--+--+--+--+-- x     0 +--+--+--+--+--+--+-- x
//    0  1  2  3  4  5  6           0  1  2  3  4  5  6           0  1  2  3  4  5  6           0  1  2  3  4  5  6    
//
//  f5(x)                                             f6(x)
//    ^                                                 ^
//    |                                                 |
//    |                                                 |
//    |                                               8 +                       o
//  6 +                       o                         |                      / \
//    |                      / \                        |                     /   \
//    |                     /   \                       |                    /     \
//    |                    /     \                      |                   /       \
//    |                   /       \                     |                  /         \
//    |                  /         \                  2 +     o-----o-----o           o-----o
//  1 +     o-----o-----o           o-----o             |
//  0 +-----+-----+-----+-----+-----+-----+--- x      0 +-----+-----+-----+-----+-----+-----+--- x
//    0     1     2     3     4     5     6             0     1     2     3     4     5     6
//
//  f2 is x-shifted of f1, f4 is y-shifted of f2, f5 is y-multiplied of f2, f6 is y-scaled of f2
//
//     f1 * f1 = 1*1 + 1*1 + 4*4 + 1*1 + 1*1 + 1*1 = 21
//     f2 * f2 = 1*1 + 1*1 + 1*1 + 4*4 + 1*1 + 1*1 = 21
//     f3 * f3 = 1*1 + 1*1 + 4*4 + 4*4 + 1*1 + 1*1 = 36
//     f4 * f4 = 4*4 + 4*4 + 4*4 + 7*7 + 4*4 + 4*4 = 129
//     f5 * f5 = 1*1 + 1*1 + 1*1 + 6*6 + 1*1 + 1*1 = 41
//     f6 * f6 = 2*2 + 2*2 + 2*2 + 8*8 + 2*2 + 2*2 = 84
//
//     f1 * f1 = 1*1 + 1*1 + 4*4 + 1*1 + 1*1 + 1*1 = 21
//     f1 * f2 = 1*1 + 1*1 + 4*1 + 1*4 + 1*1 + 1*1 = 12
//     f1 * f3 = 1*1 + 1*1 + 4*4 + 1*4 + 1*1 + 1*1 = 24
//     f1 * f4 = 1*4 + 1*4 + 4*4 + 1*7 + 1*4 + 1*4 = 39
//     f1 * f5 = 1*1 + 1*1 + 4*1 + 1*6 + 1*1 + 1*1 = 14
//
//     f2 * f3 = 1*1 + 1*1 + 1*4 + 4*4 + 1*1 + 1*1 = 24
//     f2 * f4 = 1*4 + 1*4 + 1*4 + 4*7 + 1*4 + 1*4 = 48
//     f2 * f5 = 1*1 + 1*1 + 1*1 + 4*6 + 1*1 + 1*1 = 29
//     f2 * f6 = 1*2 + 1*2 + 1*2 + 4*8 + 1*2 + 1*2 = 42
//
//     f3 * f4 = 1*4 + 1*4 + 4*4 + 4*7 + 1*4 + 1*4 = 60
//     f3 * f5 = 1*1 + 1*1 + 4*1 + 4*6 + 1*1 + 1*1 = 32
//     f3 * f6 = 1*2 + 1*2 + 4*2 + 4*8 + 1*2 + 1*2 = 48
//
//     f4 * f5 = 4*1 + 4*1 + 4*1 + 7*6 + 4*1 + 4*1 = 62
//     f4 * f6 = 4*2 + 4*2 + 4*2 + 7*8 + 4*2 + 4*2 = 96
//
//     f5 * f6 = 1*2 + 1*2 + 1*2 + 6*8 + 1*2 + 1*2 = 58
//

//  Algorithm:
//
//  1. Generate correlation functions for a synchro sequence and for a bit stream with values in range [+1; 2^16-1] to fit a float type.
//     Function generator must be reliable enough to generate functions with most unique multiplication output for any random input.
//
//  2. Multiply functions and divide the result by the maximum of absolute maximums of 2 functions.
//
//  3. Left as is or take the square root to convert value back to linear correlation value.
//
//     Examples:
//
//       Equality(f1, f1) = sqrt( 21 / 21 ) = 1 (maximum positive correlation value)
//       Equality(fa, fa) = Equality(fb, fb) = 1
//
//       Equality(f1, f2) = sqrt( 12 / max(21, 21) ) = sqrt(~0.57) = ~0.76
//       Equality(f1, f3) = sqrt( 24 / max(21, 36) ) = sqrt(~0.66) = ~0.82
//       Equality(f1, f4) = sqrt( 39 / max(21, 129) ) = sqrt(~0.30) = ~0.55
//       Equality(f1, f5) = sqrt( 14 / max(21, 41) ) = sqrt(~0.34) = ~0.58
//
//       Equality(f2, f3) = sqrt( 24 / max(21, 36) ) = sqrt(~0.66) = ~0.82
//       Equality(f2, f4) = sqrt( 48 / max(21, 129) ) = sqrt(~0.37) = ~0.61
//       Equality(f2, f5) = sqrt( 29 / max(21, 41) ) = sqrt(~0.71) = ~0.84
//       Equality(f2, f6) = sqrt( 42 / max(21, 84) ) = sqrt(0.5) = ~0.71
//
//       Equality(f3, f4) = sqrt( 60 / max(36, 129) ) = sqrt(~0.47) = ~0.68
//       Equality(f3, f5) = sqrt( 32 / max(36, 41) ) = sqrt(~0.78) = ~0.88
//       Equality(f3, f6) = sqrt( 48 / max(36, 84) ) = sqrt(~0.57) = ~0.76
//
//       Equality(f4, f5) = sqrt(62 / max(129, 41)) = sqrt(~0.48) = ~0.69
//       Equality(f4, f6) = sqrt(96 / max(129, 84)) = sqrt(~0.74) = ~0.86
//
//       Equality(f5, f6) = sqrt( 58 / max(41, 84) ) = sqrt(~0.69) = ~0.83
//
//  4. Use one of the implementation algorithms described for `/impl-token` option in the help file.
//     May needs too much memory or can be too slow, so do use the optimization command line parameters to reduce memory consumption and speedup the calculation
//     for the price of may be higher risk of uncertainty or false positive result.
//

template <typename T>
inline double end_calc_phase_time(std::tstring phase_name, const std::chrono::time_point<T> & begin_calc_time, std::vector<CalcTimePhase> & calc_time_phases)
{
    const auto end_calc_time = std::chrono::high_resolution_clock::now();

    const auto calc_time_dur = end_calc_time - begin_calc_time;

    const double calc_time_dur_sec = calc_time_dur.count() >= 0 ? // workaround for negative values
        std::chrono::duration<double>(calc_time_dur).count() : 0;

    calc_time_phases.push_back(CalcTimePhase{phase_name, calc_time_dur_sec, 0});

    return calc_time_dur_sec;
}

inline void calc_phase_time_fractions(double calc_all_time_sec, std::vector<CalcTimePhase> & calc_time_phases)
{
    for (auto & calc_time_phase : calc_time_phases) {
        calc_time_phase.calc_time_all_dur_fract = float(calc_time_phase.calc_time_dur_sec / calc_all_time_sec);
    }
}

extern inline float multiply_bits(Impl::corr_multiply_method mm, uint32_t in0_value, uint32_t in1_value, size_t size)
{
    static const uint32_t s_prime1033_numbers_arr[] = { // CAUTION: for 32-bit blocks
        1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129,
        1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259,
    };

    assert(size > 0 && 32 >= size);

    uint32_t multiplied_value = 0;

    switch (mm) {
    case Impl::corr_muliply_inverted_xor_prime1033:
    {
        // NOTE:
        //  The xor is treated as inverted (count zeros instead ones), because zero difference must gain a highest positive value.
        //  The maximum difference must gain a lowest positive value, but not a zero.
        //  As a result, is less the difference, then is higher the resulted value, so the multiplication of equal bit sequences will gain the highest value.
        //  Only after that we can convert the integer to the floating point number range (0; 1.0].
        //
        const uint32_t xor_value = (in0_value ^ in1_value);

        for (size_t i = 0; i < size; i++) {
            multiplied_value += (xor_value & (uint32_t(0x01) << i)) ? 0 : s_prime1033_numbers_arr[i];
        }
    } break;

    case Impl::corr_muliply_dispersed_value_prime1033:
    {
        // NOTE:
        //  The xor might be not enough because it gains not enough unique or low dispersion result.
        //  In that case we can increase multiplication dispersion to improve correlation certainty which
        //  is relied on more arranged or wider spectrum of a correlation value.
        //

        for (size_t i = 0; i < size; i++) {
            const uint32_t in0_dispersed_value = (in0_value & (uint32_t(0x01) << i)) ? s_prime1033_numbers_arr[i] : (i + 1) * 2;
            const uint32_t in1_dispersed_value = (in1_value & (uint32_t(0x01) << i)) ? s_prime1033_numbers_arr[i] : (i + 1) * 2;

            multiplied_value += in0_dispersed_value * in1_dispersed_value;
        }
    } break;

    default:
        assert(0);
    }

    return float(multiplied_value ? multiplied_value : 1); // return only minimal positive value
}

// returns float instead of double because 32-bit integer contains a value in range [1; 2^16-1]
//
extern inline float calculate_corr_value(Impl::corr_multiply_method mm, uint32_t in0_value, uint32_t in1_value, size_t size, float max_in0, float max_in1, bool make_linear_corr)
{
    const float corr = multiply_bits(mm, in0_value, in1_value, size) / (std::max)(max_in0, max_in1);

    // zero padded values from padded arrays must not be passed here
    assert(0 < corr && 1.0f >= corr); // must be always in range (0; 1]

    return make_linear_corr ? std::sqrt(corr) : corr;
}

void calculate_syncseq_correlation(
    const CorrInParams &                    corr_in_params,
    CorrInOutParams &                       corr_io_params,
    CorrOutParams &                         corr_out_params,
    uint8_t *                               stream_buf,
    std::vector<float> &                    corr_values_arr,
    std::vector<SyncseqCorr> &              corr_autocorr_arr,
    std::deque<SyncseqCorrMean> &           corr_max_weighted_mean_sum_deq,
    std::deque<SyncseqCorrMeanDeviat> &     corr_min_mean_deviat_sum_deq)
{
    // Old classic xor algorithm:
    //
    //  Search synchro sequence in a bit stream by sequential 32-bit blocks compare as shifted 64-bit blocks.
    //  Formula for all comparisons (K) of N 32-bit blocks in a stream:
    //    If N-even => K = ((N + 1) * N / 2 - 1) * 32
    //    If N-odd  => K = ((N + 1) / 2 * N - 1) * 32
    //
    // New algorithm does not use xor. Instead it does use 2 functions multiplication to calculate correlation.
    // The main difference is that the xor algorithm does not resistant to the input noise, when the correlation algorithm can be modified to gain that ability.
    //

    const auto begin_calc_time = std::chrono::high_resolution_clock::now();

    corr_out_params.calc_time_phases.reserve(4);

    BOOST_SCOPE_EXIT(&corr_out_params, begin_calc_time) {
        const auto calc_all_time_sec = end_calc_phase_time(_T("all"), begin_calc_time, corr_out_params.calc_time_phases);

        calc_phase_time_fractions(calc_all_time_sec, corr_out_params.calc_time_phases);
    } BOOST_SCOPE_EXIT_END;

    const auto stream_bit_size = corr_in_params.stream_bit_size;
    const auto syncseq_bit_size = corr_in_params.syncseq_bit_size;

    const auto syncseq_int32 = corr_io_params.syncseq_int32;
    const uint32_t syncseq_mask = uint32_t(~(~uint64_t(0) << syncseq_bit_size));
    const uint32_t syncseq_bytes = syncseq_int32 & syncseq_mask;

    // write back
    corr_io_params.syncseq_int32 = syncseq_bytes;

    assert(stream_bit_size);
    assert(syncseq_bit_size);
    assert(syncseq_bit_size < stream_bit_size); // must be greater

    // Recalculate the maximum repeats from the minimum repeats, because number of minimum repeats has a priority.
    //
    const auto syncseq_min_repeat = corr_in_params.period_min_repeat ? corr_in_params.period_min_repeat : 1;
    const auto syncseq_max_repeat = (std::max)(corr_in_params.period_max_repeat, syncseq_min_repeat);

    uint64_t stream_min_period =
        (std::min)(
            // If not defined, then minimal "from" period is at least 2 synchro sequence bit length, otherwise - not less than synchro sequence bit length + 1.
            // But always not greater than a bit stream length - 1.
            uint64_t(corr_in_params.min_period ? (std::max)(syncseq_bit_size + 1, corr_in_params.min_period) : syncseq_bit_size * 2),
            stream_bit_size - 1);
    uint64_t stream_max_period =
        (std::min)(
            // If not defined, then minimal "to" period is at least 2 synchro sequence bit length, otherwise - not less than synchro sequence bit length + 1.
            // But always not greater than a bit stream length - 1.
            uint64_t(corr_in_params.max_period != math::uint32_max ? (std::max)(syncseq_bit_size + 1, corr_in_params.max_period) : stream_bit_size * 2),
            stream_bit_size - 1);

    // align maximum period by minimum period
    stream_max_period = (std::max)(stream_max_period, stream_min_period);

    // Recalculate min/max periods from the minimum repeat, because number of minimum repeats has priority over a period.
    //
    const uint64_t stream_max_period_for_min_repeat = (stream_bit_size - 1) / (syncseq_min_repeat ? syncseq_min_repeat : 1);

    stream_min_period = (std::min)(stream_min_period, stream_max_period_for_min_repeat);
    stream_max_period = (std::min)(stream_max_period, stream_max_period_for_min_repeat);

    // just in case
    stream_min_period = (std::min)(stream_min_period, uint64_t(math::uint32_max));
    stream_max_period = (std::min)(stream_max_period, uint64_t(math::uint32_max));

    assert(stream_min_period);

    // CAUTION:
    //  Input conditions must be initially in consistency between each other and must not decrease the field of search of the result because of mutual inconsistency.
    //

    // write back for user notification
    corr_out_params.min_period = uint32_t(stream_min_period);
    corr_out_params.max_period = uint32_t(stream_max_period);

    if (corr_in_params.min_period && corr_in_params.min_period != math::uint32_max && corr_in_params.min_period < stream_min_period) {
        // recalculated minimum period is increased, important input condition is changed and reduced the field of search of the result, quit calculation
        corr_out_params.input_inconsistency = true;
        return;
    }

    if (corr_in_params.max_period != math::uint32_max && stream_max_period < corr_in_params.max_period) {
        // recalculated maximum period is decreased, important input condition is changed and reduced the field of search of the result, quit calculation
        corr_out_params.input_inconsistency = true;
        return;
    }

    if (corr_in_params.period_min_repeat && stream_max_period * corr_in_params.period_min_repeat >= stream_bit_size) {
        // recalculated stream bit size is increased, important input condition is changed and reduced the field of search of the result, quit calculation
        corr_out_params.input_inconsistency = true;
        return;
    }

    assert(stream_max_period * syncseq_min_repeat < stream_bit_size);

    if (syncseq_bit_size >= stream_min_period) {
        // the input conditions are not met, not enough room for calculation
        corr_out_params.input_inconsistency = true;
        return;
    }

    // Buffer is already padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block.
    //
    const uint64_t padded_stream_bit_size = (stream_bit_size + 31) & ~uint32_t(31);
    const uint64_t num_stream_32bit_blocks = padded_stream_bit_size / 32;
    uint32_t * stream_buf32 = (uint32_t *)stream_buf;

    float syncseq_corr_absmax;
    std::vector<float> stream_corr_absmax_arr(size_t(stream_bit_size), 1);

    // Phase 1:
    //
    //  Correlation values calculation, creates a moderate but still instable algorithm certainty or false positive stability within input noise.
    //
    // Produces correlation values with noticeable certainty tolerance around 33% from the math expected value of number `one` bits per synchro sequence length,
    // where math expected value is equal to the half of synchro sequence length.
    // For example, if synchro sequence has 20 bits length, then algorithm would output uncertain correlation values after a value greater than 33% of noised
    // bits of 10 bits, i.e. greater than ~3 noised bits per each 20 bits in a bit stream.
    //
    //  Note:
    //    Even if the algorithm has a complete instability for not false positive output if over a tolerance, but nonetheless that does not mean it is
    //    stable enough below the tolerance. To increase the noise tolerance and gain more stability below the tolerance you must use the second phase algorithm.
    //
    // Memory complexity:   O(N * M) - to generate complement functions, O(N) - to calculate correlation values
    // Time complexity:     O(N * M)
    //                        , where N - stream bit length, M - synchro sequence bit length
    //

    {
        const auto begin_calc_phase_time = std::chrono::high_resolution_clock::now();

        // Calculate correlation complement functions absolute maximums (by multiply to itself).
        //
        // Major time complexity: O(N * M), where N - stream bit length, M - synchro sequence bit length
        //

        // calculate absolute maximums for the synchro sequence bits
        syncseq_corr_absmax = multiply_bits(corr_in_params.corr_mm, syncseq_bytes, syncseq_bytes, syncseq_bit_size);

        // Calculate absolute maximums for the bit stream.
        //
        //
        // Major time complexity: O(N * M), where N - stream bit length, M - synchro sequence bit length
        //

        if_goto_b(phase11_break, true) {
            size_t stream_bit_offset = 0;

            for (uint32_t from = 0; from < num_stream_32bit_blocks; from++) {
                const uint64_t from64 = *(uint64_t *)(stream_buf32 + from);

                for (uint32_t i = 0; i < 32; i++, stream_bit_offset++) {
                    const uint32_t from_shifted = uint32_t(from64 >> i) & syncseq_mask;

                    stream_corr_absmax_arr[stream_bit_offset] = multiply_bits(corr_in_params.corr_mm, from_shifted, from_shifted, syncseq_bit_size);

                    // avoid calculation from zero padded array values
                    if (stream_bit_offset >= stream_bit_size) {
                        goto phase11_break;
                    }
                }
            }
        }

        float min_corr_value = math::float_max;
        float max_corr_value = 0;

        uint32_t num_corr_values_calc = 0;

        // Calculate correlation values.
        //
        // Major time complexity: O(N * M), where N - stream bit length, M - synchro sequence bit length
        //

        corr_values_arr.reserve((size_t(stream_bit_size)));

        // CAUTION:
        //  We must avoid drop to zero before an autocorrelation calculation, because it will randomly distort the being multiplied functions length.
        //

        if_goto_b(phase12_break, true) {
            size_t stream_bit_offset = 0;

            for (uint32_t from = 0; from < num_stream_32bit_blocks; from++) {
                const uint64_t from64 = *(uint64_t *)(stream_buf32 + from);

                for (uint32_t i = 0; i < 32; i++, stream_bit_offset++) {
                    const uint32_t from_shifted = uint32_t(from64 >> i) & syncseq_mask;

                    const auto corr_value = calculate_corr_value(
                        corr_in_params.corr_mm,
                        syncseq_bytes, from_shifted, syncseq_bit_size, syncseq_corr_absmax, stream_corr_absmax_arr[stream_bit_offset],
                        corr_in_params.use_linear_corr);

                    min_corr_value = (std::min)(min_corr_value, corr_value);
                    max_corr_value = (std::max)(max_corr_value, corr_value);

                    if (corr_value >= corr_in_params.corr_min) {
                        corr_values_arr.push_back(corr_value);

                        num_corr_values_calc++;
                    }
                    else {
                        corr_values_arr.push_back(0);
                    }

                    // avoid calculation from zero padded array values
                    if (stream_bit_offset >= stream_bit_size) {
                        goto phase12_break;
                    }
                }
            }
        }

        end_calc_phase_time(_T("corr values"), begin_calc_phase_time, corr_out_params.calc_time_phases);

        corr_out_params.num_corr_values_calc = num_corr_values_calc;

        corr_out_params.min_corr_value = min_corr_value;
        corr_out_params.max_corr_value = max_corr_value;
    }

    switch (corr_in_params.impl_token) {
    case Impl::impl_max_weighted_sum_of_corr_mean:
    case Impl::impl_min_sum_of_corr_mean_deviat:
    {
        // Phase 2:
        //
        //  Correlation mean (average) values calculation, significantly increases algorithm certainty or false positive stability within input noise.
        //
        // Produces correlation mean values with noticeable certainty tolerance around 66% from the math expected value of number `one` bits per synchro sequence length,
        // where math expected value is equal to the half of synchro sequence length.
        // For example, if synchro sequence has 20 bits length, then algorithm would output uncertain correlation mean values after a value greater than 66% of noised
        // bits of 10 bits, i.e. greater than ~6 noised bits per each 20 bits in a bit stream.
        //
        //  Note:
        //    Even if the algorithm has a complete instability for not false positive output if over a tolerance, but nonetheless that does not mean it has
        //    100% stability below the tolerance. In other words the algorithm just has no noted instability for not false positive output below the tolerance.
        //    But still sometimes it happens to be instable below but near the tolerance.
        //
        // Float memory complexity:     from O(N) to O(N * N * ln(N)),
        //                                depends on options, in approximation ~ O(N) for `/corr-mean-min 0.81`, `/max-corr-values-per-period <N>` and other options is default.
        //                                If `/max-corr-values-per-period <K>`, then memory complexity reduces down to `O(N * K * ln(N))`.
        // Major Time complexity:       O(N * N * ln(N))
        //                                , where N - stream bit length, K < N
        //

        corr_out_params.accum_corr_mean_calc = true;

        // CAUTION:
        //  The correlation values array (`corr_values_arr`) can has 0's values because values can be filtered out by the correlation minimum value.
        //  What means we must skip usage of these values.
        //

        if (corr_in_params.impl_token == Impl::impl_max_weighted_sum_of_corr_mean) {
            const auto begin_calc_phase_means_time = std::chrono::high_resolution_clock::now();

            //// calculate maximum storage size for correlation mean values to cancel calculations, rounding to greater
            //const uint64_t corr_max_weighted_mean_sum_deq_max_size = (corr_in_params.max_corr_mean_bytes + sizeof(corr_max_weighted_mean_sum_deq[0]) - 1) / sizeof(corr_max_weighted_mean_sum_deq[0]);
            //
            //// x 10 to reserve space for at least 10 first results greater or equal than minimal correlation mean value
            //const uint64_t corr_reserve_mean_arr_max_size = (std::min)(
            //    corr_max_weighted_mean_sum_deq_max_size,
            //    (stream_bit_size - 1) * (corr_in_params.corr_mean_min ? 10 : 1));
            //
            //corr_max_weighted_mean_sum_deq.reserve(size_t(corr_reserve_mean_arr_max_size));

            struct CorrOffsetMean
            {
                float corr_mean;
                uint32_t num_corr;
                uint64_t offset;
            };

            uint32_t num_corr_values_iter = 0;

            uint32_t num_corr_means_calc = 0;
            uint32_t num_corr_means_iter = 0;

            size_t used_corr_mean_bytes = 0;
            size_t accum_corr_mean_bytes = 0;

            float min_corr_mean_value = math::float_max;
            float max_corr_mean_value = 0;

            CorrOffsetMean corr_offset_mean;

            std::vector<CorrOffsetMean> corr_max_means_per_period; // for single period and different offsets

            corr_max_means_per_period.reserve(corr_in_params.max_corr_values_per_period + 1); // plus one to resize back to maximum if greater

            for (uint64_t period = stream_max_period; period >= stream_min_period; period--) {
                corr_max_means_per_period.clear();

                for (uint64_t i = 0, j = i + period, repeat = 0; i < stream_bit_size - 1; i++, j = i + period, repeat = 0) {
                    // skip all offsets if number of periods in an offset is greater than maximum
                    //
                    if (corr_in_params.max_periods_in_offset != math::uint32_max) {
                        if (corr_in_params.max_periods_in_offset) {
                            // including first bit of next period
                            if (i / period >= corr_in_params.max_periods_in_offset && (i % period)) {
                                break;
                            }
                        }
                        else {
                            // excluding first bit of 2d period
                            if (i / period >= 1) {
                                break;
                            }
                        }
                    }

                    // skip all offsets if the number of left periods if less than minimal value
                    //
                    if (syncseq_min_repeat >= (stream_bit_size - i + period - 1) / period) {
                        break;
                    }

                    const float first_corr_value = corr_values_arr[size_t(i)];

                    if (corr_in_params.skip_calc_on_filtered_corr_value_use && !first_corr_value) {
                        goto skip_calc_on_filtered_corr_value_use_1;
                    }

                    corr_offset_mean = CorrOffsetMean{ first_corr_value, first_corr_value ? 1U : 0U, i };

                    num_corr_values_iter++;

                    for (; j < stream_bit_size && repeat < syncseq_max_repeat; j += period, repeat++) {
                        const float next_corr_value = corr_values_arr[size_t(j)];

                        if (corr_in_params.skip_calc_on_filtered_corr_value_use && !next_corr_value) {
                            goto skip_calc_on_filtered_corr_value_use_1;
                        }

                        if (next_corr_value) {
                            corr_offset_mean.corr_mean += next_corr_value;
                            corr_offset_mean.num_corr++;
                        }

                        num_corr_values_iter++;
                    }

                    assert(repeat + 1 >= corr_offset_mean.num_corr);

                    // must be at least 1 repeat
                    if (corr_offset_mean.num_corr >= 1 + syncseq_min_repeat) {
                        corr_offset_mean.corr_mean /= corr_offset_mean.num_corr;

                        if (corr_offset_mean.corr_mean >= corr_in_params.corr_mean_min) {
                            utils::insert_sorted(corr_max_means_per_period, corr_offset_mean, [](const CorrOffsetMean & l, const CorrOffsetMean & r) -> bool {
                                return l.corr_mean > r.corr_mean;
                            });

                            if (corr_max_means_per_period.size() > corr_in_params.max_corr_values_per_period) {
                                corr_max_means_per_period.resize(corr_in_params.max_corr_values_per_period);
                            }

                            num_corr_means_calc++;
                        }
                    }

                    num_corr_means_iter++;

                    if (corr_offset_mean.corr_mean && corr_offset_mean.num_corr) {
                        min_corr_mean_value = (std::min)(min_corr_mean_value, corr_offset_mean.corr_mean);
                        max_corr_mean_value = (std::max)(max_corr_mean_value, corr_offset_mean.corr_mean);
                    }

                skip_calc_on_filtered_corr_value_use_1:;
                }

                for (const auto & corr_max_mean : corr_max_means_per_period) {
                    corr_max_weighted_mean_sum_deq.push_back(SyncseqCorrMean{
                        uint32_t(corr_max_mean.offset), uint32_t(period), corr_max_mean.num_corr, corr_max_mean.corr_mean, 0
                        });
                }

                used_corr_mean_bytes = (std::max)(used_corr_mean_bytes, corr_max_weighted_mean_sum_deq.size() * sizeof(corr_max_weighted_mean_sum_deq[0]));
                accum_corr_mean_bytes = (std::max)(accum_corr_mean_bytes, corr_max_weighted_mean_sum_deq.size() * sizeof(corr_max_weighted_mean_sum_deq[0]));

                if (accum_corr_mean_bytes >= corr_in_params.max_corr_mean_bytes) {
                    // out of buffer, cancel calculation
                    corr_out_params.accum_corr_mean_quit = true;
                    break;
                }
            }

            end_calc_phase_time(_T("corr mean values"), begin_calc_phase_means_time, corr_out_params.calc_time_phases);

            corr_out_params.min_corr_mean = min_corr_mean_value;
            corr_out_params.max_corr_mean = max_corr_mean_value;

            corr_out_params.num_corr_values_iterated = num_corr_values_iter;

            corr_out_params.num_corr_means_calc = num_corr_means_calc;
            corr_out_params.num_corr_means_iterated = num_corr_means_iter;

            corr_out_params.used_corr_mean_bytes = used_corr_mean_bytes;
            corr_out_params.accum_corr_mean_bytes = accum_corr_mean_bytes;

            const auto corr_max_weighted_mean_sum_deq_size = corr_max_weighted_mean_sum_deq.size();

            if (!corr_max_weighted_mean_sum_deq_size) {
                break;
            }

            if (!corr_in_params.skip_max_weighted_sum_of_corr_mean_calc) {
                // Phase 3:
                //
                //  Correlation maximum mean (average) values weighted sum calculation, makes groups of offsets grouped with a multiple by a period,
                //  where groups sorted by offset and then by period in a group. Or in another words does calculate weighted sums of maximal mean values
                //  in groups calculated for the same offset with a multiple by a period.
                //
                //  The output is sorted by maximum mean sum from maximum to minimum, then by offset from minimum to maximum and then by period for an
                //  offset from minimum to maximum.
                //
                //  NOTE:
                //    For speedup reasons the sort by offset does for the first mean sum, by period - for the first offset.
                //
                //  Helps to resort found offsets and periods to locate more certain values which brings even more stability for false positives.
                //
                //  The idea is to put the set of solutions with tolerance-equal mean sums which can be in different order:
                //
                //    offset  | period
                //  ----------+--------
                //  x         | k
                //  x         | k * 2
                //  x         | k * 3
                //  x         | k * m
                //  x + k     | k * m
                //  x + k * 2 | k * m
                //  x + k * 3 | k * m
                //  x + k * n | k * m
                //
                //  Into sorted order, where x, k, n, m - is a possible minimum.
                //
                //  Memory complexity:   O(N) - input, O(N) - output
                //  Time complexity:     O(N  * M)
                //                         , where N - stream bit length, M - number of used periods per offset
                //
                //  Example:
                //
                //   index |   max mean   | num corr |  offset  |  period
                //  -------+--------------+----------+----------+---------
                //     1   |    0.850     |    30    |    20    |    33
                //     2   |    0.845     |     5    |    10    |   200
                //     3   |    0.841     |    11    |    30    |    90
                //     4   |    0.836     |    11    |    10    |   100
                //     5   |    0.831     |     6    |   100    |   165
                //     6   |    0.823     |    17    |   100    |    55
                //
                //  After sort the table above, the result will be:
                //
                //         |   max mean   |          |
                //   index | weighted sum |  offset  |  period
                //  -------+--------------+----------+---------
                //     4   |    1.1174    |    10    |   100
                //     2   |    1.1174    |    10    |   200
                //     6   |    1.0082    |   100    |    55
                //     5   |    1.0082    |   100    |   165
                //     1   |    0.850     |    20    |    33
                //     3   |    0.841     |    30    |    90
                //
                //   , where:
                //
                //     1.1174 = 0.836 + 0.845 * (5 - 1) / (11 - 1)
                //     1.0082 = 0.823 + 0.831 * (6 - 1) / (17 - 1)
                //

                const auto begin_calc_weighted_means_sum_time = std::chrono::high_resolution_clock::now();

                auto begin_it = corr_max_weighted_mean_sum_deq.begin();
                auto end_it = corr_max_weighted_mean_sum_deq.end();

                // sort by offset

                std::sort(begin_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                {
                    return l.offset < r.offset;
                });

                // sort by period in an offset

                size_t step;

                auto first_it = begin_it;

                auto prev_it = begin_it;
                auto next_it = prev_it;

                for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                    auto & corr_max_mean_prev_ref = *prev_it;
                    auto & corr_max_mean_next_ref = *next_it;

                    if (corr_max_mean_prev_ref.offset != corr_max_mean_next_ref.offset) {
                        if (step > 1) {
                            std::sort(first_it, next_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                            {
                                return l.period > r.period;
                            });
                        }

                        first_it = next_it;
                        step = 0;
                    }
                }

                // calculate correlation mean weights and local mean sums, O(N * M) time complexity

                first_it = begin_it;

                prev_it = begin_it;
                next_it = prev_it;

                for (next_it++; next_it != end_it; prev_it = next_it, next_it++) {
                    auto & corr_max_mean_prev_ref = *prev_it;
                    auto & corr_max_mean_next_ref = *next_it;

                    if (corr_max_mean_prev_ref.offset == corr_max_mean_next_ref.offset) {
                        corr_max_mean_prev_ref.corr_mean_sum = corr_max_mean_prev_ref.corr_mean * (corr_max_mean_prev_ref.num_corr - 1);
                    }
                    else {
                        corr_max_mean_prev_ref.corr_mean_sum = corr_max_mean_prev_ref.corr_mean;

                        // accumulate mean sum for a minimal period
                        for (auto it = first_it; it != prev_it; it++) {
                            if (!(it->period % corr_max_mean_prev_ref.period)) {
                                corr_max_mean_prev_ref.corr_mean_sum += it->corr_mean_sum / (corr_max_mean_prev_ref.num_corr - 1);
                            }
                        }

                        first_it = next_it;
                    }
                }

                auto & corr_max_mean_last_ref = corr_max_weighted_mean_sum_deq.back();

                corr_max_mean_last_ref.corr_mean_sum = corr_max_mean_last_ref.corr_mean;

                // accumulate mean sum for a minimal period
                for (auto it = first_it; it != prev_it; it++) {
                    if (!(it->period % prev_it->period)) {
                        prev_it->corr_mean_sum += it->corr_mean_sum / (prev_it->num_corr - 1);
                    }
                }

                // calculate correlation mean sum, O(N) time complexity

                auto begin_rit = corr_max_weighted_mean_sum_deq.rbegin();
                auto end_rit = corr_max_weighted_mean_sum_deq.rend();

                auto first_rit = begin_rit;

                auto prev_rit = begin_rit;
                auto next_rit = prev_rit;

                for (next_rit++; next_rit != end_rit; prev_rit = next_rit, next_rit++) {
                    auto & corr_max_mean_prev_ref = *prev_rit;
                    auto & corr_max_mean_next_ref = *next_rit;

                    if (corr_max_mean_prev_ref.offset == corr_max_mean_next_ref.offset) {
                        if (!(corr_max_mean_next_ref.period % first_rit->period)) {
                            corr_max_mean_next_ref.corr_mean_sum = first_rit->corr_mean_sum;
                        }
                        else { // not a multiple by a minimal period, reset to mean value
                            corr_max_mean_next_ref.corr_mean_sum = corr_max_mean_next_ref.corr_mean;
                        }
                    }
                    else {
                        first_rit = next_rit;
                    }
                }

                if (corr_in_params.return_sorted_result) {
                    // sort by correlation mean sum

                    std::sort(begin_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                    {
                        return l.corr_mean_sum > r.corr_mean_sum;
                    });

                    // sort by offset in a correlation mean sum

                    first_it = begin_it;

                    prev_it = begin_it;
                    next_it = prev_it;

                    for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                        auto & corr_max_mean_prev_ref = *prev_it;
                        auto & corr_max_mean_next_ref = *next_it;

                        if (corr_max_mean_prev_ref.corr_mean_sum != corr_max_mean_next_ref.corr_mean_sum) {
                            if (step > 1) {
                                std::sort(first_it, next_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                                {
                                    return l.offset < r.offset;
                                });
                            }

                            first_it = next_it;
                            step = 0;
                        }
                    }

                    // sort the last
                    if (step > 1) {
                        std::sort(first_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                        {
                            return l.offset < r.offset;
                        });
                    }

                    // sort by period in an offset

                    first_it = begin_it;

                    prev_it = begin_it;
                    next_it = prev_it;

                    for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                        auto & corr_max_mean_prev_ref = *prev_it;
                        auto & corr_max_mean_next_ref = *next_it;

                        if (corr_max_mean_prev_ref.corr_mean_sum != corr_max_mean_next_ref.corr_mean_sum || corr_max_mean_prev_ref.offset != corr_max_mean_next_ref.offset) {
                            if (step > 1) {
                                std::sort(first_it, next_it, [&](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                                {
                                    return l.period < r.period;
                                });
                            }

                            first_it = next_it;
                            step = 0;
                        }
                    }

                    // sort the last
                    if (step > 1) {
                        std::sort(first_it, end_it, [&](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                        {
                            return l.period < r.period;
                        });
                    }
                }
                else {
                    // just search and return a single value instead of sort

                    auto begin_it = corr_max_weighted_mean_sum_deq.begin();
                    auto end_it = corr_max_weighted_mean_sum_deq.end();

                    // CAUTION:
                    //  The predicate second argument does remember on return `true`.
                    //
                    const SyncseqCorrMean max_corr_mean = *std::max_element(begin_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                    {
                        // take with lowest period and offset if previous values are equal
                        return l.corr_mean_sum < r.corr_mean_sum || l.corr_mean_sum == r.corr_mean_sum && (l.offset > r.offset || l.offset == r.offset && l.period > r.period);
                    });

                    corr_max_weighted_mean_sum_deq.resize(1);

                    corr_max_weighted_mean_sum_deq[0] = max_corr_mean;
                }

                end_calc_phase_time(_T("corr weighted means sum"), begin_calc_weighted_means_sum_time, corr_out_params.calc_time_phases);
            }
            else {
                const auto begin_calc_max_mean_time = std::chrono::high_resolution_clock::now();

                auto begin_it = corr_max_weighted_mean_sum_deq.begin();
                auto end_it = corr_max_weighted_mean_sum_deq.end();

                if (corr_in_params.return_sorted_result) {
                    // sort by correlation mean

                    std::sort(begin_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                    {
                        return l.corr_mean > r.corr_mean;
                    });

                    // sort by offset in a correlation mean

                    size_t step;

                    auto first_it = begin_it;

                    auto prev_it = begin_it;
                    auto next_it = prev_it;

                    for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                        auto & corr_max_mean_prev_ref = *prev_it;
                        auto & corr_max_mean_next_ref = *next_it;

                        if (corr_max_mean_prev_ref.corr_mean != corr_max_mean_next_ref.corr_mean) {
                            if (step > 1) {
                                std::sort(first_it, next_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                                {
                                    return l.offset < r.offset;
                                });
                            }

                            first_it = next_it;
                            step = 0;
                        }
                    }

                    // sort the last
                    if (step > 1) {
                        std::sort(first_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                        {
                            return l.offset < r.offset;
                        });
                    }

                    // sort by period in an offset

                    first_it = begin_it;

                    prev_it = begin_it;
                    next_it = prev_it;

                    for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                        auto & corr_max_mean_prev_ref = *prev_it;
                        auto & corr_max_mean_next_ref = *next_it;

                        if (corr_max_mean_prev_ref.corr_mean != corr_max_mean_next_ref.corr_mean || corr_max_mean_prev_ref.offset != corr_max_mean_next_ref.offset) {
                            if (step > 1) {
                                std::sort(first_it, next_it, [&](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                                {
                                    return l.period < r.period;
                                });
                            }

                            first_it = next_it;
                            step = 0;
                        }
                    }

                    // sort the last
                    if (step > 1) {
                        std::sort(first_it, end_it, [&](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                        {
                            return l.period < r.period;
                        });
                    }
                }
                else {
                    // just search and return a single value instead of sort

                    // CAUTION:
                    //  The predicate second argument does remember on return `true`.
                    //
                    const SyncseqCorrMean max_corr_mean_sum = *std::max_element(begin_it, end_it, [](const SyncseqCorrMean & l, const SyncseqCorrMean & r) -> bool
                    {
                        // take with lowest period and offset if previous values are equal
                        return l.corr_mean < r.corr_mean || l.corr_mean == r.corr_mean && (l.offset > r.offset || l.offset == r.offset && l.period > r.period);
                    });

                    corr_max_weighted_mean_sum_deq.resize(1);

                    corr_max_weighted_mean_sum_deq[0] = max_corr_mean_sum;
                }

                end_calc_phase_time(_T("corr max mean"), begin_calc_max_mean_time, corr_out_params.calc_time_phases);
            }
        }
        else if (corr_in_params.impl_token == Impl::impl_min_sum_of_corr_mean_deviat) {
            const auto begin_calc_phase_mean_deviats_time = std::chrono::high_resolution_clock::now();

            struct CorrOffsetMeanDeviat
            {
                float corr_mean;
                float corr_mean_deviat_sum;
                uint32_t num_corr;
                uint64_t offset;
            };

            uint32_t num_corr_values_iter = 0;

            uint32_t num_corr_means_calc = 0;
            uint32_t num_corr_means_iter = 0;

            size_t used_corr_mean_bytes = 0;
            size_t accum_corr_mean_bytes = 0;

            float min_corr_mean_value = math::float_max;
            float max_corr_mean_value = 0;

            float min_corr_mean_deviat_value = math::float_max;
            float max_corr_mean_deviat_value = 0;

            CorrOffsetMeanDeviat corr_offset_mean_deviat;

            std::vector<CorrOffsetMeanDeviat> corr_min_mean_deviat_sum_per_period; // for single period and different offsets

            corr_min_mean_deviat_sum_per_period.reserve(corr_in_params.max_corr_values_per_period + 1); // plus one to resize back to maximum if greater

            for (uint64_t period = stream_max_period; period >= stream_min_period; period--) {
                corr_min_mean_deviat_sum_per_period.clear();

                for (uint64_t i = 0, j = i + period, repeat = 0; i < stream_bit_size - 1; i++, j = i + period, repeat = 0) {
                    // skip all offsets if number of periods in an offset is greater than maximum
                    //
                    if (corr_in_params.max_periods_in_offset != math::uint32_max) {
                        if (corr_in_params.max_periods_in_offset) {
                            // including first bit of next period
                            if (i / period >= corr_in_params.max_periods_in_offset && (i % period)) {
                                break;
                            }
                        }
                        else {
                            // excluding first bit of 2d period
                            if (i / period >= 1) {
                                break;
                            }
                        }
                    }

                    // skip all offsets if the number of left periods if less than minimal value
                    //
                    if (syncseq_min_repeat >= (stream_bit_size - i + period - 1) / period) {
                        break;
                    }

                    const float first_corr_value = corr_values_arr[size_t(i)];

                    if (corr_in_params.skip_calc_on_filtered_corr_value_use && !first_corr_value) {
                        goto skip_calc_on_filtered_corr_value_use_2;
                    }

                    corr_offset_mean_deviat = CorrOffsetMeanDeviat{ first_corr_value, 0, first_corr_value ? 1U : 0U, i };

                    num_corr_values_iter++;

                    for ( ; j < stream_bit_size && repeat < syncseq_max_repeat; j += period, repeat++) {
                        const float next_corr_value = corr_values_arr[size_t(j)];

                        if (corr_in_params.skip_calc_on_filtered_corr_value_use && !next_corr_value) {
                            goto skip_calc_on_filtered_corr_value_use_2;
                        }

                        if (next_corr_value) {
                            corr_offset_mean_deviat.corr_mean += next_corr_value;
                            corr_offset_mean_deviat.num_corr++;
                        }

                        num_corr_values_iter++;
                    }

                    assert(repeat + 1 >= corr_offset_mean_deviat.num_corr);

                    // must be at least 1 repeat
                    if (corr_offset_mean_deviat.num_corr >= 1 + syncseq_min_repeat) {
                        corr_offset_mean_deviat.corr_mean /= corr_offset_mean_deviat.num_corr;

                        if (corr_offset_mean_deviat.corr_mean >= corr_in_params.corr_mean_min) {
                            // calculate deviation sum

                            if (first_corr_value) {
                                const float first_corr_deviat_value = std::fabs(corr_offset_mean_deviat.corr_mean - first_corr_value);

                                corr_offset_mean_deviat.corr_mean_deviat_sum += first_corr_deviat_value;

                                min_corr_mean_deviat_value = (std::min)(min_corr_mean_deviat_value, first_corr_deviat_value);
                                max_corr_mean_deviat_value = (std::max)(max_corr_mean_deviat_value, first_corr_deviat_value);
                            }

                            for (j = i + period, repeat = 0; j < stream_bit_size && repeat < syncseq_max_repeat; j += period, repeat++) {
                                const float next_corr_value = corr_values_arr[size_t(j)];

                                if (next_corr_value) {
                                    const float next_corr_deviat_value = std::fabs(corr_offset_mean_deviat.corr_mean - next_corr_value);

                                    corr_offset_mean_deviat.corr_mean_deviat_sum += next_corr_deviat_value;

                                    min_corr_mean_deviat_value = (std::min)(min_corr_mean_deviat_value, next_corr_deviat_value);
                                    max_corr_mean_deviat_value = (std::max)(max_corr_mean_deviat_value, next_corr_deviat_value);
                                }
                            }

                            // mean sum of mean deviation
                            corr_offset_mean_deviat.corr_mean_deviat_sum /= corr_offset_mean_deviat.num_corr;

                            if (!corr_in_params.sort_at_first_by_max_corr_mean) {
                                utils::insert_sorted(corr_min_mean_deviat_sum_per_period, corr_offset_mean_deviat, [](const CorrOffsetMeanDeviat & l, const CorrOffsetMeanDeviat & r) -> bool {
                                    return l.corr_mean_deviat_sum < r.corr_mean_deviat_sum;
                                });
                            }
                            else {
                                utils::insert_sorted(corr_min_mean_deviat_sum_per_period, corr_offset_mean_deviat, [](const CorrOffsetMeanDeviat & l, const CorrOffsetMeanDeviat & r) -> bool {
                                    return l.corr_mean > r.corr_mean;
                                });
                            }

                            if (corr_min_mean_deviat_sum_per_period.size() > corr_in_params.max_corr_values_per_period) {
                                // CAUTION:
                                //  May lose elements with minimum correlation mean deviation sum if `/sort-at-first-by-max-corr-mean` is defined.
                                //
                                corr_min_mean_deviat_sum_per_period.resize(corr_in_params.max_corr_values_per_period);
                            }

                            num_corr_means_calc++;
                        }
                    }

                    num_corr_means_iter++;

                    if (corr_offset_mean_deviat.corr_mean && corr_offset_mean_deviat.num_corr) {
                        min_corr_mean_value = (std::min)(min_corr_mean_value, corr_offset_mean_deviat.corr_mean);
                        max_corr_mean_value = (std::max)(max_corr_mean_value, corr_offset_mean_deviat.corr_mean);
                    }

                skip_calc_on_filtered_corr_value_use_2:;
                }

                for (const auto & corr_min_mean_deviat : corr_min_mean_deviat_sum_per_period) {
                    corr_min_mean_deviat_sum_deq.push_back(SyncseqCorrMeanDeviat{
                        uint32_t(corr_min_mean_deviat.offset), uint32_t(period), corr_min_mean_deviat.num_corr, corr_min_mean_deviat.corr_mean, corr_min_mean_deviat.corr_mean_deviat_sum
                    });
                }

                used_corr_mean_bytes = (std::max)(used_corr_mean_bytes, corr_min_mean_deviat_sum_deq.size() * sizeof(corr_min_mean_deviat_sum_deq[0]));
                accum_corr_mean_bytes = (std::max)(accum_corr_mean_bytes, corr_min_mean_deviat_sum_deq.size() * sizeof(corr_min_mean_deviat_sum_deq[0]));

                if (accum_corr_mean_bytes >= corr_in_params.max_corr_mean_bytes) {
                    // out of buffer, cancel calculation
                    corr_out_params.accum_corr_mean_quit = true;
                    break;
                }
            }

            end_calc_phase_time(_T("corr mean deviat values"), begin_calc_phase_mean_deviats_time, corr_out_params.calc_time_phases);

            corr_out_params.min_corr_mean = min_corr_mean_value;
            corr_out_params.max_corr_mean = max_corr_mean_value;

            corr_out_params.min_corr_mean_deviat = min_corr_mean_deviat_value;
            corr_out_params.max_corr_mean_deviat = max_corr_mean_deviat_value;

            corr_out_params.num_corr_values_iterated = num_corr_values_iter;

            corr_out_params.num_corr_means_calc = num_corr_means_calc;
            corr_out_params.num_corr_means_iterated = num_corr_means_iter;

            corr_out_params.used_corr_mean_bytes = used_corr_mean_bytes;
            corr_out_params.accum_corr_mean_bytes = accum_corr_mean_bytes;

            const auto corr_min_mean_deviat_sum_deq_size = corr_min_mean_deviat_sum_deq.size();

            if (!corr_min_mean_deviat_sum_deq_size) {
                break;
            }

            const auto begin_calc_min_mean_deviat_time = std::chrono::high_resolution_clock::now();

            auto begin_it = corr_min_mean_deviat_sum_deq.begin();
            auto end_it = corr_min_mean_deviat_sum_deq.end();

            if (corr_in_params.return_sorted_result) {
                // sort by correlation mean deviation sum

                std::sort(begin_it, end_it, [](const SyncseqCorrMeanDeviat & l, const SyncseqCorrMeanDeviat & r) -> bool
                {
                    return l.corr_mean_deviat_sum < r.corr_mean_deviat_sum;
                });

                // sort by offset in a correlation mean deviation sum

                size_t step = 0;

                auto first_it = begin_it;

                auto prev_it = begin_it;
                auto next_it = prev_it;

                for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                    auto & corr_max_mean_prev_ref = *prev_it;
                    auto & corr_max_mean_next_ref = *next_it;

                    if (corr_max_mean_prev_ref.corr_mean_deviat_sum != corr_max_mean_next_ref.corr_mean_deviat_sum) {
                        if (step > 1) {
                            std::sort(first_it, next_it, [](const SyncseqCorrMeanDeviat & l, const SyncseqCorrMeanDeviat & r) -> bool
                            {
                                return l.offset < r.offset;
                            });
                        }

                        first_it = next_it;
                        step = 0;
                    }
                }

                // sort the last
                if (step > 1) {
                    std::sort(first_it, end_it, [](const SyncseqCorrMeanDeviat & l, const SyncseqCorrMeanDeviat & r) -> bool
                    {
                        return l.offset < r.offset;
                    });
                }

                // sort by period in an offset

                first_it = begin_it;

                prev_it = begin_it;
                next_it = prev_it;

                for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                    auto & corr_max_mean_prev_ref = *prev_it;
                    auto & corr_max_mean_next_ref = *next_it;

                    if (corr_max_mean_prev_ref.corr_mean_deviat_sum != corr_max_mean_next_ref.corr_mean_deviat_sum || corr_max_mean_prev_ref.offset != corr_max_mean_next_ref.offset) {
                        if (step > 1) {
                            std::sort(first_it, next_it, [&](const SyncseqCorrMeanDeviat & l, const SyncseqCorrMeanDeviat & r) -> bool
                            {
                                return l.period < r.period;
                            });
                        }

                        first_it = next_it;
                        step = 0;
                    }
                }

                // sort the last
                if (step > 1) {
                    std::sort(first_it, end_it, [&](const SyncseqCorrMeanDeviat & l, const SyncseqCorrMeanDeviat & r) -> bool
                    {
                        return l.period < r.period;
                    });
                }
            }
            else {
                // just search and return a single value instead of sort

                // CAUTION:
                //  The predicate second argument does remember on return `true`.
                //
                const SyncseqCorrMeanDeviat max_corr_mean_deviat_sum = *std::max_element(begin_it, end_it, [](const SyncseqCorrMeanDeviat & l, const SyncseqCorrMeanDeviat & r) -> bool
                {
                    // take with lowest period and offset if correlation mean deviation sum values are equal
                    return l.corr_mean_deviat_sum > r.corr_mean_deviat_sum || l.corr_mean_deviat_sum == r.corr_mean_deviat_sum && (l.offset > r.offset || l.offset == r.offset && l.period > r.period);
                });

                corr_min_mean_deviat_sum_deq.resize(1);

                corr_min_mean_deviat_sum_deq[0] = max_corr_mean_deviat_sum;
            }

            end_calc_phase_time(_T("corr min mean deviat"), begin_calc_min_mean_deviat_time, corr_out_params.calc_time_phases);
        }
    } break;

    case Impl::impl_max_weighted_autocorr_of_corr_values:
    {
        // Phase 2:
        //
        //  Correlation values weighted autocorrelation calculation, significantly increases algorithm certainty or false positive stability within input noise.
        //
        // Produces correlation values with noticeable certainty tolerance around 33% from the math expected value of number `one` bits per synchro sequence length,
        // where math expected value is equal to the half of synchro sequence length.
        // For example, if synchro sequence has 20 bits length, then algorithm would output uncertain correlation values after a value greater than 33% of noised
        // bits of 10 bits, i.e. greater than ~3 noised bits per each 20 bits in a bit stream.
        //
        //  Note:
        //    Even if the algorithm has a complete instability for not false positive output if over a tolerance, but nonetheless that does not mean it has
        //    100% stability below the tolerance. In other words the algorithm just has no noted instability for not false positive output below the tolerance.
        //    But still sometimes it happens to be instable below but near the tolerance.
        //
        // Major memory complexity:     O(N)
        //
        // Major Time complexity:       O(N * N / 4) or O(N * N)
        //                                , where N - stream bit length
        //
        //  NOTE:
        //    1. Autocorrelation is sensitive to input noise and makes uncertain result because of it.
        //       You have to cut off that noise by using `/min-corr` option.
        //

        const auto begin_calc_phase_time = std::chrono::high_resolution_clock::now();

        // Calculate autocorrelation values.
        //
        // Major time complexity: O(N * N), where N - stream bit length
        //

        uint64_t min_offset_shift = (std::min)(stream_min_period, stream_bit_size - 1);
        uint64_t max_offset_shift = stream_bit_size - 1;
        
        if (syncseq_max_repeat != math::uint32_max) {
            max_offset_shift = (std::min)(max_offset_shift, stream_max_period * syncseq_max_repeat);
        }

        // NOTE:
        //  The minimum offset is much important than the maximum offset, because a minimal offset contains minimal number of repeats which
        //  cuts off not enough certain or noisy values (than greater the repeat, then more certain the mean value calculated from a period with N repeats).
        //  So the number of an offset shifts (positions) does reduce by the maximum offset at first, but does increase by the minimum offset at second.
        //
        auto num_offset_shifts = max_offset_shift + 1;

        num_offset_shifts = (std::max)(num_offset_shifts, min_offset_shift + 1);

        // CAUTION:
        //  Autocorrelation does decrease the effective length of input buffer bit stream and may violate consistency of input conditions has checked on consistency before.
        //  We must check on input conditions consistency again after recalculation.
        //

        if (stream_bit_size < stream_min_period + num_offset_shifts) {
            // recalculate
            num_offset_shifts = stream_bit_size - stream_min_period;

            // recheck the input conditions consistency

            if (corr_in_params.period_min_repeat) {
                if (corr_in_params.max_period != math::uint32_max && corr_in_params.max_period * corr_in_params.period_min_repeat >= num_offset_shifts ||
                    corr_in_params.min_period && corr_in_params.min_period != math::uint32_max && corr_in_params.min_period * corr_in_params.period_min_repeat >= num_offset_shifts) {
                    // recalculated stream bit size is increased, important input condition is changed and reduced the field of search of the result, quit calculation
                    corr_out_params.input_inconsistency = true;
                }
            }
            else {
                if (corr_in_params.max_period != math::uint32_max && corr_in_params.max_period >= num_offset_shifts ||
                    corr_in_params.min_period && corr_in_params.min_period != math::uint32_max && corr_in_params.min_period >= num_offset_shifts) {
                    // recalculated stream bit size is increased, important input condition is changed and reduced the field of search of the result, quit calculation
                    corr_out_params.input_inconsistency = true;
                }
            }
        }

        const auto num_autocorr_values = size_t(min_offset_shift + num_offset_shifts);
        //const auto num_syncseq_autocorr_values = size_t((min_offset_shift + num_offset_shifts) * syncseq_bit_size);
        //const auto num_syncseq_offset_shifts = size_t(num_offset_shifts * syncseq_bit_size);

        corr_autocorr_arr.reserve(size_t(num_offset_shifts));

        for (size_t i = 0; i < size_t(num_offset_shifts); i++) {
            corr_autocorr_arr.push_back(SyncseqCorr{ 0, uint32_t(stream_min_period + i), 0, 0 });
        }

        // calculate correlation square values
        std::vector<float> corr_square_values_arr(num_autocorr_values);

        // calculate correlation denominator accumulated values
        std::vector<float> corr_denominator_first_accum_value_arr(size_t(num_offset_shifts), 0);
        std::vector<float> corr_denominator_second_accum_value_arr(size_t(num_offset_shifts), 0);

        for (size_t i = 0; i < num_offset_shifts; i++) {
            const auto & corr_value = corr_values_arr[i];

            corr_square_values_arr[i] = corr_value * corr_value;
        }

        float corr_denominator_first_accum_value = 0;
        float corr_denominator_second_accum_value = 0;

        for (size_t i = 0; i < num_offset_shifts; i++) {
            corr_denominator_first_accum_value += corr_square_values_arr[i];
            corr_denominator_second_accum_value += corr_square_values_arr[size_t(stream_min_period + num_offset_shifts - i - 1)];

            corr_denominator_first_accum_value_arr[size_t(num_offset_shifts - i - 1)] = corr_denominator_first_accum_value;
            corr_denominator_second_accum_value_arr[size_t(num_offset_shifts - i - 1)] = corr_denominator_second_accum_value;
        }

        uint32_t num_corr_values_iter = 0;

        // result of 2 functions multiplication
        float corr_numerator_value;

        uint32_t num_corr;

        for (size_t i = 0, offset_shift = size_t(stream_min_period); max_offset_shift >= offset_shift && num_offset_shifts >= min_offset_shift; i++, offset_shift++, num_offset_shifts--) {
            auto & autocorr = corr_autocorr_arr[i];

            // result of 2 functions multiplication
            corr_numerator_value = 0;

            num_corr = 0;

            for (size_t j = 0; j < num_offset_shifts; j++) {
                const float corr_value =  corr_values_arr[j] * corr_values_arr[j + offset_shift];

                // count not zero
                if (corr_value) {
                    corr_numerator_value += corr_value;
                    num_corr++;
                }

                num_corr_values_iter++;
            }

            // NOTE:
            //  1. The `num_autocorr_values` here is the entire correlation set normalization factor, because the rest of formula has already normalized to [0; 0.1].
            //  2. No need to return correlation values back to linear, because they only used for a sort.
            //
            autocorr.corr_value = corr_numerator_value * num_autocorr_values / (std::max)(corr_denominator_first_accum_value_arr[i], corr_denominator_second_accum_value_arr[i]);
            autocorr.num_corr = num_corr;
        }

        corr_out_params.num_corr_values_iterated = num_corr_values_iter;

        auto begin_it = corr_autocorr_arr.begin();
        auto end_it = corr_autocorr_arr.end();

        if (corr_in_params.return_sorted_result) {
            // sort by correlation values

            std::sort(begin_it, end_it, [](const SyncseqCorr & l, const SyncseqCorr & r) -> bool
            {
                return l.corr_value > r.corr_value;
            });

            // sort by offset in a correlation value

            size_t step;

            auto first_it = begin_it;

            auto prev_it = begin_it;
            auto next_it = prev_it;

            for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                auto & corr_max_mean_prev_ref = *prev_it;
                auto & corr_max_mean_next_ref = *next_it;

                if (corr_max_mean_prev_ref.corr_value != corr_max_mean_next_ref.corr_value) {
                    if (step > 1) {
                        std::sort(first_it, next_it, [](const SyncseqCorr & l, const SyncseqCorr & r) -> bool
                        {
                            return l.offset < r.offset;
                        });
                    }

                    first_it = next_it;
                    step = 0;
                }
            }

            // sort the last
            if (step > 1) {
                std::sort(first_it, end_it, [](const SyncseqCorr & l, const SyncseqCorr & r) -> bool
                {
                    return l.offset < r.offset;
                });
            }

            // sort by period in an offset

            first_it = begin_it;

            prev_it = begin_it;
            next_it = prev_it;

            for (step = 0, next_it++; next_it != end_it; step++, prev_it = next_it, next_it++) {
                auto & corr_max_mean_prev_ref = *prev_it;
                auto & corr_max_mean_next_ref = *next_it;

                if (corr_max_mean_prev_ref.corr_value != corr_max_mean_next_ref.corr_value || corr_max_mean_prev_ref.offset != corr_max_mean_next_ref.offset) {
                    if (step > 1) {
                        std::sort(first_it, next_it, [&](const SyncseqCorr & l, const SyncseqCorr & r) -> bool
                        {
                            return l.period < r.period;
                        });
                    }

                    first_it = next_it;
                    step = 0;
                }
            }

            // sort the last
            if (step > 1) {
                std::sort(first_it, end_it, [&](const SyncseqCorr & l, const SyncseqCorr & r) -> bool
                {
                    return l.period < r.period;
                });
            }
        }
        else {
            // just search and return a single value instead of sort

            // CAUTION:
            //  The predicate second argument does remember on return `true`.
            //
            const SyncseqCorr max_corr_autocorr = *std::max_element(begin_it, end_it, [](const SyncseqCorr & l, const SyncseqCorr & r) -> bool
            {
                // take with lowest period and offset if correlation mean deviation sum values are equal
                return l.corr_value < r.corr_value || l.corr_value == r.corr_value && (l.offset > r.offset || l.offset == r.offset && l.period > r.period);
            });

            corr_autocorr_arr.resize(1);

            corr_autocorr_arr[0] = max_corr_autocorr;
        }

        end_calc_phase_time(_T("corr autocorr"), begin_calc_phase_time, corr_out_params.calc_time_phases);

    } break;

    default:
    {
        assert(0); // not implemented
    } break;
    }
}

// Search algorithm false positive statistic calculation code to test an algorithm phase for stability within input noise.
//
void calculate_syncseq_correlation_false_positive_stats(
    const std::vector<float> &              corr_values_arr,
    const std::deque<SyncseqCorrMean> *     corr_max_weighted_mean_sum_deq_ptr,
    const std::deque<SyncseqCorrMeanDeviat> * corr_min_mean_deviat_sum_deq_ptr,
    const std::vector<uint32_t> &           true_positions_index_arr,
    size_t &                                true_num,
    size_t                                  stat_arrs_size,
    std::vector<float> &                    false_max_corr_arr,
    std::vector<size_t> &                   false_max_index_arr,
    std::vector<float> &                    true_max_corr_arr,
    std::vector<size_t> &                   true_max_index_arr,
    std::vector<float> &                    false_in_true_max_corr_arr,
    std::vector<size_t> &                   false_in_true_max_index_arr,
    std::vector<float> &                    saved_true_in_false_max_corr_arr,
    std::vector<size_t> &                   saved_true_in_false_max_index_arr,
    std::vector<SyncseqCorrMeanStats> *     false_in_true_corr_max_weighted_mean_sum_arr_ptr,
    std::vector<SyncseqCorrMeanDeviatStats> * false_in_true_corr_min_mean_deviat_sum_arr_ptr)
{
    false_max_corr_arr.resize(stat_arrs_size);
    false_max_index_arr.resize(stat_arrs_size);

    true_max_corr_arr.resize(stat_arrs_size);
    true_max_index_arr.resize(stat_arrs_size);

    false_in_true_max_corr_arr.resize(stat_arrs_size);
    false_in_true_max_index_arr.resize(stat_arrs_size);

    saved_true_in_false_max_corr_arr.reserve(sizeof(true_positions_index_arr) / sizeof(true_positions_index_arr[0]));
    saved_true_in_false_max_index_arr.reserve(sizeof(true_positions_index_arr) / sizeof(true_positions_index_arr[0]));

    true_num = 0;

    false_max_corr_arr[0] = true_max_corr_arr[0] = false_in_true_max_corr_arr[0] = corr_values_arr[0];
    false_max_index_arr[0] = true_max_index_arr[0] = false_in_true_max_index_arr[0] = 0;

    bool is_true_position_index = false;
    for (auto true_position_index : true_positions_index_arr) {
        if (true_position_index == false_max_index_arr[0]) {
            is_true_position_index = true;
            false_max_index_arr[0] = 0;
            break;
        }
    }
    if (is_true_position_index) {
        true_num++;
    }
    else {
        true_max_corr_arr[0] = 0;
        true_max_index_arr[0] = 0;
    }

    for (size_t i = 1; i < corr_values_arr.size(); i++) {
        const float v = corr_values_arr[i];

        is_true_position_index = false;

        for (auto true_position_index : true_positions_index_arr) {
            if (true_position_index == i) {
                is_true_position_index = true;
                break;
            }
        }

        bool is_found = false;

        for (size_t j = 0; j < false_in_true_max_corr_arr.size(); j++) {
            if (false_in_true_max_corr_arr[j] < v) {
                is_found = true;

                if (false_in_true_max_corr_arr[j]) {
                    // move true position correlation values out of array to restore them later
                    if (false_in_true_max_corr_arr.back()) {
                        for (auto true_position_index : true_positions_index_arr) {
                            if (true_position_index == false_in_true_max_index_arr.back()) {
                                saved_true_in_false_max_corr_arr.push_back(false_in_true_max_corr_arr.back());
                                saved_true_in_false_max_index_arr.push_back(true_position_index);
                                break;
                            }
                        }
                    }

                    for (size_t k = false_in_true_max_corr_arr.size(); --k > j; ) {
                        if (false_in_true_max_corr_arr[k - 1]) {
                            false_in_true_max_corr_arr[k] = false_in_true_max_corr_arr[k - 1];
                            false_in_true_max_index_arr[k] = false_in_true_max_index_arr[k - 1];
                        }
                    }
                }

                false_in_true_max_corr_arr[j] = v;
                false_in_true_max_index_arr[j] = i;
                break;
            }
        }

        if (is_true_position_index) {
            true_num++;

            if (!is_found) {
                saved_true_in_false_max_corr_arr.push_back(v);
                saved_true_in_false_max_index_arr.push_back(i);
            }

            for (size_t j = 0; j < true_max_corr_arr.size(); j++) {
                if (!true_max_corr_arr[j] || true_max_corr_arr[j] > v) {
                    if (true_max_corr_arr[j]) {
                        for (size_t k = true_max_corr_arr.size(); --k > j; ) {
                            true_max_corr_arr[k] = true_max_corr_arr[k - 1];
                            true_max_index_arr[k] = true_max_index_arr[k - 1];
                        }
                    }
                    true_max_corr_arr[j] = v;
                    true_max_index_arr[j] = i;
                    break;
                }
            }
        }
        else {
            for (size_t j = 0; j < false_max_corr_arr.size(); j++) {
                if (false_max_corr_arr[j] < v) {
                    if (false_max_corr_arr[j]) {
                        for (size_t k = false_max_corr_arr.size(); --k > j; ) {
                            false_max_corr_arr[k] = false_max_corr_arr[k - 1];
                            false_max_index_arr[k] = false_max_index_arr[k - 1];
                        }
                    }
                    false_max_corr_arr[j] = v;
                    false_max_index_arr[j] = i;
                    break;
                }
            }
        }
    }

    for (size_t j = 0; j < false_in_true_max_corr_arr.size(); j++) {
        for (auto true_position_index : true_positions_index_arr) {
            if (true_position_index == false_in_true_max_index_arr[j]) {
                false_in_true_max_corr_arr[j] = 0;
                false_in_true_max_index_arr[j] = 0;
                break;
            }
        }
    }

    // clear the array end using true position correlation values moved out of array
    for (size_t i = 0, j = false_in_true_max_corr_arr.size() - 1; i < saved_true_in_false_max_corr_arr.size(); i++, j--) {
        false_in_true_max_corr_arr[j] = 0;
        false_in_true_max_index_arr[j] = 0;
        if (!j) break;
    }

    // the second and third phase algorithm output analysis
    if (corr_max_weighted_mean_sum_deq_ptr && false_in_true_corr_max_weighted_mean_sum_arr_ptr) {
        const auto & corr_max_weighted_mean_sum_deq = *corr_max_weighted_mean_sum_deq_ptr;
        auto & false_in_true_corr_max_weighted_mean_sum_arr = *false_in_true_corr_max_weighted_mean_sum_arr_ptr;

        false_in_true_corr_max_weighted_mean_sum_arr.resize((std::min)(stat_arrs_size, corr_max_weighted_mean_sum_deq.size()));

        for (size_t j = 0; j < false_in_true_corr_max_weighted_mean_sum_arr.size(); j++) {
            auto & corr_mean_ref = corr_max_weighted_mean_sum_deq[j];
            auto & false_in_true_corr_max_weighted_mean_sum_ref = false_in_true_corr_max_weighted_mean_sum_arr[j];

            false_in_true_corr_max_weighted_mean_sum_ref = SyncseqCorrMeanStats{ corr_mean_ref, false };

            for (auto true_position_index : true_positions_index_arr) {
                if (true_position_index == corr_mean_ref.offset) {
                    false_in_true_corr_max_weighted_mean_sum_ref.is_true = true;
                    break;
                }
            }
        }
    }

    if (corr_min_mean_deviat_sum_deq_ptr && false_in_true_corr_min_mean_deviat_sum_arr_ptr) {
        const auto & corr_corr_min_mean_deviat_sum_deq = *corr_min_mean_deviat_sum_deq_ptr;
        auto & false_in_true_corr_min_mean_deviat_sum_arr = *false_in_true_corr_min_mean_deviat_sum_arr_ptr;

        false_in_true_corr_min_mean_deviat_sum_arr.resize((std::min)(stat_arrs_size, corr_corr_min_mean_deviat_sum_deq.size()));

        for (size_t j = 0; j < false_in_true_corr_min_mean_deviat_sum_arr.size(); j++) {
            auto & corr_mean_deviat_ref = corr_corr_min_mean_deviat_sum_deq[j];
            auto & false_in_true_corr_max_weighted_mean_sum_ref = false_in_true_corr_min_mean_deviat_sum_arr[j];

            false_in_true_corr_max_weighted_mean_sum_ref = SyncseqCorrMeanDeviatStats{ corr_mean_deviat_ref, false };

            for (auto true_position_index : true_positions_index_arr) {
                if (true_position_index == corr_mean_deviat_ref.offset) {
                    false_in_true_corr_max_weighted_mean_sum_ref.is_true = true;
                    break;
                }
            }
        }
    }
}
