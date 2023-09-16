#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <argp.h>

#include "bitHelpers.h"
#define NUM_THREADS 1

void tests();

/* argp struct */
struct flags {

};

const char *argp_program_bug_address = "https://github.com/Chewt/lefoux/issues";
#ifdef VERSION
const char *argp_program_version = VERSION;
#endif

/* Parse Arguments */
static int parse_opt (int key, char *arg, struct argp_state *state)
{
    struct flags* flags = state->input;
    switch(key)
    {
        case 500:
            tests();
            break;
    }
    return 0;
}

int main(int argc, char** argv)
{
    /* Command line args */
    struct flags flags;
    struct argp_option options[] = {
        {"test", 500, 0, 0, "Run unit tests"},
        { 0 }
    };
    struct argp argp = {options, parse_opt, 0, "Multithreaded chess engine.",
                        0,       0,         0};
    if (argp_parse(&argp, argc, argv, 0, 0, &flags))
    {
        fprintf(stderr, "Error parsing arguments\n");
        exit(-1);
    }

#ifdef _OPENMP
	fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
#else
    fprintf( stderr, "No OpenMP support!\n" );
    return 1;
#endif

    omp_set_num_threads(NUM_THREADS);

#ifdef DEBUG
    tests();
#endif

    return 0;
}

void tests()
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

    fprintf(stderr, "Tests passed: %s%d%s of %d\n",
            good, pass, clear, pass + fail);
}
