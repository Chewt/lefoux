#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "uci.h"
#include "board.h"

/*******************************************************************************
 *
 * Command functions
 *
 * These functions must all follow the same pattern and function
 * signature. They must return an int, and take two arguments: a Board pointer
 * and a char pointer. The Board pointer is the current boardstate, and the
 * char pointer is the entire command that is being processed. The name of the
 * function corresponds to the command name.
 *
 ******************************************************************************/

int isready(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int uci(Board* board, char* command)
{
    char* s = "id name lefoux\nid author Hayden Johnson, Zach Gorman\nuciok\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int position(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int go(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int debug(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int setoption(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int ucinewgame(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int stop(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

int ponderhit(Board* board, char* command)
{
    char* s = "command not yet implemented\n";
    if (write(1, s, strlen(s)) == -1)
        printf("WHAT?!? there was an error\n");
    return 1;
}

/* This struct holds all of the commands that are accepted by lefoux. The format
 * for this struct is a keyword to match, that is the first word in the command,
 * and a function pointer to be called if the command matches the keyword. The
 * last entry is filled with zeros to mark the end of the array.
 */
Command allcommands[] = {
    {"isready", isready},
    {"uci", uci},
    {"position", position},
    {"go", go},
    {"debug", debug},
    {"setoption", setoption},
    {"ucinewgame", ucinewgame},
    {"stop", stop},
    {"ponderhit", ponderhit},
    {{0},0} // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119
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
        if (!strcmp("quit", token))
            return 0;
        if (!strcmp(c->match, token))
            return c->func(board, command_copy);
    }
    return 1;
}