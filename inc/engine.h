#ifndef ENGINE
#define ENGINE

#include "board.h"

Move find_best_move(Board* board, uint8_t depth);
int8_t evaluateBoard(Board* board);

#endif
