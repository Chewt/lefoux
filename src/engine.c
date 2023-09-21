#include <limits.h>     // INT_MIN
#include <stdlib.h>     // rand()
#include <stdint.h>     // Fancy integer types
#include <string.h>     // memcpy()

#include "board.h"
#include "bitHelpers.h"

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

    // Not used for the present moment
    //int8_t score;

    Move bestMove = -300;
    Board tmpBoard;
    // MAX_MOVES_PER_POSITION*sizeof(Move) = 218 * 4 = 872 bytes
    Move moves[MAX_MOVES_PER_POSITION];
    numMoves = genAllLegalMoves(board, moves);
    for (i=0; i<numMoves; i++)
    {
        memcpy(&tmpBoard, board, sizeof(Board));

        boardMove(&tmpBoard, moves[i]);
        // Update the move with its weight
        moves[i] = msetweight(moves[i], minMax(&tmpBoard, depth-1));
        // If the weight of the current move is higher than the best move,
        // update the best move. If they are the same, update half of the time
        // randomly. This favors later tying moves, oh well.
        if (mgetweight(moves[i]) > mgetweight(bestMove) ||
                (mgetweight(moves[i]) == mgetweight(bestMove) && rand() & 1))
        {
            bestMove = moves[i];
        }
    }
    return mgetweight(bestMove);
}

Move find_best_move(Board* board, uint8_t depth)
{
    // Assumes MAX_MOVES_PER_POSITION < 256
    uint8_t i;
    uint8_t numMoves;

    // Not used for the present moment
    //int8_t score;

    Move bestMove = 0;
    Board tmpBoard;
    // MAX_MOVES_PER_POSITION*sizeof(Move) = 218 * 4 = 872 bytes
    Move moves[MAX_MOVES_PER_POSITION];
    numMoves = genAllLegalMoves(board, moves);
    for (i=0; i<numMoves; i++)
    {
        memcpy(&tmpBoard, board, sizeof(Board));

        boardMove(&tmpBoard, moves[i]);
        // Update the move with its weight
        moves[i] = msetweight(moves[i], minMax(&tmpBoard, depth-1));
        // If the weight of the current move is higher than the best move,
        // update the best move. If they are the same, update half of the time
        // randomly. This favors later tying moves, oh well.
        if (mgetweight(moves[i]) > mgetweight(bestMove) ||
                (mgetweight(moves[i]) == mgetweight(bestMove) && rand() & 1))
        {
            bestMove = moves[i];
        }
    }
    return bestMove;
}

