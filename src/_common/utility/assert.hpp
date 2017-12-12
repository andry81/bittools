#pragma once

#include "utility/preprocessor.hpp"
#include "utility/platform.hpp"
#include "utility/debug.hpp"

#if defined(UNIT_TESTS) || defined(BENCH_TESTS) && defined(_DEBUG)
#define GTEST_DONT_DEFINE_ASSERT_TRUE 1
#define GTEST_DONT_DEFINE_ASSERT_FALSE 1
#define GTEST_DONT_DEFINE_ASSERT_EQ 1
#define GTEST_DONT_DEFINE_ASSERT_NE 1
#define GTEST_DONT_DEFINE_ASSERT_LE 1
#define GTEST_DONT_DEFINE_ASSERT_LT 1
#define GTEST_DONT_DEFINE_ASSERT_GE 1
#define GTEST_DONT_DEFINE_ASSERT_GT 1

#include <gtest/gtest.h>
#endif

#include <cassert>


#define ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp, precondition) \
    if (!(precondition)); else if(bool(exp)); else DEBUG_BREAK(true)

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
//  * all lambdas w/o capture by reference to avoid bugged implementation in the MSVC 2015 around uninitialized parameters
//    (bug in the `Visual Studio 2015 Update 3` with rarely uninitialized lambda captured variable, mostly because of lambda assign to an auto variable).
//  * all unnecessary lambdas replaced by explicit structure with parentheses operator with void return to avoid slow down around excessive lambdas usage in the debug.
//  * if debugger is attached but `::testing::GTEST_FLAG(break_on_failure)` has not been setted, then an assertion does a post break.
//  * The `auto` has used instead of `decltype()` to avoid MSVC 2015 error: `error C2352: '...': illegal call of non-static member function`

#ifdef USE_ASSERT_WITH_INPLACE_STRUCT_OPERATOR_INSTEAD_LAMBDAS

#define VERIFY_TRUE_IMPL(exp) [](const decltype(exp) & exp_var, const char * exp_str) -> const decltype(exp) & { \
        ::utility::AssertTrue assert_impl; assert_impl(exp_var, exp_str); return exp_var; \
    }

#define VERIFY_TRUE(exp) VERIFY_TRUE_IMPL(exp)(exp, UTILITY_PP_STRINGIZE(exp))
#define ASSERT_TRUE(exp) (void)0; { \
        const decltype(exp) & UTILITY_PP_CONCAT(_exp_var_, UTILITY_PP_LINE) = (exp); \
        ::utility::AssertTrue UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_exp_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(exp)); \
    } (void)0

////

#define VERIFY_FALSE_IMPL(exp) [](const decltype(exp) & exp_var, const char * exp_str) -> const decltype(exp) & { \
        ::utility::AssertFalse assert_impl; assert_impl(exp_var, exp_str); return exp_var; \
    }

#define VERIFY_FALSE(exp) VERIFY_FALSE_IMPL(exp)(exp, UTILITY_PP_STRINGIZE(exp))
#define ASSERT_FALSE(exp) (void)0; { \
        const decltype(exp) & UTILITY_PP_CONCAT(_exp_var_, UTILITY_PP_LINE) = (exp); \
        ::utility::AssertFalse UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_exp_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(exp)); \
    } (void)0

////

#define VERIFY_EQ_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        ::utility::AssertEQ assert_impl; assert_impl(v_1, v_2, v1_str, v2_str); return v_1; \
    }

#define VERIFY_EQ(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_EQ(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE) = (v2); \
        ::utility::AssertEQ UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_NE_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        ::utility::AssertNE assert_impl; assert_impl(v_1, v_2, v1_str, v2_str); return v_1; \
    }

#define VERIFY_NE(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_NE(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE) = (v2); \
        ::utility::AssertNE UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_LE_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        ::utility::AssertLE assert_impl; assert_impl(v_1, v_2, v1_str, v2_str); return v_1; \
    }

