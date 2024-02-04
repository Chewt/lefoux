#include <stdio.h>
#include <limits.h>     // INT_MIN
#include <stdlib.h>     // rand()
#include <stdint.h>     // Fancy integer types
#include <string.h>     // memcpy()

#include <omp.h>

#include "engine.h"
#include "board.h"
#include "bitHelpers.h"
#include "uci.h"

/*
 * Returns the net weight of pieces on the board. A positive number
 * indicates an advantage for white, negative for black, 0 for even.
 * A variation of this returns a positive number if the player to
 * move has a better weight.
 */
int8_t netWeightOfPieces(Board* board)
{
    int8_t weight = 0;
    // Weights for PAWN, KNIGHT, BISHOP, ROOK, QUEEN, and KING
    int8_t weights[] = {1, 3, 3, 5, 8, 120};
    // This line returns positive weight for white
    // int to_move = WHITE;
    // This line returns a positive weight for the player moving
    int to_move = bgetcol(board->info) ? BLACK : WHITE;
    for (int i=PAWN; i<=KING; i++)
    {
        weight += ( getNumBits(board->pieces[i + to_move])
                -   getNumBits(board->pieces[i + (to_move ^ BLACK)])
                ) * weights[i];
    }
    return weight;
}

int8_t evaluateBoard(Board* board)
{
    return netWeightOfPieces(board);
}

int alphaBeta( Board* board, int8_t alpha, int8_t beta, int8_t depthleft ) {
    if ( depthleft == 0 ) return evaluateBoard(board);
    Move moves[MAX_MOVES_PER_POSITION];
    uint8_t numMoves = genAllLegalMoves(board, moves);
    int i;
    for (i = 0; i < numMoves; ++i) {
        Move undo = boardMove(board, moves[i]);
        int8_t weight = -alphaBeta(board, -beta, -alpha, depthleft - 1 );
        undoMove(board, undo);
        if( weight >= beta )
            return beta;   // fail hard beta-cutoff
        if( weight > alpha )
            alpha = weight; // alpha acts like max in MiniMax
    }
    return alpha;
}

int compareMoveWeights(const void* one, const void* two)
{
    if (mgetweight((*(Move*)one)) > mgetweight((*(Move*)two)))
        return -1;
    else if (mgetweight((*(Move*)one)) < mgetweight((*(Move*)two)))
        return 1;
    return 0;
}

Move findBestMove(Board* board, uint8_t depth)
{
    // Assumes MAX_MOVES_PER_POSITION < 256
    uint8_t i;
    uint8_t numMoves;

    // Setup independent variables for each thread
    Board boards[NUM_THREADS];
    for (i=0; i<NUM_THREADS; i++) memcpy(&boards[i], board, sizeof(Board));
    // MAX_MOVES_PER_POSITION*sizeof(Move) = 218 * 4 = 872 bytes
    Move moves[MAX_MOVES_PER_POSITION];
    numMoves = genAllLegalMoves(board, moves);
    int8_t alpha = -126;
    int8_t beta = 127;
    for (int curdepth=1; curdepth <= depth; curdepth++) {
        // Generate tasks for this loop to be parallelized
        #pragma omp taskloop untied default(shared)
        for (i=0; i<numMoves; i++)
        {
            int me = omp_get_thread_num();
            Move undoM = boardMove(&boards[me], moves[i]);
            // Update the move with its weight
            int8_t weight = -alphaBeta(&boards[me], -beta, -(alpha - 1), curdepth);
            moves[i] = msetweight(moves[i], weight);
            undoMove(&boards[me], undoM);
            // Update alpha if a better weight was found. Critical to avoid race
            // conditions
            #pragma omp critical
            if (weight > alpha) {
                alpha = weight;
                // Update global statet best move in case the search is interrupted
                g_state.bestMove = moves[i];
            }
            // If UCI_STOP, cancel remaining tasks
            if (g_state.flags & UCI_STOP)
            {
                #pragma omp cancel taskgroup
                // Similar to break;
            }
        }
        // Sort the moves so we can find the best one!
        qsort(moves, numMoves, sizeof(Move), compareMoveWeights);
    }
    Move bestMove = 0;
    if (numMoves) bestMove = moves[0];
    for (i = 0; i < numMoves; ++i)
    {
        if (mgetweight(moves[i]) != mgetweight(bestMove))
            break;
    }
    int j;
    for (j = 0; j < 0; ++j)
    {
        fprintf(stderr, "%c%c%c%c weight: %d\n",
                mgetsrc(moves[j]) % 8 + 'a',
                mgetsrc(moves[j]) / 8 + '1',
                mgetdst(moves[j]) % 8 + 'a',
                mgetdst(moves[j]) / 8 + '1',
                mgetweight(moves[j]));
    }
    // If there are many bestMoves, pick one randomly
    if (i - 1 > 0)
        bestMove = moves[rand() % (i - 1)];
    return bestMove;
}

