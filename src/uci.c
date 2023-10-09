#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "uci.h"
#include "board.h"
#include "engine.h"
#include "timer.h"

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
        fprintf(stderr, "WHAT?!? there was an error\n");
    return 1;
}

int uci(Board* board, char* command)
{
    char* s = "id name Lefoux\nid author Hayden Johson, Zach Gorman\nuciok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "WHAT?!? there was an error\n");
    return 1;
}

int position(Board* board, char* command)
{
    char *saveptr;
    /* token should be "position" */
    char *token = strtok_r(command, " \n", &saveptr);
    /* How to read the next parameter */
    token = strtok_r(NULL, " \n", &saveptr);
    if (token && !strcmp(token, "fen"))
    {
        int charsRead = loadFen(board, saveptr);
        if (!charsRead)
        {
            fprintf(stderr, "fen formatted incorrectly, likely not enough "
                            "fields: %s\n", saveptr);
            return 0;
        }
        saveptr += charsRead;
    }
    else if (token && !strcmp(token, "startpos"))
    {
        Board b = getDefaultBoard();
        memcpy(board, &b, sizeof(Board));
    } else if (token) {
        fprintf(stderr, "Unknown postion type: %s\n", token);
        return 1;
    } else {
        fprintf(stderr, "No position type given. Use: position <type> <args>\n");
        return 1;
    }
    /* Process moves after position is set */
    return 1;
}

int go(Board* board, char* command)
{
    char *saveptr;
    char *token = strtok_r(command, " \n", &saveptr);
    token = strtok_r(NULL, " \n", &saveptr);
    if (token && !strcmp(token, "depth"))
    {
        token = strtok_r(NULL, " \n", &saveptr);
        if (token)
        {
            int depth = atoi(token);
            Move m = findBestMove(board, depth);
            char s[16];
            printMove(m);
            snprintf(s, 15, "bestmove %c%c%c%c\n", 
                    mgetsrc(m) % 8 + 'a', 
                    mgetsrc(m) / 8 + '1',
                    mgetdst(m) % 8 + 'a', 
                    mgetdst(m) / 8 + '1');
            if (write(1, s, strlen(s)) == -1)
                fprintf(stderr, "WHAT?!? there was an error\n");
        }
    }
    return 1;
}

int debug(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "WHAT?!? there was an error\n");
    return 1;
}

int setoption(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "WHAT?!? there was an error\n");
    return 1;
}

int ucinewgame(Board* board, char* command)
{
    *board = getDefaultBoard();
    return 1;
}

int stop(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "WHAT?!? there was an error\n");
    return 1;
}

int ponderhit(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "WHAT?!? there was an error\n");
    return 1;
}

/* Non-uci commands */

int printboard(Board* board, char* command)
{
    printBoard(board);
    return 1;
}

int perft(Board* board, char* command)
{
    char *saveptr;
    /* token should be "perft" */
    char *token = strtok_r(command, " \n", &saveptr);
    /* Get depth */
    if (!(token = strtok_r(NULL, " \n", &saveptr))) {
        fprintf(stderr, "No depth given. Use: perft <depth>\n");
        return 1;
    }

    int depth = atoi(token);
    if (!depth) {
        fprintf(stderr, "Either depth is 0 or depth isn't a number: %s\n",
                token);
        return 1;
    } else if (depth < 0) {
        fprintf(stderr, "Depth is less than 0: %d\n", depth);
        return 1;
    }

    Timer t;
    PerftInfo p = {0};
    StartTimer(&t);
    perftRun(board, &p, depth);
    StopTimer(&t);
    printf("Took %.6f seconds\n", t.time_taken);
    printPerft(p);
    return 1;
}

int fen(Board* board, char* command)
{
    printFen(board);
    return 1;
}

/* This struct holds all of the commands that are accepted by lefoux. The format
 * for this struct is a keyword to match, that is the first word in the command,
 * and a function pointer to be called if the command matches the keyword. The
 * last entry is filled with zeros to mark the end of the array.
 */
Command allcommands[] = {
    // Uci commands
    {"isready", isready},
    {"uci", uci},
    {"position", position},
    {"go", go},
    {"debug", debug},
    {"setoption", setoption},
    {"ucinewgame", ucinewgame},
    {"stop", stop},
    {"ponderhit", ponderhit},
    // Non-uci commands
    {"printboard", printboard},
    {"perft", perft},
    {"fen", fen},
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
    fprintf(stderr, "Unknown command: %s\n", token);
    return 1;
}
