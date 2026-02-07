/*
 * Minimal unit test framework for HIME
 * No external dependencies required
 */

#ifndef HIME_TEST_FRAMEWORK_H
#define HIME_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test result counters */
static int _test_passed = 0;
static int _test_failed = 0;
static int _test_total = 0;
static const char *_current_test = NULL;

/* Colors for terminal output */
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

/* Test macros */
#define TEST(name)                       \
    static void test_##name (void);      \
    static void run_test_##name (void) { \
        _current_test = #name;           \
        _test_total++;                   \
        test_##name ();                  \
    }                                    \
    static void test_##name (void)

#define RUN_TEST(name)      \
    do {                    \
        run_test_##name (); \
    } while (0)

/* Assertion macros */
#define ASSERT_TRUE(expr)                                                           \
    do {                                                                            \
        if (!(expr)) {                                                              \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_TRUE(%s) failed\n",              \
                     __FILE__, __LINE__, #expr);                                    \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_FALSE(expr)                                                          \
    do {                                                                            \
        if (expr) {                                                                 \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_FALSE(%s) failed\n",             \
                     __FILE__, __LINE__, #expr);                                    \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_EQ(expected, actual)                                                 \
    do {                                                                            \
        if ((expected) != (actual)) {                                               \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_EQ(%s, %s) failed\n",            \
                     __FILE__, __LINE__, #expected, #actual);                       \
            fprintf (stderr, "       expected: %ld, actual: %ld\n",                 \
                     (long) (expected), (long) (actual));                           \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_NE(expected, actual)                                                 \
    do {                                                                            \
        if ((expected) == (actual)) {                                               \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_NE(%s, %s) failed\n",            \
                     __FILE__, __LINE__, #expected, #actual);                       \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_STR_EQ(expected, actual)                                             \
    do {                                                                            \
        const char *_exp = (expected);                                              \
        const char *_act = (actual);                                                \
        if (_exp == NULL && _act == NULL)                                           \
            break;                                                                  \
        if (_exp == NULL || _act == NULL || strcmp (_exp, _act) != 0) {             \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_STR_EQ(%s, %s) failed\n",        \
                     __FILE__, __LINE__, #expected, #actual);                       \
            fprintf (stderr, "       expected: \"%s\"\n", _exp ? _exp : "(null)");  \
            fprintf (stderr, "       actual:   \"%s\"\n", _act ? _act : "(null)");  \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                        \
    do {                                                                            \
        if ((ptr) == NULL) {                                                        \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_NOT_NULL(%s) failed\n",          \
                     __FILE__, __LINE__, #ptr);                                     \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_NULL(ptr)                                                            \
    do {                                                                            \
        if ((ptr) != NULL) {                                                        \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_NULL(%s) failed\n",              \
                     __FILE__, __LINE__, #ptr);                                     \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define ASSERT_MEM_EQ(expected, actual, size)                                       \
    do {                                                                            \
        if (memcmp ((expected), (actual), (size)) != 0) {                           \
            fprintf (stderr, COLOR_RED "FAIL" COLOR_RESET ": %s\n", _current_test); \
            fprintf (stderr, "       %s:%d: ASSERT_MEM_EQ(%s, %s, %s) failed\n",    \
                     __FILE__, __LINE__, #expected, #actual, #size);                \
            _test_failed++;                                                         \
            return;                                                                 \
        }                                                                           \
    } while (0)

/* Mark test as passed (called implicitly if no assertion fails) */
#define TEST_PASS()                                                               \
    do {                                                                          \
        fprintf (stderr, COLOR_GREEN "PASS" COLOR_RESET ": %s\n", _current_test); \
        _test_passed++;                                                           \
    } while (0)

/* Test suite management */
#define TEST_SUITE_BEGIN(name)         \
    int main (int argc, char **argv) { \
        (void) argc;                   \
        (void) argv;                   \
        fprintf (stderr, "\n" COLOR_YELLOW "=== %s ===" COLOR_RESET "\n\n", name);

#define TEST_SUITE_END()                                                            \
    fprintf (stderr, "\n" COLOR_YELLOW "=== Results ===" COLOR_RESET "\n");         \
    fprintf (stderr, "Total:  %d\n", _test_total);                                  \
    fprintf (stderr, "Passed: " COLOR_GREEN "%d" COLOR_RESET "\n", _test_passed);   \
    if (_test_failed > 0)                                                           \
        fprintf (stderr, "Failed: " COLOR_RED "%d" COLOR_RESET "\n", _test_failed); \
    else                                                                            \
        fprintf (stderr, "Failed: %d\n", _test_failed);                             \
    fprintf (stderr, "\n");                                                         \
    return _test_failed > 0 ? 1 : 0;                                                \
    }

#endif /* HIME_TEST_FRAMEWORK_H */
