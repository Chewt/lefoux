#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"

// 800 bytes needed per TEntry, so good initial capacity is probably around 1GiB (0x40000000 bytes) of memory
#define ZOBRIST_INIT_CAPACITY ((int)( 0x40000000 / sizeof(TEntry)))

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

void zobrist_init(Zobrist *table);
int zhash_board( Board *board );
int zhash_move( Board *board, Move move );
TEntry* zobrist_read( int hash );
TEntry* zobrist_write( int hash, TEntry te );
TEntry* zobrist_write_table( Zobrist *table, int hash, TEntry te );
TEntry* zobrist_read_table(Zobrist *table, int hash);
void zobrist_free( Zobrist *table );

#endif
