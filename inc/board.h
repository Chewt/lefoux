#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>

#define MAX_MOVES_PER_POSITION 218

/******************************************************************************
 * Contains all the necessary information for a given position.
 *
 * - pieces is an array of bitboards, one for each piece for each color.
 *   pieces is indexed with the piece type and color added together, for
 *   example: board.pieces[PAWN + WHITE] gives the white pawns bitboard
 * - info contains extra information about the board and is arranged like so:
 *
 * | En passant  | Castling  | Color to play
 * +---------------------------------+
 * | 6 bits      | 4 bits    | 1 bit |
 * +---------------------------------+
 * The first 5 bits are not used.
 *
 * - En passant describes where an en passant play can happen if at all.
 * - Castling describes which castling options are still possible.
 * - Color to play describes whether it is black or white to play.
 *****************************************************************************/
typedef struct
{
    uint64_t pieces[12];
    uint16_t info;
} Board;

enum {
   PAWN = 0,
   KNIGHT,
   BISHOP,
   ROOK,
   QUEEN,
   KING
};

enum {
   WHITE = 0,
   BLACK = 7
};

// Extract en passant from info
#define bgetenp(x) ((uint8_t)(0x3F & (x >> 5)))

// Extract castling from info
#define bgetcas(x) ((uint8_t)(0x0F & (x >> 1)))

// Extract color to play from info.
#define bgetcol(x) ((uint8_t)(0x01 & x))

/******************************************************************************
 * Move information stored in a single 32 bit integer
 *
 * Information is laid out like so:
 *
 * | Weight        | Source      | Destination | Piece   | Promote | Color
 * +-----------------------------------------------------------------------+
 * | 8 bits        | 6 bits      | 6 bits      | 3 bit   | 3 bit   | 1 bit |
 * +-----------------------------------------------------------------------+
 * The first 5 bits are not used.
 *
 * - Weight is the calculated strength of the move.
 * - Source is the source location of the piece that will move
 * - Destination is the destination of the piece that will move
 * - Piece is the type of piece that is moving
 * - Color is the color of the piece that is moving
 * - Promote is the piece that a pawn will promote to
 *****************************************************************************/
typedef uint32_t Move;

// Extract move weight
#define mgetweight(x) ((int8_t)(0xFF & (x >> 19)))
// Returns a move with the weight set
#define msetweight(m, v) ((Move)(m | ((0xFF & v) << 19))

// Extract source location from move
#define mgetsrc(x)    ((uint8_t)(0x3F & (x >> 13)))

// Extract destination location from move
#define mgetdst(x)    ((uint8_t)(0x3F & (x >>  7)))

// Extract piece type from move
#define mgetpiece(x)  ((uint8_t)(0x07 & (x >>  4)))

// Extract promotion piece from move
#define mgetprom(x)   ((uint8_t)(0x07 & (x >>  1)))

// Extract color to play from move.
#define mgetcol(x)    ((uint8_t)(0x01 & x))

#endif /* end of include guard: BOARD_H */
