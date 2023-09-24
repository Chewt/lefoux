#ifndef ENGINE
#define ENGINE

#include "board.h"

typedef struct {
    uint64_t nodes;
    uint64_t captures;
    uint64_t enpassants;
    uint64_t checks;
    uint64_t checkmates;
    uint64_t castles;
    uint64_t promotions;
} PerftInfo;

Move find_best_move(Board* board, uint8_t depth);
int8_t evaluateBoard(Board* board);
void perftRun(Board* board, PerftInfo* pi, uint8_t depth);
void printPerft(PerftInfo pi);

#endif
