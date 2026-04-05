#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"

// Good initial capacity is probably around 1GiB (0x40000000 bytes) of memory
// #define ZOBRIST_INIT_CAPACITY ((int)( 4 * (0x40000000 / sizeof(TEntry))))
// Testing with power of two capacity
#define ZOBRIST_INIT_CAPACITY ((int)(0x8000000))

typedef enum 
{
    EXACT,
    LOWERBOUND,
    UPPERBOUND
} NodeType;

typedef struct
{
   uint8_t depth;
   int8_t score;
   uint64_t hash;
   NodeType nodeType;
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
TEntry* zobrist_read( uint64_t hash );
TEntry* zobrist_write( uint64_t hash, TEntry te );
TEntry* zobrist_write_table( Zobrist *table, uint64_t hash, TEntry te );
TEntry* zobrist_read_table(Zobrist *table, uint64_t hash);
void zobrist_free( Zobrist *table );

#endif
