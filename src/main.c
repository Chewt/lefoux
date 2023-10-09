#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <argp.h>
#include <string.h>

#include "bitHelpers.h"
#include "board.h"
#include "tests.h"
#include "magic.h"
#include "uci.h"

#ifndef NUM_THREADS
#define NUM_THREADS 1
#endif

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
    fprintf( stderr, "NUM_THREADS = %d\n", NUM_THREADS);
#else
    fprintf( stderr, "No OpenMP support!\n" );
    return 1;
#endif

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

    // TESTING
    printf("%s\n", flags.fen);

    FILE* input = fdopen(0, "r");

    Board board = getDefaultBoard();
    int running = 1;
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

