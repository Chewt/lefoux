#ifndef UCI_H
#define UCI_H

#include "board.h"

/* The max size of the command buffer, we can increase if need be */
#define COMMAND_LIMIT 2048

/* Command struct holds the name of the command, and a function pointer to be
 * called to carry out the command                                            
 */
typedef struct {
    char match[32];
    int (*func)(Board*, char*);
} Command;

int ProcessCommand(Board* board, char* command);

#endif /* end of include guard: UCI_H */
