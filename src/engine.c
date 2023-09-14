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
   int8_t i;
   uint64_t bitboard;
   int8_t numBits;
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
 * Populates the array moves with legal moves
 * and returns the number of legal moves.
 * Use MAX_MOVES_PER_POSITION as the max size
 * for moves
 */
int8_t genAllLegalMoves(Board *board, Move *moves)
{
   return 0;
}

/*
 * Updates the board based on the data provided in
 * move. Assumes the move is legal
 * Idea: Maybe return a move that can undo the current
 * move utilizing the 5 unused bits to give info about undo.
 * This could save on memory by storing a move instead of
 * a whole board in minMax()
 */
void boardMove(Board *board, Move move)
{
   return;
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
   int8_t score;
   Move bestMove;
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
   int8_t score;
   Move bestMove;
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

