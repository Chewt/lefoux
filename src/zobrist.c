#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zobrist.h"
#include "board.h"
#include "bitHelpers.h"

Zobrist g_ztable = {0};

void zobrist_init()
{
   g_ztable.capacity = ZOBRIST_INIT_CAPACITY;
   g_ztable.items = calloc( ZOBRIST_INIT_CAPACITY, sizeof(TEntry) );

   srand(0); // Sets the random number seed for predictable randomness
   // srand(time(null)); // Sets the random seed for unknown randomness
   g_zhashes.blackToMove = rand();
   for (int i=PAWN; i<=_KING; i++)
   {
      for (int j=IA1; j<=IH8; j++)
      {
         g_zhashes.pieceSquare[i][j] = rand();
      }
   }
   for (int i=0; i<4; i++)
   {
      g_zhashes.castling[i] = rand();
   }
   for (int i=0; i<4; i++)
   {
      g_zhashes.enPassant[i] = rand();
   }
}

// Computes a hash for the provided board
// Uses g_zhashes read-only, thread safe
int zhash_board( Board *board )
{
   int hash = 0;
   uint64_t piece;
   for (int pieceType=PAWN; pieceType<=_KING; pieceType++)
   {
      uint64_t pieces = board->pieces[pieceType];
      while ((piece = pieces & -pieces)) {
         enumIndexSquare square = bitScanForward(piece);
         hash ^= g_zhashes.pieceSquare[pieceType][square];
         // Iterate to next piece
         pieces ^= piece;
      }
   }
   if (bgetcol(board->info) == _BLACK)
      hash ^= g_zhashes.blackToMove;
   if (board->info & 1 << 1)
      hash ^= g_zhashes.castling[0];
   if (board->info & 1 << 2)
      hash ^= g_zhashes.castling[1];
   if (board->info & 1 << 3)
      hash ^= g_zhashes.castling[2];
   if (board->info & 1 << 4)
      hash ^= g_zhashes.castling[3];
   uint8_t enPassant = (board->info >> 5) & 0x7;
   if ( enPassant )
      hash ^= g_zhashes.enPassant[enPassant];
   return hash;
}

