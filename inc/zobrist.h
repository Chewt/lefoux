#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"

// Good initial capacity is probably around 1GiB (0x40000000 bytes) of memory
// Need to keep this a power of 2!
#define ZOBRIST_INIT_CAPACITY ((int)( 2 * (0x40000000 / sizeof(TEntry))))
// #define ZOBRIST_INIT_CAPACITY ((int)(2048))

typedef struct
{
   uint8_t depth;
   int8_t score;
   // Extra var keeps this struct a power of 2
   uint16_t alignmentVar;
   int hash;
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

void zobrist_init(Zobrist *table);
int zhash_board( Board *board );
int zhash_move( Board *board, Move move );
TEntry* zobrist_read( int hash );
TEntry* zobrist_write( int hash, TEntry te );
TEntry* zobrist_write_table( Zobrist *table, int hash, TEntry te );
TEntry* zobrist_read_table(Zobrist *table, int hash);
void zobrist_free( Zobrist *table );

#endif
