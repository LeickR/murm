// MIT License
//
// Copyright (c) 2026 Leick Robinson (Virtual Science Workshop)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// Tiny header-only test framework for the murm test suite.
//
// Usage:
//
//     #include "test_framework.hpp"
//
//     TEST(my_test_name) {
//         ASSERT_TRUE(2 + 2 == 4);
//         ASSERT_EQ(42, my_function());
//     }
//
//     RUN_ALL_TESTS()    // expands to int main() that runs every TEST() above
//
// Each test binary should contain exactly one RUN_ALL_TESTS().  The runner
// returns 0 if every test passed and 1 otherwise, and prints a per-test
// PASS/FAIL line plus a final summary.

#ifndef MURM_TEST_FRAMEWORK_HPP
#define MURM_TEST_FRAMEWORK_HPP

#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace murm_test {

struct TestCase {
    const char *name;
    void (*fn)();
};

// All registered tests live in a single translation-unit-local vector.
inline std::vector<TestCase> &registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char *name, void (*fn)()) {
        registry().push_back({name, fn});
    }
};

// Thrown by the assertion macros to abort the current test without
// terminating the whole binary.  The runner catches this and counts the
// test as a failure.
struct AssertionFailure {
    std::string where;
    std::string what;
};

inline std::ostream &fail_stream() { return std::cerr; }

inline int run_all(const char *suite_name) {
    auto &tests = registry();
    int passed = 0;
    int failed = 0;
    std::cout << "[ suite ] " << suite_name << " (" << tests.size() << " tests)\n";
    for (const auto &t : tests) {
        try {
            t.fn();
            std::cout << "  [ PASS ] " << t.name << "\n";
            ++passed;
        } catch (const AssertionFailure &af) {
            std::cout << "  [ FAIL ] " << t.name << "\n";
            std::cout << "    at " << af.where << ": " << af.what << "\n";
            ++failed;
        } catch (const std::exception &e) {
            std::cout << "  [ FAIL ] " << t.name << " (uncaught exception: "
                      << e.what() << ")\n";
            ++failed;
        } catch (...) {
            std::cout << "  [ FAIL ] " << t.name << " (uncaught non-std exception)\n";
            ++failed;
        }
    }
    std::cout << "[ done  ] " << suite_name
              << ": " << passed << " passed, " << failed << " failed\n";
    return (failed == 0) ? 0 : 1;
}

} // namespace murm_test


#define MURM_TEST_CONCAT_INNER(a, b) a##b
#define MURM_TEST_CONCAT(a, b)       MURM_TEST_CONCAT_INNER(a, b)

#define TEST(test_name)                                                         \
    static void MURM_TEST_CONCAT(test_fn_, test_name)();                        \
    static ::murm_test::Registrar MURM_TEST_CONCAT(test_reg_, test_name) {       \
        #test_name, &MURM_TEST_CONCAT(test_fn_, test_name)                      \
    };                                                                          \
    static void MURM_TEST_CONCAT(test_fn_, test_name)()

#define RUN_ALL_TESTS()                                                         \
    int main(int /*argc*/, char ** /*argv*/) {                                  \
        return ::murm_test::run_all(__FILE__);                                  \
    }


// Throw an AssertionFailure with location info baked in.
#define MURM_TEST_FAIL(msg_stream)                                              \
    do {                                                                        \
        std::ostringstream _murm_test_oss;                                      \
        _murm_test_oss << msg_stream;                                           \
        ::murm_test::AssertionFailure _af;                                      \
        _af.where = std::string(__FILE__) + ":" + std::to_string(__LINE__);     \
        _af.what  = _murm_test_oss.str();                                       \
        throw _af;                                                              \
    } while (0)

#define ASSERT_TRUE(cond)                                                       \
    do {                                                                        \
        if (!(cond)) MURM_TEST_FAIL("ASSERT_TRUE failed: " #cond);              \
    } while (0)

#define ASSERT_FALSE(cond)                                                      \
    do {                                                                        \
        if ((cond)) MURM_TEST_FAIL("ASSERT_FALSE failed: " #cond);              \
    } while (0)

#define ASSERT_EQ(expected, actual)                                             \
    do {                                                                        \
        auto _e = (expected);                                                   \
        auto _a = (actual);                                                     \
        if (!(_e == _a))                                                        \
            MURM_TEST_FAIL("ASSERT_EQ failed: " #expected " == " #actual        \
                           "; expected=" << _e << ", actual=" << _a);           \
    } while (0)

#define ASSERT_NE(lhs, rhs)                                                     \
    do {                                                                        \
        auto _l = (lhs);                                                        \
        auto _r = (rhs);                                                        \
        if (!(_l != _r))                                                        \
            MURM_TEST_FAIL("ASSERT_NE failed: " #lhs " != " #rhs                \
                           "; lhs=" << _l << ", rhs=" << _r);                   \
    } while (0)

#define ASSERT_NEAR(expected, actual, tolerance)                                \
    do {                                                                        \
        double _e = (expected);                                                 \
        double _a = (actual);                                                   \
        double _t = (tolerance);                                                \
        if (std::fabs(_e - _a) > _t)                                            \
            MURM_TEST_FAIL("ASSERT_NEAR failed: " #expected " ≈ " #actual       \
                           " (tol=" << _t << "); expected=" << _e               \
                           << ", actual=" << _a);                               \
    } while (0)

#define ASSERT_STREQ(expected, actual)                                          \
    do {                                                                        \
        std::string _e = (expected);                                            \
        std::string _a = (actual);                                              \
        if (_e != _a)                                                           \
            MURM_TEST_FAIL("ASSERT_STREQ failed: " #expected " == " #actual     \
                           "; expected=\"" << _e << "\", actual=\""             \
                           << _a << "\"");                                      \
    } while (0)

#endif // MURM_TEST_FRAMEWORK_HPP
