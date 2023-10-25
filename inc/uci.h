#ifndef UCI_H
#define UCI_H

#include "board.h"

/* The max size of the command buffer, we can increase if need be */
#define COMMAND_LIMIT 2048

/* Default number of threads for proper uci */
#ifndef NUM_THREADS
#define NUM_THREADS 3
#endif

/* Command struct holds the name of the command, and a function pointer to be
 * called to carry out the command
 */
typedef struct {
    char match[32];
    int (*func)(Board*, char*);
} Command;

typedef struct {
    uint8_t flags;
    Move bestMove;
} UciState;

enum UciStates {
    UCI_STOP = 0x1,
    UCI_DEBUG = 0x2
};

/* Defined in main.c */
extern UciState g_state;

/*
 * @brief a wrapper that formats the output according to UCI and only
 * prints when UCI_DEBUG is set in g_state.flags
 * @param args same arg parameters to printf, will be preceded with
 * "info string " and printed to console. Resulting string from args
 * must be less than 256 bytes
 */
#define printdebug(...) if (g_state.flags & UCI_DEBUG) { \
    write(1, "info string ", 12);\
    char mystr[256];\
    sprintf(mystr, __VA_ARGS__);\
    write(1, mystr, strlen(mystr));\
}

int ProcessCommand(Board* board, char* command);

void uciDebug(char *info);

#endif /* end of include guard: UCI_H */
