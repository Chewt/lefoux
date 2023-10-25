#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "uci.h"
#include "board.h"
#include "engine.h"
#include "timer.h"
#include "bitHelpers.h"

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
        fprintf(stderr, "Error writing to stdout");
    return 1;
}

int uci(Board* board, char* command)
{
    char *s = {
        "id name Lefoux " LEFOUX_VERSION "\n"
        "id author Hayden Johnson and Zachary Gorman\n"
        "uciok\n"
    };
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "Error writing to stdout");
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
        fprintf(stderr, "Unknown postion type, must be of [fen | startpos]: "
                        "%s\n", token);
        return 1;
    } else {
        fprintf(stderr, "No position type given. Use: position [fen | startpos]"
                        " moves ....\n");
        return 1;
    }
    /* Process moves after position is set */
    /* Token should be "moves" */
    token = strtok_r(NULL, " \n", &saveptr);
    /* Now each token should be a LAN move */
    while ( (token = strtok_r(NULL, " \n", &saveptr)) )
    {
        Move m = parseLANMove(board, token);
        if (!m) continue;
        m = boardMove(board, m);
    }
    return 1;
}

int go(Board* board, char* command)
{
    char *saveptr;
    /* token should be "go" */
    char *token = strtok_r(command, " \n", &saveptr);
    /* parameters to tweak with subcommands */
    Move moves[MAX_MOVES_PER_POSITION];
    int numMoves = 0;
    int depth = 5;
    int ponder = 0;
    int maxTime = 0;
    /* go subcommand */
    while ( (token = strtok_r(NULL, " \n", &saveptr)) )
    {
        if (token && !strcmp(token, "searchmoves"))
        {
            while ( (token = strtok_r(NULL, " \n", &saveptr)) )
            {
                moves[numMoves] = parseLANMove(board, token);
                if (!moves[numMoves]) break;
                numMoves++;
            }
        }
        if (token && !strcmp(token, "ponder"))
        {
            ponder = 1;
        }
        if (token && !strcmp(token, "wtime"))
        {
            /* token is number of ms left on the clock for white */
            token = strtok_r(NULL, " \n", &saveptr);
        }
        if (token && !strcmp(token, "btime"))
        {
            /* token is number of ms left on the clock for black */
            token = strtok_r(NULL, " \n", &saveptr);
        }
        if (token && !strcmp(token, "winc"))
        {
            /* token is white's increment per move in ms if x > 0 */
            token = strtok_r(NULL, " \n", &saveptr);
        }
        if (token && !strcmp(token, "binc"))
        {
            /* token is black's increment per move in ms if x > 0 */
            token = strtok_r(NULL, " \n", &saveptr);
        }
        if (token && !strcmp(token, "movestogo"))
        {
            /* token is the number of moves until next time control */
            token = strtok_r(NULL, " \n", &saveptr);
        }
        if (token && !strcmp(token, "depth"))
        {
            /* token is the number of plies to search */
            token = strtok_r(NULL, " \n", &saveptr);
            depth = atoi(token);
        }
        if (token && !strcmp(token, "nodes"))
        {
            /* token is the number of nodes to search */
            token = strtok_r(NULL, " \n", &saveptr);
            depth = atoi(token);
            // An estimate conversion from number of nodes to ply
            depth = bitScanReverse(depth);
        }
        if (token && !strcmp(token, "mate"))
        {
            /* token is the number of moves to find a mate */
            token = strtok_r(NULL, " \n", &saveptr);
            depth = atoi(token);
        }
        if (token && !strcmp(token, "movetime"))
        {
            /* token is the amount of time to search in ms */
            token = strtok_r(NULL, " \n", &saveptr);
            maxTime = atoi(token);
        }
        if (token && !strcmp(token, "infinite"))
        {
            /* Search until the "stop" command */
            depth = 0xff;
        }
    }
    g_state.flags &= ~UCI_STOP;
    // Task to start searching
    // Don't spawn task with only 1 thread since it won't get picked up
    // and will hang
    #if NUM_THREADS > 1
    #pragma omp task untied
    #endif
    {
        // This section is basically the guts of findBestMove
        g_state.bestMove = msetweight(0, -127);
        if (!numMoves) g_state.bestMove = findBestMove(board, depth);
        int i;
        for (i=0; i<numMoves; i++)
        {
            Move undoM = boardMove(board, moves[i]);
            moves[i] = msetweight(moves[i],
                    mgetweight(findBestMove(board, depth)));
            undoMove(board, undoM);
            if (g_state.flags & UCI_STOP) break;
            // Update global state in case this is interrupted
            if (mgetweight(moves[i]) > mgetweight(g_state.bestMove))
                g_state.bestMove = moves[i];
        }
        g_state.flags |= UCI_STOP;
        qsort(moves, i, sizeof(Move), compareMoveWeights);
        if (numMoves) g_state.bestMove = moves[0];
        for (i = 0; i < numMoves; ++i)
        {
            if (mgetweight(moves[i]) != mgetweight(g_state.bestMove))
                break;
        }
        // If there are many bestMoves, pick one randomly
        if (i - 1 > 0)
            g_state.bestMove = moves[rand() % (i - 1)];
        /* Deliver bestMove */
        if (!ponder) {
            char s[] = {"bestmove a1h8q\n"};
            sprintLANMove(s + 9, g_state.bestMove);
            int n = strlen(s);
            s[n] = '\n';
            if (write(1, s, strlen(s)) == -1)
                fprintf(stderr, "Error writing to stdout");
        }
    }
    // Task to time the search
    // Don't spawn task with fewer than 3 threads, otherwise timer starts
    // after search is finished
    #if NUM_THREADS >= 3
    if (maxTime)
    {
        #pragma omp task untied
        {
            usleep(maxTime * 1000);
            g_state.flags |= UCI_STOP;
        }
    }
    #endif

    return 1;
}

int debug(Board* board, char* command)
{
    g_state.flags |= UCI_DEBUG;
    return 1;
}

int setoption(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "Error writing to stdout");
    return 1;
}

int ucinewgame(Board* board, char* command)
{
    *board = getDefaultBoard();
    return 1;
}

int stop(Board* board, char* command)
{
    g_state.flags |= UCI_STOP;
    char s[] = {"bestmove a1h8q\n"};
    sprintLANMove(s + 9, g_state.bestMove);
    int n = strlen(s);
    s[n] = '\n';
    // printMove(g_state.bestMove);
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "Error writing to stdout");
    return 1;
}

int ponderhit(Board* board, char* command)
{
    char* s = "readyok\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "Error writing to stdout");
    return 1;
}

int uciquit(Board *board, char *command) { return 0; }

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
    perftRunThreaded(board, &p, depth);
    StopTimer(&t);
    #ifdef CSV
    printf("%.6f,", t.time_taken);
    #else
    printf("Took %.6f seconds\n", t.time_taken);
    #endif
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
    {"register", isready},
    {"ucinewgame", isready},
    {"quit", uciquit},
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
        if (!strcmp(c->match, token))
            return c->func(board, command_copy);
    }
    fprintf(stderr, "Unknown command: %s\n", token);
    return 1;
}

void uciInfo(char *info)
{
    char* s = "info string ";
    char* n = "\n";
    if (write(1, s, strlen(s)) == -1)
        fprintf(stderr, "Error writing to stdout");
    if (write(1, info, strlen(info)) == -1)
        fprintf(stderr, "Error writing to stdout");
    if (write(1, n, strlen(n)) == -1)
        fprintf(stderr, "Error writing to stdout");
}
