#include <stdio.h>
#include <stdint.h>

#include "board.h"
#include "bitHelpers.h"

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
    int8_t int8_t_res;

#define RUN_TEST( testCode, resultVar, resultFmt, expected ) { \
    name = #testCode; \
    if (( resultVar = testCode ) == expected) { \
        fprintf(stderr, "Test %s%s%s %spassed%s.\n", \
                nameColor, name, clear, good, clear); \
        pass++; \
    } else { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. \n" \
                "  Expected: %s" resultFmt "%s\n" \
                "  Actual:   %s" resultFmt "%s\n", \
                nameColor, name, clear, bad, clear, \
                good, expected, clear, \
                bad, resultVar, clear); \
        fail++; \
    } \
}
    RUN_TEST( bitScanForward(  0xF00  ), int_res, "%d", 8 );
    RUN_TEST( bitScanReverse(  0xF00  ), int_res, "%d", 11 );
    RUN_TEST( shiftWrapLeft( 0xFF00000000000000UL, 8 ), uint64_t_res, "%lx", 0xFFUL);
    RUN_TEST( shiftWrapRight( 0xFFUL, 8 ), uint64_t_res, "%lx", 0xFF00000000000000UL);
    RUN_TEST( getNumBits( 0xFFULL ), int_res, "%d", 8 );
    RUN_TEST( getNumBits( 0ULL ), int_res, "%d", 0 );

    fprintf(stderr, "Tests passed: %s%d%s of %d\n",
            good, pass, clear, pass + fail);

    return fail;
}
