#include <stdio.h>
#include <limits.h>     // INT_MIN
#include <stdlib.h>     // rand()
#include <stdint.h>     // Fancy integer types
#include <string.h>     // memcpy()
#include <omp.h>

#include "engine.h"
#include "board.h"
#include "bitHelpers.h"

#ifndef NUM_THREADS
#define NUM_THREADS 1
#endif

/*
 * Returns the net weight of pieces on the board. A positive number
 * indicates an advantage for white, negative for black, 0 for even.
 */
int8_t netWeightOfPieces(Board* board)
{
    int8_t weight = 0;
    const int8_t pawnWeight = 1;
    const int8_t knightWeight = 3;
    const int8_t bishopWeight = 3;
    const int8_t rookWeight = 5;
    const int8_t queenWeight = 8;

    weight += ( getNumBits(board->pieces[PAWN + WHITE])
            - getNumBits(board->pieces[PAWN + BLACK]) )
            * pawnWeight;
    weight += ( getNumBits(board->pieces[KNIGHT + WHITE])
            - getNumBits(board->pieces[KNIGHT + BLACK]) )
            * knightWeight;
    weight += ( getNumBits(board->pieces[BISHOP + WHITE])
            - getNumBits(board->pieces[BISHOP + BLACK]) )
            * bishopWeight;
    weight += ( getNumBits(board->pieces[ROOK + WHITE])
            - getNumBits(board->pieces[ROOK + BLACK]) )
            * rookWeight;
    weight += ( getNumBits(board->pieces[QUEEN + WHITE])
            - getNumBits(board->pieces[QUEEN + BLACK]) )
            * queenWeight;

    return weight;
}

int8_t evaluateBoard(Board* board)
{
    return netWeightOfPieces(board);
}

/*
 * Computes the weight of a move in a position using minMax.
 * Depth is in half-moves
 */
int8_t minMax(Board* board, uint8_t depth)
{
    if (depth == 0) return evaluateBoard(board);
    // Assumes MAX_MOVES_PER_POSITION < 256
    uint8_t i;
    uint8_t numMoves;
    int color = bgetcol(board->info) ? BLACK : WHITE;

    // Not used for the present moment
    //int8_t score;

    Move bestMove = 0;
    // MAX_MOVES_PER_POSITION*sizeof(Move) = 218 * 4 = 872 bytes
    Move moves[MAX_MOVES_PER_POSITION];
    numMoves = genAllLegalMoves(board, moves);
    for (i=0; i<numMoves; i++)
    {
        Move currentMove = moves[i];
        boardMove(board, currentMove);
        // Update the move with its weight
        int weight = -minMax(board, depth -1);
        moves[i] = msetweight(currentMove, weight);
        undoMove(board, currentMove);
        // If the weight of the current move is higher than the best move,
        // update the best move. If they are the same, update half of the time
        // randomly. This favors later tying moves, oh well.
        if (bestMove == 0)
            bestMove = moves[i];
        else if (mgetweight(moves[i]) > mgetweight(bestMove) ||
                (mgetweight(moves[i]) == mgetweight(bestMove) && rand() & 1))
        {
            bestMove = moves[i];
        }
    }
    return mgetweight(bestMove);
}

int alphaBetaMin( Board* board, int alpha, int beta, int depthleft );
int alphaBetaMax( Board* board, int alpha, int beta, int depthleft ) {
    if ( depthleft == 0 ) return evaluateBoard(board);
    Move moves[MAX_MOVES_PER_POSITION];
    uint8_t numMoves = genAllLegalMoves(board, moves);
    int i;
    for (i = 0; i < numMoves; ++i) {
        Move undo = moves[i];
        boardMove(board, moves[i]);
        int weight = alphaBetaMin(board, alpha, beta, depthleft - 1 );
        undoMove(board, undo);
        if( weight >= beta )
            return beta;   // fail hard beta-cutoff
        if( weight > alpha )
            alpha = weight; // alpha acts like max in MiniMax
    }
    return alpha;
}

int alphaBetaMin( Board* board, int alpha, int beta, int depthleft ) {
    if ( depthleft == 0 ) return -evaluateBoard(board);
    Move moves[MAX_MOVES_PER_POSITION];
    uint8_t numMoves = genAllLegalMoves(board, moves);
    int i;
    for (i = 0; i < numMoves; ++i) {
        Move undo = moves[i];
        boardMove(board, moves[i]);
        int weight = alphaBetaMax( board, alpha, beta, depthleft - 1 );
        undoMove(board, undo);
        if( weight <= alpha )
            return alpha; // fail hard alpha-cutoff
        if( weight < beta )
            beta = weight; // beta acts like min in MiniMax
    }
    return beta;
}

Move findBestMove(Board* board, uint8_t depth)
{
    // Assumes MAX_MOVES_PER_POSITION < 256
    uint8_t i;
    uint8_t numMoves;

    int color = bgetcol(board->info) ? BLACK : WHITE;
    Move bestMove = 0;
    // MAX_MOVES_PER_POSITION*sizeof(Move) = 218 * 4 = 872 bytes
    Move moves[MAX_MOVES_PER_POSITION];
    numMoves = genAllLegalMoves(board, moves);
    for (i=0; i<numMoves; i++)
    {
        // Copy of move is needed to preserve prevInfo
        Move currentMove = moves[i];
        boardMove(board, currentMove);
        // Update the move with its weight
        //int weight = minMax(board, depth - 1);
        int weight = alphaBetaMax(board, -128, 128, depth);
        moves[i] = msetweight(currentMove, weight);
            
        undoMove(board, currentMove);
        // If the weight of the current move is higher than the best move,
        // update the best move. If they are the same, update half of the time
        // randomly. This favors later tying moves, oh well.
        if (bestMove == 0)
            bestMove = moves[i];
        else if (mgetweight(moves[i]) > mgetweight(bestMove) ||
                (mgetweight(moves[i]) == mgetweight(bestMove) && rand() & 1))
        {
            bestMove = moves[i];
        }
    }
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

#pragma omp parallel for
    for (i = 0; i < n_moves; ++i)
    {
        Board t;
        memcpy(&t, &(boards[omp_get_thread_num()]), sizeof(Board));
        boardMove(&t, movelist[i]);
        perftRun(&t, &(pilist[omp_get_thread_num()]), depth - 1);
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
    printf("\nNodes: %ld\nCaptures: %ld\nEn Passants: %ld\nCastles: %ld\nChecks: "
           "%ld\nCheckmates: %ld\n",
           pi.nodes, pi.captures, pi.enpassants, pi.castles, pi.checks,
           pi.checkmates);
}
