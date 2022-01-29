// Author: Andrey Dibrov (andry at inbox dot ru)
//

#include "autocorrelation.hpp"

#include <boost/utility/binary.hpp>

#include <tacklelib/utility/math.hpp>

#include <limits>
#include <cmath>

// The autocorrelation function algorithm for a synchro sequence in a bit stream.
//

//  Auto correlation function default range:
//
//      0 - minimal correlation value.
//     +1 - maximal positive correlation value.
//     -1 - maximal negative correlation value.
//

//  The general approach, conditions and properties:
//
//  1. We must represent a bit stream together with a synchro sequence as a set of natural values greater or equal of 1 to build a function
//     with natural values (no need to generate real values from a bit set) to multiply functions.
//     In other words we have to generate more non bit values for comparison than the bit stream length instead of bit values from a bit stream.
//
//     Update: But because the generator function already tested for stable results and can be reduced, then we can leave the length of
//             a function to generate the same size as input bit set length.
//
//     The same for the synchro sequence bit set.
//     For this reason the generator function is exist.
//
//  2. Both functions basically will not infinitely increase or decrease, otherwise the different parts of the same function
//     can be relatively equal if min/max of both is normalized to the same range, but is different if not normalized.
//     Because both functions does not infinitely increase or decrease (with or without a limit), then we can compare without maximum and minimum
//     normalization and can avoid a function rescale and resample.
//
//  3. The function multiplication to the exactly same function must gain the maximal correlation result from the range [0; 1], i.e. 1.
//     To achieve that we multiply one function to another and divide by a possible maximum.
//     For example, if absolute maximum one of the functions is greater, then we just divide by greater absolute maximum to normalize a correlation value to range <= 1.
//
//  4. We must avoid a function [-inf; 0] range for natural numbers and [-inf; 1] for real numbers because:
//
//       Multiplication to a negative value can reduce the sum instead of increase it, so we may gain a lesser value related to a maximum.
//       In real numbers quantity a multiplication of a value from the range (0; 1) with a value from the range [1; +inf] may gain a lesser absolute value.
//       Multiplication by 0 gets the lesser absolute value too.
//
//     To avoid reduction of the sum, we always y-shift all functions on +1 independently to a maximum and minimum to shift all values into [1; +inf] range.
//
//  5. Because we don't need an inversed or maximal negative correlation range - `[-1; 0)` (an inversed synchro sequence does not need to be found),
//     then we search solutions only in the positive numbers quantity - `[1; +inf]`.
//
//  6. Because multiplication of functions does gain a square factor value, then we may take the square root to return back to linear value.
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
//            fa(x) * fb(x)   - normalized linear autocorrelation value in range (0; 1]
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
//      fp * fq = (a + c) * c + (b * d) * b
//      fp * fp = (a + c) ^ 2 + d ^ 2
//      fq * fq = (b + d) ^ 2 + c ^ 2
//      a > 0, b > 0, c >= 1, d >= 1
//
//      then:
//
//        `(a + c) * c + (b + d) * b <= max((a + c) ^ 2 + d ^ 2, (b + d) ^ 2 + c ^ 2)` - must has a solution
//
//      To solve this we can use `Wolfram Alpha` online:
//
//        * `(a + c) * c + (b + d) * b <= max((a + c) ^ 2 + d ^ 2, (b + d) ^ 2 + c ^ 2), a > 0, b > 0, c >= 1, d >= 1`:
//
//          https://www.wolframalpha.com/input/?i=%28a+%2B+c%29+*+c+%2B+%28b+%2B+d%29+*+b+<%3D+max%28%28a+%2B+c%29+%5E+2+%2B+d+%5E+2%2C+%28b+%2B+d%29+%5E+2+%2B+c+%5E+2%29%2C+a+>+0%2C+b+>+0%2C+c+>%3D+1%2C+d+>%3D+1
//
//          Solution:
//
//            a c + b^2 + b d + c^2 <=
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
//        * `(x + u) * u + (y * v) * y <= max((x + u) ^ 2 + v ^ 2, (y + v) ^ 2 + u ^ 2), x > 0, y > 0, u >= 1, v >= 1`:
//
//          https://www.wolframalpha.com/input/?i=%28x+%2B+u%29+*+u+%2B+%28y+%2B+v%29+*+y+<%3D+max%28%28x+%2B+u%29+%5E+2+%2B+v+%5E+2%2C+%28y+%2B+v%29+%5E+2+%2B+u+%5E+2%29%2C+x+>+0%2C+y+>+0%2C+u+>%3D+1%2C+v+>%3D+1
//
//          Solution:
//
//            u>=1, v>=u, x>0, y>0
//            u>=1, 1<=v<u, x>(u^2 sqrt((v^4 (2 u^2 - v^2))/(u^2 - v^2)^2) - v^2 sqrt((v^4 (2 u^2 - v^2))/(u^2 - v^2)^2) + u v^2)/(u^2 - v^2), y>=(u x - v^2)/v
//            u>=1, 1<=v<u, x>(u^2 sqrt((v^4 (2 u^2 - v^2))/(u^2 - v^2)^2) - v^2 sqrt((v^4 (2 u^2 - v^2))/(u^2 - v^2)^2) + u v^2)/(u^2 - v^2), 0<y<=1/2 (sqrt(4 u x + 5 v^2 + 4 x^2) - v)
//            u>=1, 1<=v<u, 0<x<=(u^2 sqrt((v^4 (2 u^2 - v^2))/(u^2 - v^2)^2) - v^2 sqrt((v^4 (2 u^2 - v^2))/(u^2 - v^2)^2) + u v^2)/(u^2 - v^2), y>0
//
//            , where:
//
//              x, y, u, v  - real numbers
//
//        * `Solve [(x + z) * z + (y + w) * y <= max((x + z) ^ 2 + w ^ 2, (y + w) ^ 2 + z ^ 2), x > 0, y > 0, z >= 1, w >= 1]`:
//
//          https://www.wolframalpha.com/input/?i=Solve+%5B%28x+%2B+z%29+*+z+%2B+%28y+*+w%29+*+y+<%3D+max%28%28x+%2B+z%29+%5E+2+%2B+w+%5E+2%2C+%28y+%2B+w%29+%5E+2+%2B+z+%5E+2%29%2C+x+>+0%2C+y+>+0%2C+z+>%3D+1%2C+w+>%3D+1%5D
//
//          Solution (most simple):
//
//            w>=1 and x>0 and 0<y<=sqrt(2 w^2 + x^2) and z>=1
//            w >= 1 and x > 0 and y > sqrt(2 w ^ 2 + x ^ 2) and 1 <= z <= (w ^ 2 + w y) / x
//            w>=1 and x>0 and y>sqrt(2 w^2 + x^2) and z>=(-w^2 + w y - x^2 + y^2)/x
//
//            , where:
//
//              x, y, z, w  - real numbers
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
//  1. Generate autocorrelation functions for a synchro sequence and for a bit stream with values in range [+1; 2^16-1] to fit a float type.
//     Function generator must be reliable enough to generate functions with most unique multiplication output for any random input.
//
//  2. Multiply functions and divide the result by the maximum of absolute maximums of 2 functions.
//
//  3. Take the square root to convert value back to linear correlation value.
//
//     Examples:
//
//       Equality(f1, f1) = sqrt( 21 / 21 ) = 1 (maximum positive correlation value)
//       Equality(fa, fa) = Equality(fb, fb) = 1
//
//       Equality(f1, f2) = sqrt( 12 / max(21, 21) ) = ~0.76
//       Equality(f1, f3) = sqrt( 24 / max(21, 36) ) = ~0.82
//       Equality(f1, f4) = sqrt( 39 / max(21, 129) ) = ~0.55
//       Equality(f1, f5) = sqrt( 14 / max(21, 41) ) = ~0.58
//
//       Equality(f2, f3) = sqrt( 24 / max(21, 36) ) = ~0.82
//       Equality(f2, f4) = sqrt( 48 / max(21, 129) ) = ~0.61
//       Equality(f2, f5) = sqrt( 29 / max(21, 41) ) = ~0.84
//       Equality(f2, f6) = sqrt( 42 / max(21, 84) ) = ~0.71
//
//       Equality(f3, f4) = sqrt( 60 / max(36, 129) ) = ~0.68
//       Equality(f3, f5) = sqrt( 32 / max(36, 41) ) = ~0.88
//       Equality(f3, f6) = sqrt( 48 / max(36, 84) ) = ~0.76
//
//       Equality(f4, f5) = sqrt(62 / max(129, 41)) = ~0.69
//       Equality(f4, f6) = sqrt(96 / max(129, 84)) = ~0.86
//
//       Equality(f5, f6) = sqrt( 58 / max(41, 84) ) = ~0.83
//
//  4. Use autocorrelation mean values calculation for all pairs of offset and period in a bit stream within some distance.
//     May needs too much memory and can be too slow, so do use the optimization command line parameters to reduce memory consumption and speedup the calculation
//     for the price of may be higher risk of uncertainty or false positive result:
//      * A stream buffer size must be enough to fit a complete real stream period length.
//      * Minimal period length must be less than a real period length.
//      * A memory buffer for autocorrelation mean values must be enough to include at least one period.
//      * A maximal repeat quantity must be enough to accumulate enough certainty for a particular offset and period.
//      * etc
//     If all conditions are met, the result can be much stabler to the input noise than just a simple autocorrelation values set without the mean calculation.
//

