#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zobrist.h"
#include "board.h"
#include "bitHelpers.h"

uint64_t zrand() 
{
    uint64_t num = 0;
    num = rand();
    num <<= 32;
    num |= rand();
    return num;
}

// g_zhashes is a global table with prand values for computing the Zobrist hash
// This table is intended to be read-only outside of zobrist_init
static ZHashes g_zhashes = {0};

// g_ztable is a global hash table containing TEntries storing depth, score,
// and board state intended to store board positions that have already been
// calculated.
static Zobrist g_ztable = {0};

// Clear the given zobrist table
void zobrist_clear(Zobrist* table) {
    if (table->items)
        free(table->items);
    table->items = calloc(table->capacity, sizeof(TEntry));
}

// Initialize the given zobrist table, and the prand values in g_zhashes if not
// already initialized.
void zobrist_init(Zobrist* table)
{
    // initialize the given hash table, or the global hash table
    Zobrist* t = (table) ? table : &g_ztable;
    t->capacity = ZOBRIST_INIT_CAPACITY;
    zobrist_clear(t);

    // Initialize prand numbers if not initialized
    ZHashes empty_zhashes = {0};
    if (!memcmp(&g_zhashes, &empty_zhashes, sizeof(ZHashes)))
    {
        srand(80085); // Sets the random number seed for predictable randomness
                  // srand(time(null)); // Sets the random seed for unknown randomness
        g_zhashes.blackToMove = zrand();
        for (int i=PAWN; i<=_KING; i++)
        {
            for (int j=IA1; j<=IH8; j++)
            {
                g_zhashes.pieceSquare[i][j] = zrand();
            }
        }
        for (int i=0; i<4; i++)
        {
            g_zhashes.castling[i] = zrand();
        }
        for (int i=0; i<4; i++)
        {
            g_zhashes.enPassant[i] = zrand();
        }
    }
}

