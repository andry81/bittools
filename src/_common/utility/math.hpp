#pragma once

#include "utility/platform.hpp"

#include <cstdint>

#include <utility>


#define INT32_LOG2(x) ::utility::int32_log2<x>::value
#define UINT32_LOG2(x) ::utility::uint32_log2<x>::value

#if defined(ENABLE_POF2_DEFINITIONS) && !defined(DISABLE_POF2_DEFINITIONS)

#define INT32_MULT_POF2(x, y) int32_t(int32_t(x) << INT32_LOG2(y))
#define UINT32_MULT_POF2(x, y) uint32_t(uint32_t(x) << UINT32_LOG2(y))

#define INT32_DIV_POF2(x, y) int32_t(int32_t(x) >> INT32_LOG2(y))
#define UINT32_DIV_POF2(x, y) uint32_t(uint32_t(x) >> UINT32_LOG2(y))

#define INT32_DIVREM_POF2(x, y) ::utility::divrem<int32_t>{ int32_t(x) >> INT32_LOG2(y), int32_t(x) & ((y) - 1) }
#define UINT32_DIVREM_POF2(x, y) ::utility::divrem<uint32_t>{ uint32_t(x) >> UINT32_LOG2(y), uint32_t(x) & ((y) - 1) }

#else

#define INT32_MULT_POF2(x, y) int32_t(int32_t(x) * (y)) //(int32_t(x) * ::utility::int32_pof2<y>::value)
#define UINT32_MULT_POF2(x, y) uint32_t(uint32_t(x) * (y)) //(uint32_t(x) * ::utility::uint32_pof2<y>::value)

#define INT32_DIV_POF2(x, y) int32_t(int32_t(x) / (y)) // (int32_t(x) / ::utility::int32_pof2<y>::value)
#define UINT32_DIV_POF2(x, y) uint32_t(uint32_t(x) / (y)) //(uint32_t(x) / ::utility::uint32_pof2<y>::value)

#define INT32_DIVREM_POF2(x, y) ::utility::divrem<int32_t>{ int32_t(x) / (y), int32_t(x) % (y) } //(int32_t(x) / ::utility::int32_pof2<y>::value), int32_t(x) % (y) }
#define UINT32_DIVREM_POF2(x, y) ::utility::divrem<uint32_t>{ uint32_t(x) / (y), uint32_t(x) % (y) } //(uint32_t(x) / ::utility::uint32_pof2<y>::value), uint32_t(x) % (y) }

#endif


namespace math
{
    template<typename T>
    struct divrem
    {
        T quot;
        T rem;
    };

    template<int32_t x>
    struct int32_log2 {
        static_assert(x && !(x & (x - 1)), "value must be power of 2");
        static const int value = (1 + int32_log2<x / 2>::value);
    };

    template<>
    struct int32_log2<1> {
        static const int value = 0;
    };

    template<uint32_t x>
    struct uint32_log2 {
        static_assert(x && !(x & (x - 1)), "value must be power of 2");
        static const uint32_t value = (1 + uint32_log2<x / 2>::value);
    };

    template<>
    struct uint32_log2<1> {
        static const uint32_t value = 0;
    };

    template<int32_t x>
    struct int32_pof2
    {
        static_assert(x && !(x & (x - 1)), "value must be power of 2");
        static const int32_t value = x;
    };

    template<uint32_t x>
    struct uint32_pof2
    {
        static_assert(x && !(x & (x - 1)), "value must be power of 2");
        static const uint32_t value = x;
    };

    template<typename R, typename T0, typename T1>
    FORCE_INLINE R t_add_no_overflow(T0 a, T1 b)
    {
        R res = R(a + b);
        res |= -(res < a);
        return res;
    }

    template<typename R, typename T0, typename T1>
    FORCE_INLINE R t_sub_no_overflow(T0 a, T1 b)
    {
        R res = R(a - b);
        res &= -(res <= a);
        return res;
    }

    FORCE_INLINE uint32_t uint32_add_no_overflow(uint32_t a, uint32_t b)
    {
        return t_add_no_overflow<uint32_t>(a, b);
    }

    FORCE_INLINE uint64_t uint64_add_no_overflow(uint64_t a, uint64_t b)
    {
        return t_add_no_overflow<uint64_t>(a, b);
    }

    FORCE_INLINE uint32_t uint32_sub_no_overflow(uint32_t a, uint32_t b)
    {
        return t_sub_no_overflow<uint32_t>(a, b);
    }

    FORCE_INLINE uint64_t uint64_sub_no_overflow(uint64_t a, uint64_t b)
    {
        return t_sub_no_overflow<uint64_t>(a, b);
    }
}
