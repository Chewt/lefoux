#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <argp.h>
#include <string.h>
#include <time.h>

#include "bitHelpers.h"
#include "board.h"
#include "tests.h"
#include "magic.h"
#include "uci.h"

/* Global variable across all files that include uci.h */
UciState g_state = { 0 };

/* argp struct */
struct flags {
    char fen[64];
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
        case 'f':
            memcpy(flags->fen, arg, strlen(arg));
            break;
        case 500:
            #pragma omp parallel
            #pragma omp single
            result = tests();
            // Program should return non-zero if a test fails
            if (result) exit(result);
            // This line always exit after a test, comment to exit only after
            // a failure
            exit(result);
            break;
        case 501:
            exit(computeMagic());
    }
    return 0;
}

int main(int argc, char** argv)
{
   /* Lefoux cli preamble */
#ifdef _OPENMP
    fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
    fprintf( stderr, "NUM_THREADS = %d\n", NUM_THREADS);
    fprintf( stderr, "Lefoux " LEFOUX_VERSION "\n");
#else
    fprintf( stderr, "No OpenMP support!\n" );
    return 1;
#endif

    srand(time(NULL));
    omp_set_num_threads(NUM_THREADS);

    /* Command line args */
    struct flags flags;
    flags.fen[0] = 0;
    struct argp_option options[] = {
        {"fen", 'f', "STRING", 0, "start board with position", 0},
        {"test", 500, 0, 0, "Run unit tests", 0},
        {"magic", 501, 0, 0, "Compute magic numbers for Rooks and Bishops", 0},
        { 0 }
    };
    struct argp argp = {options, parse_opt, 0, "Multithreaded chess engine.",
        0,       0,         0};
    if (argp_parse(&argp, argc, argv, 0, 0, &flags))
    {
        fprintf(stderr, "Error parsing arguments\n");
        exit(-1);
    }

    FILE* input = fdopen(0, "r");

    Board board = getDefaultBoard();
    g_state.flags = 0;

    int running = 1;
    // This directive enables parallelism
    #pragma omp parallel
    // This directive says this while loop is handled by only one thread
    #pragma omp single
    while (running)
    {
        char* command = malloc(COMMAND_LIMIT);
        size_t size = COMMAND_LIMIT;
        if (getline(&command, &size, input) == -1)
        {
            fprintf(stderr, "Error reading from command line\n");
            exit(-1);
        }
        running = ProcessCommand(&board, command);
        free(command);
    }
    fclose(input);

    return 0;
}