// Autocorrelation complement function generator to make a function to multiply with most correlation equality within the noise when 2 functions are equal without the noise.
//
// Has a set of properties:
//
//  1. Generate a set of values in range of [1; 2^16-1] from arbitrary bit set.
//  2. To make a stable multiplication with minimal affect from input noise, where the true positions in a bit stream with equal or
//     most equal comparison would have higher correlation values and all others would have lower correlation values.
//
extern inline void generate_autocorr_complement_func_from_bitset(uint32_t value, uint32_t value_bit_size, uint32_t * out_ptr, size_t offset)
{
    static const uint32_t s_default_prime_numbers_arr[] = { // CAUTION: for 32-bit blocks
        1249,
        1237,
        1231,
        1229,
        1223,
        1217,
        1213,
        1201,
        1193,
        1187,
        1181,
        1171,
        1163,
        1153,
        1151,
        1129,
        1123,
        1117,
        1109,
        1103,
        1097,
        1093,
        1091,
        1087,
        1069,
        1063,
        1061,
        1051,
        1049,
        1039,
        1033,
        1031
    };

    const size_t function_len = value_bit_size;

    for (size_t i = 0, j = function_len - 1; i < function_len; i++, j--) {
        out_ptr[offset] = (value & (uint32_t(0x01) << i)) ? s_default_prime_numbers_arr[i] : (i + 1) * 2;
        //out_ptr[offset] = (value & (uint32_t(0x01) << i)) ? 1000 + i : (i + 1) * 2;
        offset++;
    }
}

