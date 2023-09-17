#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <argp.h>

#include "bitHelpers.h"
#include "board.h"
#include "tests.h"
#include "magic.h"

#define NUM_THREADS 1

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
    int result;
    switch(key)
    {
        case 500:
            // Program should return non-zero if a test fails
            result = tests();
            if (result) exit(result);
            break;
        case 501:
            exit(computeMagic());
    }
    return 0;
}

int main(int argc, char** argv)
{
#ifdef _OPENMP
	fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
#else
    fprintf( stderr, "No OpenMP support!\n" );
    return 1;
#endif

    omp_set_num_threads(NUM_THREADS);

    /* Command line args */
    struct flags flags;
    struct argp_option options[] = {
        {"test", 500, 0, 0, "Run unit tests"},
        {"magic", 501, 0, 0, "Compute magic numbers for Rooks and Bishops"},
        { 0 }
    };
    struct argp argp = {options, parse_opt, 0, "Multithreaded chess engine.",
                        0,       0,         0};
    if (argp_parse(&argp, argc, argv, 0, 0, &flags))
    {
        fprintf(stderr, "Error parsing arguments\n");
        exit(-1);
    }

    return 0;
}

