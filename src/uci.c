#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "uci.h"
#include "board.h"

void uciReady(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
}

Command allcommands[] = {
    {"isready", uciReady},
    { 0 }
};

int ProcessCommand(Board* board, char* command)
{
    char command_copy[COMMAND_LIMIT];
    memcpy(command_copy, command, COMMAND_LIMIT);
    char* saveptr;
    char* token = strtok_r(command, "\n", &saveptr);
    token = strtok_r(command, " ", &saveptr);
    Command* c;
    for (c = allcommands; c->match[0] != 0; c++)
    {
        if (!strcmp("exit", token))
            return 0;
        if (!strcmp(c->match, token))
        {
            c->func(board, command_copy);
            break;
        }
    }
    return 1;
}