void perftRunThreaded(Board* board, PerftInfo* pi, uint8_t depth)
{
    // Below code is very similar to perftRun
    if (depth < 1)
    {
        pi->nodes = 1;
        return;
    }

    // For depth == 1, it might not be worth threading
    if (depth == 1)
    {
        perftRun(board, pi, depth);
        return;
    }

    Move movelist[MAX_MOVES_PER_POSITION];
    int n_moves = genAllLegalMoves(board, movelist);
    int i;

    PerftInfo pilist[NUM_THREADS] = {0};
    Board boards[NUM_THREADS];
    for (i = 0; i < NUM_THREADS; i++)
        memcpy(boards+i, board, sizeof(Board));

#pragma omp taskloop untied default(shared)
    for (i = 0; i < n_moves; ++i)
    {
        int me = omp_get_thread_num();
        movelist[i] = boardMove(&boards[me], movelist[i]);
        perftRun(&boards[me], &pilist[me], depth - 1);
        undoMove(&boards[me], movelist[i]);
    }

    // Combine pi results
    for (i=0; i<NUM_THREADS; i++)
    {
        pi->nodes += pilist[i].nodes;
        pi->captures += pilist[i].captures;
        pi->enpassants += pilist[i].enpassants;
        pi->castles += pilist[i].castles;
        pi->promotions += pilist[i].promotions;
        pi->checks += pilist[i].checks;
        pi->checkmates += pilist[i].checkmates;
    }
}

void perftRun(Board* board, PerftInfo* pi, uint8_t depth)
{
    if (depth < 1)
    {
        pi->nodes = 1;
        return;
    }

    Move movelist[MAX_MOVES_PER_POSITION];
    int n_moves = genAllLegalMoves(board, movelist);
    int i;

    if (depth == 1)
    {
        // Nodes
        pi->nodes += n_moves;

        int enemyColor = (bgetcol(board->info) == _WHITE) ? BLACK : WHITE;
        uint64_t enemyPieces = 0UL;
        for (i = 0; i < 6; ++i)
            enemyPieces |= board->pieces[enemyColor + i];

        for (i = 0; i < n_moves; ++i)
        {

            if ((mgetpiece(movelist[i]) == PAWN) && (mgetsrc(movelist[i]) == ID5))
            {
                //printBoard(board);
                //printMove(movelist[i]);
            }

            // Captures
            if (mgetdstbb(movelist[i]) & enemyPieces)
            {
                // printMove(movelist[i]);
                pi->captures++;
            }

            // En passants
            if ((mgetpiece(movelist[i]) == PAWN) && bgetenp(board->info)
                && (mgetdst(movelist[i]) == bgetenpsquare(board->info)))
            {
                pi->enpassants++;
                pi->captures++;
            }

            // Checks
            boardMove(board, movelist[i]);

            // TESTS
            //printBoard(board);
            //printMove(movelist[i]);
            //printFen(board);

            uint64_t attack_map = genAllAttackMap(board, enemyColor ^ BLACK);
            if (attack_map & board->pieces[enemyColor + KING])
            {
                //printFen(board);
                pi->checks++;
            }
            undoMove(board, movelist[i]);

            // Castles
            if (mgetpiece(movelist[i]) == KING)
            {
                if (bgetcol(board->info) == _WHITE)
                {
                    if ((bgetcas(board->info) & 0x8) &&
                            mgetdst(movelist[i]) == IC1)
                        pi->castles++;
                    else if ((bgetcas(board->info) & 0x4) &&
                            mgetdst(movelist[i]) == IG1)
                        pi->castles++;
                }
                else if (bgetcol(board->info) == _BLACK)
                {
                    if ((bgetcas(board->info) & 0x2) &&
                            mgetdst(movelist[i]) == IC8)
                        pi->castles++;
                    else if ((bgetcas(board->info) & 0x1) &&
                            mgetdst(movelist[i]) == IG8)
                        pi->castles++;
                }
            }
        }
        return;
    }

    for (i = 0; i < n_moves; ++i)
    {
        //Board t;
        //memcpy(&t, board, sizeof(Board));
        //boardMove(&t, movelist[i]);
        //perftRun(&t, pi, depth - 1);
        boardMove(board, movelist[i]);
        perftRun(board, pi, depth - 1);
        undoMove(board, movelist[i]);
    }
}

void printPerft(PerftInfo pi)
{
    #ifdef CSV
    printf("%d,%ld,%ld,%ld,%ld,%ld,%ld\n", NUM_THREADS,
           pi.nodes, pi.captures, pi.enpassants, pi.castles, pi.checks,
           pi.checkmates);
    #else
    printf("\nNodes: %ld\nCaptures: %ld\nEn Passants: %ld\nCastles: %ld\nChecks: "
           "%ld\nCheckmates: %ld\n",
           pi.nodes, pi.captures, pi.enpassants, pi.castles, pi.checks,
           pi.checkmates);
    #endif
}
