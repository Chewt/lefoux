#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"

#define ZOBRIST_INIT_CAPACITY 0xFFFFFFF

typedef struct
{
   uint8_t depth;
   int8_t score;
   Board board;
} TEntry;

typedef struct
{
    int capacity;
    TEntry *items;
} Zobrist;

typedef struct
{
   int pieceSquare[12][64];
   int blackToMove;
   int castling[4];
   int enPassant[8];
} ZHashes;

// g_zhashes is a global table with prand values for computing the Zobrist hash
// This table is intended to be read-only outside of zobrist_init
ZHashes g_zhashes;
// g_ztable is a global hash table containing TEntries storing depth, score,
// and board state intended to store board positions that have already been
// calculated.
Zobrist g_ztable;

void zobrist_init();
int zhash_board( Board *board );
int zhash_move( Board *board, Move move );
TEntry* zobrist_read( int hash );
TEntry* zobrist_write( int hash, TEntry te );
void zobrist_free( Zobrist *table );

#endif
