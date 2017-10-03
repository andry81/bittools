#pragma once

#include "debug.hpp"

#ifdef UNIT_TESTS
#include <gtest/gtest.h>
#endif

#include <cassert>

#define ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp, precondition) \
    if (!(precondition)); else if(!!(exp)); else ::utility::debug_break()

#ifdef GTEST_FAIL

#ifdef _MSC_VER
    #if _MSC_VER < 1600 // < MSVC++ 10 (Visual Studio 2010)
        #error lambda is not supported
    #endif
#else
    #if __cplusplus < 201103L
        #error lambda is not supported
    #endif
#endif

// TIPS:
//  * all lambdas captured by reference because of the error in the MSVC 2015:
//    `error C3493 : '...' cannot be implicitly captured because no default capture mode has been specified`
//  * if debugger is attached but `::testing::GTEST_FLAG(break_on_failure)` has not been setted,
//    then an assertion does a post break.

// gtest asserts rebind with the `void` error workaround (C++11 and higher is required)
#undef ASSERT_TRUE
#define ASSERT_TRUE(condition) [&]() -> void { \
        const bool is_success = ::utility::is_true(condition); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_TEST_BOOLEAN_(is_success, #condition, false, true, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_TEST_BOOLEAN_(is_success, #condition, false, true, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(is_success, !break_on_failure); \
    }()
#undef ASSERT_FALSE
#define ASSERT_FALSE(condition) [&]() -> void { \
        const bool is_success = ::utility::is_false(condition); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_TEST_BOOLEAN_(is_success, #condition, true, false, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_TEST_BOOLEAN_(is_success, #condition, true, false, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(is_success, !break_on_failure); \
    }()

#if !GTEST_DONT_DEFINE_ASSERT_EQ
#undef ASSERT_EQ
#define ASSERT_EQ(val1, val2) [&]() -> void { \
        const ::testing::AssertionResult exp_value = ::testing::internal::EqHelper<GTEST_IS_NULL_LITERAL_(val1)>::Compare(#val1, #val2, val1, val2); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
    }()
#endif

#if !GTEST_DONT_DEFINE_ASSERT_NE
#undef ASSERT_NE
#define ASSERT_NE(val1, val2) [&]() -> void { \
        const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperNE(#val1, #val2, val1, val2); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
    }()
#endif

#if !GTEST_DONT_DEFINE_ASSERT_LE
#undef ASSERT_LE
#define ASSERT_LE(val1, val2) [&]() -> void { \
        const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperLE(#val1, #val2, val1, val2); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
    }()
#endif

#if !GTEST_DONT_DEFINE_ASSERT_LT
#undef ASSERT_LT
#define ASSERT_LT(val1, val2) [&]() -> void { \
        const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperLT(#val1, #val2, val1, val2); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
    }()
#endif

#if !GTEST_DONT_DEFINE_ASSERT_GE
#undef ASSERT_GE
#define ASSERT_GE(val1, val2) [&]() -> void { \
        const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperGE(#val1, #val2, val1, val2); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
    }()
#endif

#if !GTEST_DONT_DEFINE_ASSERT_GT
#undef ASSERT_GT
#define ASSERT_GT(val1, val2) [&]() -> void { \
        const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperGT(#val1, #val2, val1, val2); \
        const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
        if (break_on_failure) { \
            GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
        } else { \
            GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
        } \
        ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
    }()
#endif

#define ASSERT(x) ASSERT_TRUE(x)

#else

#ifndef ASSERT_IMPL
#define ASSERT_IMPL(exp) assert(exp)
#endif

#ifdef _DEBUG

#define ASSERT_TRUE(exp) ASSERT_IMPL(exp)
#define ASSERT_FALSE(exp) ASSERT_IMPL(!(exp))

#define ASSERT_EQ(v1, v2) ASSERT_IMPL((v1) == (v2))
#define ASSERT_NE(v1, v2) ASSERT_IMPL((v1) != (v2)))
#define ASSERT_LE(v1, v2) ASSERT_IMPL((v1) <= (v2))
#define ASSERT_LT(v1, v2) ASSERT_IMPL((v1) < (v2))
#define ASSERT_GE(v1, v2) ASSERT_IMPL((v1) >= (v2))
#define ASSERT_GT(v1, v2) ASSERT_IMPL((v1) > (v2))

#define ASSERT(exp) ASSERT_IMPL(exp)

#else

#define ASSERT_TRUE(exp) (::utility::is_true(exp), (void)0)
#define ASSERT_FALSE(exp) (::utility::is_false(exp), (void)0))

#define ASSERT_EQ(v1, v2) (::utility::is_equal(v1, v2), (void)0)
#define ASSERT_NE(v1, v2) (::utility::is_not_equal(v1, v2), (void)0)
#define ASSERT_LE(v1, v2) (::utility::is_less_or_equal(v1, v2), (void)0)
#define ASSERT_LT(v1, v2) (::utility::is_less(v1, v2), (void)0)
#define ASSERT_GE(v1, v2) (::utility::is_greater_or_equal(v1, v2), (void)0)
#define ASSERT_GT(v1, v2) (::utility::is_greater(v1, v2), (void)0)

#define ASSERT(exp) ::utility::is_true(exp)

#endif

#endif

namespace utility
{
    // TIPS:
    // * to capture parameters by reference in macro definitions for single evaluation
    // * to suppress `unused variable` warnings like: `warning C4101: '...': unreferenced local variable`
    template<typename T>
    inline bool is_true(const T & v)
    {
        return !!v; // to avoid warnings of truncation to bool
    }

    template<typename T>
    inline bool is_false(const T & v)
    {
        return !v; // to avoid warnings of truncation to bool
    }

    template<typename T1, typename T2>
    inline bool is_equal(const T1 & v1, const T2 & v2)
    {
        return v1 == v2;
    }

    template<typename T1, typename T2>
    inline bool is_not_equal(const T1 & v1, const T2 & v2)
    {
        return v1 != v2;
    }

    template<typename T1, typename T2>
    inline bool is_less_or_equal(const T1 & v1, const T2 & v2)
    {
        return v1 <= v2;
    }

    template<typename T1, typename T2>
    inline bool is_less(const T1 & v1, const T2 & v2)
    {
        return v1 < v2;
    }

    template<typename T1, typename T2>
    inline bool is_greater_or_equal(const T1 & v1, const T2 & v2)
    {
        return v1 >= v2;
    }

    template<typename T1, typename T2>
    inline bool is_greater(const T1 & v1, const T2 & v2)
    {
        return v1 > v2;
    }
}