// Computes a hash for the provided undo move
// Uses g_zhashes read-only, thread safe
int zhash_move( Board *board, Move move )
   {
      int hash = 0;
    int color = (mgetcol(move) == _WHITE) ? WHITE : BLACK;

    /* Remove src piece */
    hash ^= g_zhashes.pieceSquare[mgetpiece(move) + color][mgetsrc(move)];
    /* Add dst piece */
    hash ^= g_zhashes.pieceSquare[mgetpiece(move) + mgetprom(move) + color][mgetdst(move)];

    /* Move rooks when castling */
    /* White Kingside */
    if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE1)
                                  && (mgetdst(move) == IG1))
    {
       hash ^= g_zhashes.pieceSquare[ROOK + WHITE][IH1];
       hash ^= g_zhashes.pieceSquare[ROOK + WHITE][IF1];
    }
    /* White Queenside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE1)
                                       && (mgetdst(move) == IC1))
    {
       hash ^= g_zhashes.pieceSquare[ROOK + WHITE][IA1];
       hash ^= g_zhashes.pieceSquare[ROOK + WHITE][ID1];
    }
    /* Black Kingside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE8)
                                       && (mgetdst(move) == IG8))
    {
       hash ^= g_zhashes.pieceSquare[ROOK + BLACK][IH8];
       hash ^= g_zhashes.pieceSquare[ROOK + BLACK][IF8];
    }
    /* Black Queenside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE8)
                                       && (mgetdst(move) == IC8))
    {
       hash ^= g_zhashes.pieceSquare[ROOK + BLACK][IA8];
       hash ^= g_zhashes.pieceSquare[ROOK + BLACK][ID8];
    }

    uint16_t infoDiff = board->info ^ mgetprevinfo(move);

    // Always toggle the color!
   hash ^= g_zhashes.blackToMove;
   if (infoDiff & 1 << 1)
      hash ^= g_zhashes.castling[0];
   if (infoDiff & 1 << 2)
      hash ^= g_zhashes.castling[1];
   if (infoDiff & 1 << 3)
      hash ^= g_zhashes.castling[2];
   if (infoDiff & 1 << 4)
      hash ^= g_zhashes.castling[3];
   uint8_t enPassant = (board->info >> 5) & 0x7;
   if ( enPassant )
      hash ^= g_zhashes.enPassant[enPassant];
   return hash;
}


// Returns a pointer to the table entry. Returns NULL if the entry hasn't
// been evaluated yet
// Thread safe, except when table is being resized
TEntry* zobrist_read( int hash )
{
   return &(g_ztable.items[ hash % g_ztable.capacity ]);
}

TEntry* zobrist_write_table( Zobrist *table, int hash, TEntry te );

// Doubles the size of the table
// Contains a race condition with readers and writers
void zobrist_resize( Zobrist *table )
{
   Zobrist new_table = {
      .capacity = table->capacity * 2,
      .items = calloc( table->capacity * 2, sizeof(TEntry) )
   };
   // Deep copy
   for (int i=0; i < table->capacity; i++) {
      int hash = zhash_board(  &( table->items[i].board )  );
      zobrist_write_table( &new_table, hash, table->items[i] );
   }
   // Use after free race condition here for readers and writers
   {
      free(table->items);
      *table = new_table;
   }
}

// Writes the provided table entry at hash point IF the provided table
// entry has a higher depth. Returns the table entry written to the hash
// location (or the existing table entry if no write occurred)
//
// Thread UNSAFE
TEntry* zobrist_write_table( Zobrist *table, int hash, TEntry te )
{
   // TODO idk this is complicated
   // Capacity here isn't reliable during a resize. Will want to wait for
   // any resize functionality to finish before continuing, however we don't
   // need a lock here for every write operation
   hash %= table->capacity;
   for (int j = hash; j < table->capacity; j++)
   {
      // If the boards aren't the same, check the next bucket
      if (memcmp( &( table->items[j].board ), &(te.board), sizeof(Board) ))
         continue;
      // At this point, we've found a bucket we want to write to, however another
      // thread could've found the same bucket.
      // TODO obtain a lock here, then check that the bucket is still free
      if (memcmp( &( table->items[j].board ), &(te.board), sizeof(Board) ))
      {
         // TODO free the lock
         continue;
      }
      // Check if the item we're looking at is a board. All boards
      // should have a white KING (right?)
      if ( !(  table->items[j].board.pieces[KING]  ) )
      {
         table->items[j] = te;
         // TODO free the lock
         return &(table->items[j]);
      }
      // Keep the entry that computed to the furthest depth
      if ( table->items[j].depth < te.depth )
         table->items[j] = te;
      // TODO free the lock
      return &(table->items[j]);
   }
   // If we get here, we collided with every slot above hash, so check below!
   for (int j = hash - 1; j >= 0; j--)
   {
      // If the boards aren't the same, check the next bucket
      if (memcmp( &( table->items[j].board ), &(te.board), sizeof(Board) ))
         continue;
      // At this point, we've found a bucket we want to write to, however another
      // thread could've found the same bucket.
      // TODO obtain a lock here, then check that the bucket is still free
      if (memcmp( &( table->items[j].board ), &(te.board), sizeof(Board) ))
      {
         // TODO free the lock
         continue;
      }
      // Check if the item we're looking at is a board. All boards
      // should have a white KING (right?)
      if ( !(  table->items[j].board.pieces[KING]  ) )
      {
         table->items[j] = te;
         // TODO free the lock
         return &(table->items[j]);
      }
      // Keep the entry that computed to the furthest depth
      if ( table->items[j].depth < te.depth )
         table->items[j] = te;
      // TODO free the lock
      return &(table->items[j]);
   }
   // We've traversed the whole table and collided in every bucket. Time to resize
   zobrist_resize(table);
   return zobrist_write_table( table, hash, te );
}

TEntry* zobrist_write( int hash, TEntry te )
{
   return zobrist_write_table( &g_ztable, hash, te );
}

void zobrist_free( Zobrist *table )
{
   free(table);
}