extern inline uint64_t multiply_autocorr_funcs(const uint32_t * in0_ptr, const uint32_t * in1_ptr, size_t size)
{
    uint64_t sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += uint64_t(in0_ptr[i]) * in1_ptr[i];
    }

    return sum;
}

// returns float instead of double because in0/in1/max_in0/max_in1 contains values in range [1; 2^16-1]
//
extern inline float autocorr_value(const uint32_t * in0_ptr, const uint32_t * in1_ptr, size_t size, uint64_t max_in0, uint64_t max_in1)
{
    const float autocorr = sqrtf(float(multiply_autocorr_funcs(in0_ptr, in1_ptr, size)) / (std::max)(max_in0, max_in1));

    // zero padded values from padded arrays must not be passed here
    assert(0 < autocorr && 1.0f >= autocorr); // must be always in range (0; 1]

    return autocorr;
}

void calculate_syncseq_autocorrelation(
    uint8_t *                           stream_buf,
    uint64_t                            stream_bit_size,
    uint32_t                            stream_min_period,
    uint32_t                            stream_max_period,
    uint32_t                            syncseq_bit_size,
    uint32_t                            syncseq_bytes,
    uint32_t                            syncseq_min_repeat,
    uint32_t                            syncseq_max_repeat,
    std::vector<float> &                autocorr_values_arr,
    uint64_t                            autocorr_mean_buf_max_size,
    std::deque<SyncseqAutocorr> *       autocorr_mean_deq_ptr)
{
    assert(syncseq_bit_size < stream_bit_size); // must be greater

    // Buffer is already padded to a multiple of 4 bytes plus 4 bytes reminder to be able to read and shift the last 32-bit block as 64-bit block.
    //
    const uint64_t padded_stream_bit_size = (stream_bit_size + 31) & ~uint32_t(31);
    const uint64_t num_stream_32bit_blocks = padded_stream_bit_size / 32;
    uint32_t * stream_buf32 = (uint32_t *)stream_buf;

    std::vector<uint32_t> syncseq_bits_block_autocorr_weight_arr((size_t(stream_bit_size)));
    std::vector<uint32_t> stream_bits_block_autocorr_weight_arr((size_t(stream_bit_size * syncseq_bit_size)));

    std::vector<uint64_t> stream_autocorr_absmax_arr((size_t(stream_bit_size)));
    uint64_t syncseq_autocorr_absmax_arr = 1; // minimum

    autocorr_values_arr.resize((size_t(stream_bit_size)));

    // generate autocorrelation complement function from the synchro sequence
    generate_autocorr_complement_func_from_bitset(syncseq_bytes, syncseq_bit_size, &syncseq_bits_block_autocorr_weight_arr[0], 0);

    // generate autocorrelation complement function from the bit stream
    const uint32_t syncseq_mask = uint32_t(~(~uint64_t(0) << syncseq_bit_size));

    uint64_t stream_bit_offset = 0;
    bool break_ = false;

    // linear complexity
    for (uint32_t from = 0; from < num_stream_32bit_blocks; from++) {
        const uint64_t from64 = *(uint64_t *)(stream_buf32 + from);

        for (uint32_t i = 0; i < 32; i++) {
            const uint32_t from_shifted = uint32_t(from64 >> i) & syncseq_mask;

            generate_autocorr_complement_func_from_bitset(from_shifted, syncseq_bit_size, &stream_bits_block_autocorr_weight_arr[size_t(stream_bit_offset * syncseq_bit_size)], 0);

            stream_bit_offset++;

            // avoid calculation from zero padded array values to avoid shift from range (0; 1] to range [0; 1]
            if (stream_bit_offset >= stream_bit_size) {
                break_ = true;
                break;
            }
        }

        if (break_) break;
    }

    // calculate absolute maximums for the synchro sequence autocorrelation function
    syncseq_autocorr_absmax_arr = multiply_autocorr_funcs(
        syncseq_bits_block_autocorr_weight_arr.data(),
        syncseq_bits_block_autocorr_weight_arr.data(),
        syncseq_bit_size);

    // calculate absolute maximums for the bit stream autocorrelation function
    for (size_t i = 0; i < stream_bit_size; i++) {
        stream_autocorr_absmax_arr[i] = multiply_autocorr_funcs(
            &stream_bits_block_autocorr_weight_arr[i * syncseq_bit_size],
            &stream_bits_block_autocorr_weight_arr[i * syncseq_bit_size],
            syncseq_bit_size);
    }

    // First phase:
    //  Autocorrelation values calculation, creates a moderate but still instable algorithm certainty or false positive stability within input noise.
    //
    // Produces correlation values with noticeable certainty tolerance around 33% from the math expected value of number `one` bits per synchro sequence length,
    // where math expected value is equal to half of synchro sequence length.
    // For example, if synchro sequence has 20 bits length, then algorithm would output uncertain correlation values after a value greater than 33% of noised
    // bits of 10 bits, i.e. greater than ~3 noised bits per 20 bit synchro sequence.
    //
    // Complexity: O(N * M), where N - stream bit length, M - synchro sequence bit length
    //

    for (size_t i = 0; i < stream_bit_size; i++) {
        auto & autocorr_value_ref = autocorr_values_arr[i];
        autocorr_value_ref =
            autocorr_value(
                syncseq_bits_block_autocorr_weight_arr.data(),
                &stream_bits_block_autocorr_weight_arr[i * syncseq_bit_size],
                syncseq_bit_size,
                syncseq_autocorr_absmax_arr,
                stream_autocorr_absmax_arr[i]);
    }

    // Second phase:
    //  Autocorrelation mean (average) values calculation, significantly increases algorithm certainty or false positive stability within input noise.
    //
    // Produces correlation mean values with noticeable certainty tolerance around 66% from the math expected value of number `one` bits per synchro sequence length,
    // where math expected value is equal to half of synchro sequence length.
    // For example, if synchro sequence has 20 bits length, then algorithm would output uncertain correlation mean values after a value greater than 66% of noised
    // bits of 10 bits, i.e. greater ~6 noised bits per 20 bit synchro sequence.
    //
    // Complexity: O(N * N * N), where N - stream bit length
    //

    if (autocorr_mean_deq_ptr) {
        auto & autocorr_mean_deq = *autocorr_mean_deq_ptr;

        const uint64_t stream_from_period = stream_min_period != math::uint32_max ? (std::max)(syncseq_bit_size + 1, stream_min_period) : syncseq_bit_size + 1;
        const uint64_t stream_to_period = stream_max_period;

        const uint64_t stream_from_period_bit_size = stream_bit_size - stream_from_period + 1; // excluding synchro sequence and 1 data bit

        // CAUTION:
        //
        //  Can consume too much memory, because to find a stream period does need at least
        //  `(stream_bit_size - syncseq_bit_size) * T * sizeof(SyncseqAutocorr)` bytes to store correlation mean values,
        //  where - T - the real stream period!
        //
        const uint64_t autocorr_mean_deq_max_size = (std::min)(
            autocorr_mean_buf_max_size / sizeof(autocorr_mean_deq[0]),
            stream_from_period_bit_size * (stream_from_period_bit_size - 1) / 2); // all possible stream period variants but not maximum

        //autocorr_mean_deq.reserve(size_t(autocorr_mean_deq_max_size));

        for (uint64_t i = 0, period = stream_from_period, j = i + period; i < stream_from_period_bit_size - 1 && i < stream_to_period; i++, period = stream_from_period, j = i + period) {
            if (syncseq_min_repeat >= (stream_bit_size - i + period - 1) / period) {
                break;
            }

            autocorr_mean_deq.push_back(SyncseqAutocorr{});

            SyncseqAutocorr & autocorr_mean_ref = autocorr_mean_deq.back();

            autocorr_mean_ref.corr_mean += autocorr_values_arr[size_t(i)];
            autocorr_mean_ref.num_corr++;
            autocorr_mean_ref.offset = i;

            if (j >= stream_bit_size || syncseq_max_repeat < autocorr_mean_ref.num_corr) {
                continue;
            }

            autocorr_mean_ref.period = uint32_t(period);

            do {
                autocorr_mean_ref.corr_mean += autocorr_values_arr[size_t(j)];
                autocorr_mean_ref.num_corr++;
                j += period;
            } while (j < stream_bit_size && syncseq_max_repeat >= autocorr_mean_ref.num_corr);

            // calculate mean (average) value
            autocorr_mean_ref.corr_mean /= autocorr_mean_ref.num_corr;

            for (period++, j = i + period; stream_to_period >= period; period++, j = i + period) {
                if ((stream_bit_size - i + period - 1) / period < syncseq_min_repeat + 1) {
                    break;
                }

                autocorr_mean_deq.push_back(SyncseqAutocorr{});

                SyncseqAutocorr & syncseq_next_autocorr_mean_ref = autocorr_mean_deq.back();

                syncseq_next_autocorr_mean_ref.corr_mean += autocorr_values_arr[size_t(i)];
                syncseq_next_autocorr_mean_ref.num_corr++;
                syncseq_next_autocorr_mean_ref.offset = i;

                if (j >= stream_bit_size || syncseq_max_repeat < syncseq_next_autocorr_mean_ref.num_corr) {
                    break;
                }

                syncseq_next_autocorr_mean_ref.corr_mean += autocorr_values_arr[size_t(j)];
                syncseq_next_autocorr_mean_ref.num_corr++;
                syncseq_next_autocorr_mean_ref.period = uint32_t(period);

                if (syncseq_max_repeat < syncseq_next_autocorr_mean_ref.num_corr) {
                    // calculate mean (average) value
                    syncseq_next_autocorr_mean_ref.corr_mean /= syncseq_next_autocorr_mean_ref.num_corr;
                    break;
                }

                do {
                    syncseq_next_autocorr_mean_ref.corr_mean += autocorr_values_arr[size_t(j)];
                    syncseq_next_autocorr_mean_ref.num_corr++;
                    j += period;
                } while (j < stream_bit_size && syncseq_max_repeat >= syncseq_next_autocorr_mean_ref.num_corr);

                // calculate mean (average) value
                syncseq_next_autocorr_mean_ref.corr_mean /= syncseq_next_autocorr_mean_ref.num_corr;
            }
        }
    }
}