#define VERIFY_LE(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_LE(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE) = (v2); \
        ::utility::AssertLE UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_LT_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        ::utility::AssertLT assert_impl; assert_impl(v_1, v_2, v1_str, v2_str); return v_1; \
    }

#define VERIFY_LT(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_LT(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE) = (v2); \
        ::utility::AssertLT UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_GE_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        ::utility::AssertGE assert_impl; assert_impl(v_1, v_2, v1_str, v2_str); return v_1; \
    }

#define VERIFY_GE(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_GE(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE) = (v2); \
        ::utility::AssertGE UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_GT_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        ::utility::AssertGT assert_impl; assert_impl(v_1, v_2, v1_str, v2_str); return v_1; \
    }

#define VERIFY_GT(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_GT(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE) = (v2); \
        ::utility::AssertGT UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE); \
        UTILITY_PP_CONCAT(_assert_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_var_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_var_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

#else

#define VERIFY_TRUE_IMPL(exp) [](const decltype(exp) & exp_var, const char * exp_str) -> const decltype(exp) & { \
        static const auto & fail_break = [](const decltype(exp) & exp_var, const char * exp_str) -> void { \
            const bool is_success = ::utility::is_true(exp_var); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_TEST_BOOLEAN_(is_success, exp_str, false, true, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_TEST_BOOLEAN_(is_success, exp_str, false, true, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(is_success, !break_on_failure); \
        }; fail_break(exp_var, exp_str); \
        return exp_var; \
    }
#define VERIFY_TRUE(exp) VERIFY_TRUE_IMPL(exp)(exp, UTILITY_PP_STRINGIZE(exp))
#define ASSERT_TRUE(exp) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_exp_, UTILITY_PP_LINE) = (exp); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_TRUE_IMPL(UTILITY_PP_CONCAT(_exp_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_exp_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(exp)); \
    } (void)0

#define VERIFY_FALSE_IMPL(exp) [](const decltype(exp) & exp_var, const char * exp_str) -> const decltype(exp) & { \
        static const auto & fail_break = [](const decltype(exp) & exp_var, const char * exp_str) -> void { \
            const bool is_success = ::utility::is_false(exp_var); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_TEST_BOOLEAN_(is_success, exp_str, true, false, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_TEST_BOOLEAN_(is_success, exp_str, true, false, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(is_success, !break_on_failure); \
        }; fail_break(exp_var, exp_str); \
        return exp_var; \
    }
#define VERIFY_FALSE(exp) VERIFY_FALSE_IMPL(exp)(exp, UTILITY_PP_STRINGIZE(exp))
#define ASSERT_FALSE(exp) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_exp_, UTILITY_PP_LINE) = (exp); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_FALSE_IMPL(UTILITY_PP_CONCAT(_exp_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_exp_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(exp)); \
    } (void)0

#define VERIFY_EQ_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        static const auto & fail_break = [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> void { \
            const ::testing::AssertionResult exp_value = ::testing::internal::EqHelper<GTEST_IS_NULL_LITERAL_(v_1)>::Compare(v1_str, v2_str, v_1, v_2); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
        }; fail_break(v_1, v_2, v1_str, v2_str); \
        return v_1; \
    }
#define VERIFY_EQ(v1, v2) VERIFY_EQ_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_EQ(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE) = (v2); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_EQ_IMPL(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_NE_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        static const auto & fail_break = [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> void { \
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperNE(v1_str, v2_str, v_1, v_2); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
        }; fail_break(v_1, v_2, v1_str, v2_str); \
        return v_1; \
    }
#define VERIFY_NE(v1, v2) VERIFY_NE_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_NE(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE) = (v2); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_NE_IMPL(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_LE_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        static const auto & fail_break = [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> void { \
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperLE(v1_str, v2_str, v_1, v_2); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
        }; fail_break(v_1, v_2, v1_str, v2_str); \
        return v_1; \
    }
#define VERIFY_LE(v1, v2) VERIFY_LE_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_LE(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE) = (v2); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_LE_IMPL(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_LT_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        static const auto & fail_break = [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> void { \
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperLT(v1_str, v2_str, v_1, v_2); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
        }; fail_break(v_1, v_2, v1_str, v2_str); \
        return v_1; \
    }