// Computes a hash for the provided board
// Uses g_zhashes read-only, thread safe
int zhash_board( Board *board )
{
    uint64_t hash = 0;
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
    uint64_t hash = 0;
    int color = (mgetcol(move) == _WHITE) ? WHITE : BLACK;

    /* Remove src piece */
    hash ^= g_zhashes.pieceSquare[mgetpiece(move) + color][mgetsrc(move)];
    /* Add dst piece */
    hash ^= g_zhashes.pieceSquare[mgetpiece(move) + mgetprom(move) + color][mgetdst(move)];

    /* Move rooks when castling */
    /* White Kingside */
    if ((mgetpiece(move) == KING) && 
        (mgetsrc(move)   == IE1)  && 
        (mgetdst(move)   == IG1))
    {
        hash ^= g_zhashes.pieceSquare[ROOK + WHITE][IH1];
        hash ^= g_zhashes.pieceSquare[ROOK + WHITE][IF1];
    }
    /* White Queenside */
    else if ((mgetpiece(move) == KING) &&
             (mgetsrc(move)   == IE1)  &&
             (mgetdst(move)   == IC1))
    {
        hash ^= g_zhashes.pieceSquare[ROOK + WHITE][IA1];
        hash ^= g_zhashes.pieceSquare[ROOK + WHITE][ID1];
    }
    /* Black Kingside */
    else if ((mgetpiece(move) == KING) &&
            (mgetsrc(move)    == IE8)  &&
            (mgetdst(move)    == IG8))
    {
        hash ^= g_zhashes.pieceSquare[ROOK + BLACK][IH8];
        hash ^= g_zhashes.pieceSquare[ROOK + BLACK][IF8];
    }
    /* Black Queenside */
    else if ((mgetpiece(move) == KING) &&
             (mgetsrc(move)   == IE8)  &&
             (mgetdst(move)   == IC8))
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

// should have a white KING (right?)
int is_entry_empty(TEntry et) {
    return !et.hash;
}

TEntry* zobrist_get(Zobrist *table, Board* board)
{
    uint64_t hash = zhash_board(board);
    int table_index = hash & (table->capacity - 1);
    for (int i = 0; i < table->capacity; i++)
    {
        // If the boards aren't the same, check the next bucket
        if (table->items[table_index].hash != hash) 
        {
            table_index = (table_index + 1) & (table->capacity - 1);
            continue;
        }
        return zobrist_read_table(table, table_index);
    }
    return NULL;
}

void zobrist_resize(Zobrist *table);
TEntry* zobrist_put(Zobrist *table, TEntry te)
{
    uint64_t hash = te.hash;
    // First check resize lock to see if table is being resized and wait until
    // it is available (no need to take the lock though, only the resize
    // function should take it)

    // A naive solution would be to have a global lock on all writes to table,
    // or store one lock for every bucket and only lock when trying to write to
    // a specific bucket. If the lock is free, grab the lock and write to the
    // bucket. If its not free (at the time of trying to write to it), then
    // wait until it is free, grab the lock, check for a collision.

    // TODO idk this is complicated
    // Capacity here isn't reliable during a resize. Will want to wait for
    // any resize functionality to finish before continuing, however we don't
    // need a lock here for every write operation
    int table_index = hash & (table->capacity - 1);
    for (int i = 0; i < table->capacity; i++)
    {
        // If the boards aren't the same, check the next bucket
        if (table->items[table_index].hash != hash) 
        {
            table_index = (table_index + 1) & (table->capacity - 1);
            continue;
        }
        // At this point, we've found a bucket we want to write to, however another
        // thread could've found the same bucket.
        // TODO obtain a lock here, then check that the bucket is still free
        if (table->items[table_index].hash != hash)
        {
            // TODO free the lock
            continue;
        }
        // Check if the item we're looking at is a board. All boards
        if (is_entry_empty(table->items[table_index]))
        {
            table->items[table_index] = te;
            // TODO free the lock
            return &(table->items[table_index]);
        }
        // Keep the entry that computed to the furthest depth
        if ( table->items[table_index].depth < te.depth )
            table->items[table_index] = te;
        // TODO free the lock
        return &(table->items[table_index]);
    }
    // We've traversed the whole table and collided in every bucket. Time to resize
    zobrist_resize(table);
    return zobrist_write_table(table, hash, te);

}

TEntry* zobrist_read_table(Zobrist *table, uint64_t hash)
{
    TEntry* entry = &table->items[hash & (table->capacity - 1)];
    return (is_entry_empty(*entry) || entry->hash != hash) ? NULL : entry;
}

// Returns a pointer to the table entry. Returns NULL if the entry hasn't
// been evaluated yet
// Thread safe, except when table is being resized
TEntry* zobrist_read(uint64_t hash)
{
    return zobrist_read_table(&g_ztable, hash);
}

TEntry* zobrist_write_table(Zobrist *table, uint64_t hash, TEntry te);

// Doubles the size of the table
// Contains a race condition with readers and writers
void zobrist_resize(Zobrist *table)
{
    Zobrist new_table = {
        .capacity = table->capacity * 2,
        .items = calloc( table->capacity * 2, sizeof(TEntry) )
    };
    // Deep copy
    for (int i=0; i < table->capacity; i++) {
        zobrist_put( &new_table, table->items[i] );
    }
    // Use after free race condition here for readers and writers
    free(table->items);
    table->capacity = new_table.capacity;
    table->items = new_table.items;
}

// Writes the provided table entry at hash point IF the provided table
// entry has a higher depth. Returns the table entry written to the hash
// location (or the existing table entry if no write occurred)
TEntry* zobrist_write_table( Zobrist *table, uint64_t hash, TEntry te )
{
    int table_index = hash & (table->capacity - 1);
// Commented out to reduce serialization overhead, we accept any and all risk :D
#pragma omp critical (zobristWrite)
    if (table->items[table_index].depth < te.depth)
        table->items[table_index] = te;
    return &(table->items[table_index]);
}

TEntry* zobrist_write( uint64_t hash, TEntry te )
{
    return zobrist_write_table( &g_ztable, hash, te );
}

// Clear memory used by the table. Leave freeing the table itself up to the
// user, as they may have allocated it either on the stack, or the heap.
void zobrist_free( Zobrist *table )
{
    if (table->items)
        free(table->items);
    table->items = NULL;
}

