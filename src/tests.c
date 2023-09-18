#include <stdio.h>
#include <stdint.h>

#include "board.h"
#include "bitHelpers.h"

/*
 * tests
 * @brief runs a series of tests
 * @return number of tests that failed
int tests()
{
    int pass = 0;
    int fail = 0;

    char *good = "\e[32m";
    char *bad = "\e[31m";
    char *clear = "\e[0m";
    char *nameColor = "\e[33m";

    char *name;
    int int_res;
    uint64_t uint64_t_res;

    /*
     * RUN_TEST
     * @brief macro to quickly create a test.
     * @param testCode literal code to run in the test
     * @param resultVar an already declared variable to store result of testCode
     *   into.
     * @param resultFmt a printf format specifier for resultVar
     * @param expected the expected value, compared to resultVar
     */
#define RUN_TEST( testCode, resultVar, resultFmt, expected ) { \
    name = #testCode; \
    if (( resultVar = (testCode) ) == ( expected )) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s.\n", \
                nameColor, name, clear, good, clear); \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. \n" \
                "  Expected: %s" resultFmt "%s\n" \
                "  Actual:   %s" resultFmt "%s\n" \
                "  Diff:     %s" resultFmt "%s\n", \
                nameColor, name, clear, bad, clear, \
                good, (expected), clear, \
                bad, resultVar, clear, \
                bad, resultVar ^ (expected), clear); \
        fail++; \
    } \
}

    /*
     * RUN_TEST_FMT
     * @brief macro to quickly create a test with custom resultVar formatting
     * @param testCode literal code to run in the test
     * @param resultVar an already declared variable to store result of testCode
     *   into.
     * @param resultFmt a function that formats resultVar and expected.
     * @param expected the expected value, compared to resultVar
     */
#define RUN_TEST_FMT( testCode, resultVar, resultFmt, expected ) { \
    name = #testCode; \
    if (( resultVar = (testCode) ) == (expected) ) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s.\n", \
                nameColor, name, clear, good, clear); \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. \n", \
                nameColor, name, clear, bad, clear); \
        fprintf(stderr, "  Expected: %s\n", good); \
        resultFmt(expected); \
        fprintf(stderr, "%s  Actual:   %s\n", clear, bad); \
        resultFmt(resultVar); \
        fprintf(stderr, "%s\n", clear); \
        fail++; \
    } \
}
    RUN_TEST( bitScanForward(  0xF00  ), int_res, "%d", 8 );
    RUN_TEST( bitScanReverse(  0xF00  ), int_res, "%d", 11 );
    RUN_TEST( shiftWrapLeft( 0xFF00000000000000UL, 8 ), uint64_t_res, "%lx",
              0xFFUL);
    RUN_TEST( shiftWrapRight( 0xFFUL, 8 ), uint64_t_res, "%lx",
              0xFF00000000000000UL);
    RUN_TEST( getNumBits( 0xFFUL ), int_res, "%d", 8 );
    RUN_TEST( getNumBits( 0UL ), int_res, "%d", 0 );
    RUN_TEST_FMT( magicLookupRook( 0x103UL , A1 ), uint64_t_res,
                  printBitboard, 0x102UL);
    RUN_TEST_FMT( magicLookupRook( 0xFFFFUL , A1 ), uint64_t_res,
                  printBitboard, 0x102UL);
    RUN_TEST_FMT( magicLookupRook( 0xFFFFUL << 48, A1 ), uint64_t_res,
                  printBitboard, ((AFILE | RANK[0]) & ~1UL) & ~RANK[7] );
    RUN_TEST_FMT( magicLookupBishop( 0xFFFFFFUL , B2 ), uint64_t_res,
                  printBitboard, 0x50005UL);
    RUN_TEST_FMT( magicLookupBishop( 0xFFFFUL << 48, B2 ), uint64_t_res,
                  printBitboard, 0x0040201008050005 );
    fprintf(stderr, "Tests passed: %s%d%s of %d\n",
            good, pass, clear, pass + fail);
#undef RUN_TEST
#undef RUN_TEST_FMT

    return fail;
}