#define VERIFY_LT(v1, v2) VERIFY_LT_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_LT(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE) = (v2); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_LT_IMPL(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_GE_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        static const auto & fail_break = [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> void { \
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperGE(v1_str, v2_str, v_1, v_2); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
        }; fail_break(v_1, v_2, v1_str, v2_str); \
        return v_1; \
    }
#define VERIFY_GE(v1, v2) VERIFY_GE_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_GE(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE) = (v2); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_GE_IMPL(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

////

#define VERIFY_GT_IMPL(v1, v2) [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> const decltype(v1) & { \
        static const auto & fail_break = [](const decltype(v1) & v_1, const decltype(v2) & v_2, const char * v1_str, const char * v2_str) -> void { \
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperGT(v1_str, v2_str, v_1, v_2); \
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure); \
            if (break_on_failure) { \
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_); \
            } else { \
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_); \
            } \
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure); \
        }; fail_break(v_1, v_2, v1_str, v2_str); \
        return v_1; \
    }
#define VERIFY_GT(v1, v2) VERIFY_GT_IMPL(v1, v2)(v1, v2, UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2))
#define ASSERT_GT(v1, v2) (void)0; { \
        const auto & UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE) = (v1); \
        const auto & UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE) = (v2); \
        static const auto & UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE) = VERIFY_GT_IMPL(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE)); \
        UTILITY_PP_CONCAT(verify_impl_, UTILITY_PP_LINE)(UTILITY_PP_CONCAT(_v1_, UTILITY_PP_LINE), UTILITY_PP_CONCAT(_v2_, UTILITY_PP_LINE), UTILITY_PP_STRINGIZE(v1), UTILITY_PP_STRINGIZE(v2)); \
    } (void)0

#endif

#define VERIFY(x) VERIFY_TRUE(x)
#define ASSERT(x) ASSERT_TRUE(x)

#define ASSERT_VERIFY_ENABLED 1

#else

#ifndef ASSERT_IMPL
#define ASSERT_IMPL(exp) assert(exp)
#endif

