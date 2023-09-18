#ifndef UCI_H
#define UCI_H

#include "board.h"

#define COMMAND_LIMIT 2048

typedef struct {
    char match[32];
    void (*func)(Board*, char*);
} Command;

int ProcessCommand(Board* board, char* command);

#endif /* end of include guard: UCI_H */