// search algorithm false positive statistic calculation code to test algorithm stability within input noise
//
void calculate_syncseq_autocorrelation_false_positive_stats(
    const std::vector<float> &              autocorr_values_arr,
    const std::deque<SyncseqAutocorr> *     autocorr_mean_deq_ptr,
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
    std::vector<SyncseqAutocorrStats> *     false_in_true_max_corr_mean_arr_ptr)
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

    false_max_corr_arr[0] = true_max_corr_arr[0] = false_in_true_max_corr_arr[0] = autocorr_values_arr[0];
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

    for (size_t i = 1; i < autocorr_values_arr.size(); i++) {
        const float v = autocorr_values_arr[i];

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

    if (autocorr_mean_deq_ptr && false_in_true_max_corr_mean_arr_ptr) {
        const auto & autocorr_mean_deq = *autocorr_mean_deq_ptr;
        auto & false_in_true_max_corr_mean_arr = *false_in_true_max_corr_mean_arr_ptr;

        false_in_true_max_corr_mean_arr.resize((std::min)(stat_arrs_size, autocorr_mean_deq.size()));

        for (size_t j = 0; j < false_in_true_max_corr_mean_arr.size(); j++) {
            auto & autocorr_mean_ref = autocorr_mean_deq[j];
            auto & false_in_true_max_corr_mean_ref = false_in_true_max_corr_mean_arr[j];

            false_in_true_max_corr_mean_ref = SyncseqAutocorrStats{ autocorr_mean_ref, false };

            for (auto true_position_index : true_positions_index_arr) {
                if (true_position_index == autocorr_mean_ref.offset) {
                    false_in_true_max_corr_mean_ref.is_true = true;
                    break;
                }
            }
        }
    }
}