#ifndef VERIFY_IMPL
#define VERIFY_IMPL(exp) [](const decltype(exp) & exp_) -> const decltype(exp) & { \
        if (!::utility::is_true(exp_)) { \
            _wassert(_CRT_WIDE(#exp), _CRT_WIDE(UTILITY_PP_FILE), (unsigned)(UTILITY_PP_LINE)); \
        } \
        return exp_; \
    }
#endif

#ifdef _DEBUG

#define VERIFY_TRUE(exp) VERIFY_IMPL(exp)(exp)
#define VERIFY_FALSE(exp) VERIFY_IMPL(!(exp))(!(exp))

#define VERIFY_EQ(v1, v2) VERIFY_IMPL((v1) == (v2))((v1) == (v2))
#define VERIFY_NE(v1, v2) VERIFY_IMPL((v1) != (v2))((v1) != (v2))
#define VERIFY_LE(v1, v2) VERIFY_IMPL((v1) <= (v2))((v1) <= (v2))
#define VERIFY_LT(v1, v2) VERIFY_IMPL((v1) < (v2))((v1) < (v2))
#define VERIFY_GE(v1, v2) VERIFY_IMPL((v1) >= (v2))((v1) >= (v2))
#define VERIFY_GT(v1, v2) VERIFY_IMPL((v1) > (v2))((v1) > (v2))

#define ASSERT_TRUE(exp) VERIFY_IMPL(exp)(exp)
#define ASSERT_FALSE(exp) VERIFY_IMPL(!(exp))(!(exp))

#define ASSERT_EQ(v1, v2) ASSERT_IMPL((v1) == (v2))
#define ASSERT_NE(v1, v2) ASSERT_IMPL((v1) != (v2))
#define ASSERT_LE(v1, v2) ASSERT_IMPL((v1) <= (v2))
#define ASSERT_LT(v1, v2) ASSERT_IMPL((v1) < (v2))
#define ASSERT_GE(v1, v2) ASSERT_IMPL((v1) >= (v2))
#define ASSERT_GT(v1, v2) ASSERT_IMPL((v1) > (v2))

#define VERIFY(exp) VERIFY_IMPL(exp)(exp)
#define ASSERT(exp) ASSERT_IMPL(exp)

#define ASSERT_VERIFY_ENABLED 1

#else

#define VERIFY_TRUE(exp) ::utility::is_true(exp)
#define VERIFY_FALSE(exp) ::utility::is_false(exp)

#define VERIFY_EQ(v1, v2) ::utility::is_equal(v1, v2)
#define VERIFY_NE(v1, v2) ::utility::is_not_equal(v1, v2)
#define VERIFY_LE(v1, v2) ::utility::is_less_or_equal(v1, v2)
#define VERIFY_LT(v1, v2) ::utility::is_less(v1, v2)
#define VERIFY_GE(v1, v2) ::utility::is_greater_or_equal(v1, v2)
#define VERIFY_GT(v1, v2) ::utility::is_greater(v1, v2)

#define ASSERT_TRUE(exp) (void)0; { UTILITY_UNUSED(exp); } (void)0
#define ASSERT_FALSE(exp) (void)0; { UTILITY_UNUSED(exp); } (void)0

#define ASSERT_EQ(v1, v2) (void)0; { UTILITY_UNUSED2(v1, v2); } (void)0
#define ASSERT_NE(v1, v2) (void)0; { UTILITY_UNUSED2(v1, v2); } (void)0
#define ASSERT_LE(v1, v2) (void)0; { UTILITY_UNUSED2(v1, v2); } (void)0
#define ASSERT_LT(v1, v2) (void)0; { UTILITY_UNUSED2(v1, v2); } (void)0
#define ASSERT_GE(v1, v2) (void)0; { UTILITY_UNUSED2(v1, v2); } (void)0
#define ASSERT_GT(v1, v2) (void)0; { UTILITY_UNUSED2(v1, v2); } (void)0

#define VERIFY(exp) ::utility::is_true(exp)
#define ASSERT(exp) (void)0; { UTILITY_UNUSED(exp); } (void)0

#define ASSERT_VERIFY_DISABLED 1

#endif

#endif

#ifdef ASSERT_VERIFY_ENABLED
#define IF_ASSERT_VERIFY(exp) exp
#define IF_ASSERT_VERIFY2(e1, e2) e1, e2
#else
#define IF_ASSERT_VERIFY(exp) (void)0; { static const auto & UTILITY_PP_CONCAT(if_assert_verify_impl_, UTILITY_PP_LINE) = [&]() -> void { exp; }; UTILITY_UNUSED(UTILITY_PP_CONCAT(if_assert_verify_impl_, UTILITY_PP_LINE)); } (void)0
#define IF_ASSERT_VERIFY2(e1, e2) (void)0; { static const auto & UTILITY_PP_CONCAT(if_assert_verify_impl_, UTILITY_PP_LINE) = [&]() -> void { e1, e2; }; UTILITY_UNUSED(UTILITY_PP_CONCAT(if_assert_verify_impl_, UTILITY_PP_LINE)); } (void)0
#endif


namespace utility
{
    // TIPS:
    // * to capture parameters by reference in macro definitions for single evaluation
    // * to suppress `unused variable` warnings like: `warning C4101: '...': unreferenced local variable`
    template<typename T>
    FORCE_INLINE bool is_true(const T & v)
    {
        return !!v; // to avoid warnings of truncation to bool
    }

    template<typename T>
    FORCE_INLINE bool is_false(const T & v)
    {
        return !v; // to avoid warnings of truncation to bool
    }

    template<typename T1, typename T2>
    FORCE_INLINE bool is_equal(const T1 & v1, const T2 & v2)
    {
        return v1 == v2;
    }

    template<typename T1, typename T2>
    FORCE_INLINE bool is_not_equal(const T1 & v1, const T2 & v2)
    {
        return v1 != v2;
    }

    template<typename T1, typename T2>
    FORCE_INLINE bool is_less_or_equal(const T1 & v1, const T2 & v2)
    {
        return v1 <= v2;
    }

    template<typename T1, typename T2>
    FORCE_INLINE bool is_less(const T1 & v1, const T2 & v2)
    {
        return v1 < v2;
    }

    template<typename T1, typename T2>
    FORCE_INLINE bool is_greater_or_equal(const T1 & v1, const T2 & v2)
    {
        return v1 >= v2;
    }

    template<typename T1, typename T2>
    FORCE_INLINE bool is_greater(const T1 & v1, const T2 & v2)
    {
        return v1 > v2;
    }

    struct AssertTrue {
        template <typename T>
        FORCE_INLINE void operator()(const T & exp_var, const char * exp_str) const {
            const bool is_success = ::utility::is_true(exp_var);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_TEST_BOOLEAN_(is_success, exp_str, false, true, GTEST_FATAL_FAILURE_);
            } else {
                GTEST_TEST_BOOLEAN_(is_success, exp_str, false, true, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(is_success, !break_on_failure);
        };
    };

    struct AssertFalse {
        template <typename T>
        FORCE_INLINE void operator()(const T & exp_var, const char * exp_str) const {
            const bool is_success = ::utility::is_false(exp_var);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_TEST_BOOLEAN_(is_success, exp_str, false, true, GTEST_FATAL_FAILURE_);
            } else {
                GTEST_TEST_BOOLEAN_(is_success, exp_str, false, true, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(is_success, !break_on_failure);
        };
    };

    struct AssertEQ {
        template <typename T1, typename T2>
        FORCE_INLINE void operator()(const T1 & v1, const T2 & v2, const char * v1_str, const char * v2_str) const {
            const ::testing::AssertionResult exp_value = ::testing::internal::EqHelper<GTEST_IS_NULL_LITERAL_(v1)>::Compare(v1_str, v2_str, v1, v2);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_);
            }
            else {
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure);
        };
    };

    struct AssertNE {
        template <typename T1, typename T2>
        FORCE_INLINE void operator()(const T1 & v1, const T2 & v2, const char * v1_str, const char * v2_str) const {
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperNE(v1_str, v2_str, v1, v2);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_);
            } else {
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure);
        };
    };

    struct AssertLE {
        template <typename T1, typename T2>
        FORCE_INLINE void operator()(const T1 & v1, const T2 & v2, const char * v1_str, const char * v2_str) const {
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperLE(v1_str, v2_str, v1, v2);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_);
            }
            else {
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure);
        };
    };

    struct AssertLT {
        template <typename T1, typename T2>
        FORCE_INLINE void operator()(const T1 & v1, const T2 & v2, const char * v1_str, const char * v2_str) const {
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperLT(v1_str, v2_str, v1, v2);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_);
            }
            else {
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure);
        };
    };

    struct AssertGE {
        template <typename T1, typename T2>
        FORCE_INLINE void operator()(const T1 & v1, const T2 & v2, const char * v1_str, const char * v2_str) const {
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperGE(v1_str, v2_str, v1, v2);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_);
            }
            else {
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure);
        };
    };

    struct AssertGT {
        template <typename T1, typename T2>
        FORCE_INLINE void operator()(const T1 & v1, const T2 & v2, const char * v1_str, const char * v2_str) const {
            const ::testing::AssertionResult exp_value = ::testing::internal::CmpHelperGT(v1_str, v2_str, v1, v2);
            const bool break_on_failure = ::testing::GTEST_FLAG(break_on_failure);
            if (break_on_failure) {
                GTEST_ASSERT_(exp_value, GTEST_FATAL_FAILURE_);
            }
            else {
                GTEST_ASSERT_(exp_value, GTEST_NONFATAL_FAILURE_);
            }
            ASSERT_FAIL_BREAK_ON_ATTACHED_DEBUGGER(exp_value, !break_on_failure);
        };
    };
}
