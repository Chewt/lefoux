#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>


/******************************************************************************
 * Contains all the necessary information for a given position.
 *
 * pieces is an array of bitboards, one for each piece for each color.
 *
 * info contains extra information about the board and is arranged like so:
 *
 * | En passant  | Castling  | Color to play
 * +---------------------------------+
 * | 6 bits      | 4 bits    | 1 bit |  
 * +---------------------------------+
 * The first 5 bits are not used
 *
 * En passant: describes where an en passant play can happen if at all.
 *
 * Castling: describes which castling options are still possible.
 *
 * Color: to play describes whether it is black or white to play.
 *****************************************************************************/
typedef struct
{
    uint64_t pieces[12];
    uint16_t info;
} Board;

// Extract en passant from info
#define getenp(x) (0x3F & (x >> 5))

// Extract castling from info
#define getcas(x) (0x0F & (x >> 1))

// Extract color to play from info.
// Also used to extract color from Move type below
#define getcol(x) (0x01 & x)


/******************************************************************************
 * Move information stored in a single 32 bit integer
 * 
 * Information is laid out like so:
 *
 * | Weight        | Source      | Destination | Piece   | Promotion | Color
 * +-----------------------------------------------------------------------+
 * | 8 bits        | 6 bits      | 6 bits      | 3 bit   | 3 bit   | 1 bit |  
 * +-----------------------------------------------------------------------+
 *
 * - Weight is the calculated strength of the move.
 * - Source is the source location of the piece that will move
 * - Destination is the destination of the piece that will move
 * - Piece is the type of piece that is moving
 * - Color is the color of the piece that is moving
 * - Promotion is the piece that a pawn will promote to
 *****************************************************************************/
typedef uint32_t Move;

#define getweight(x) ((int8_t)(0xFF & (x >> 19)))
#define getsrc(x)    ((int8_t)(0x3F & (x >> 13)))
#define getdst(x)    ((int8_t)(0x3F & (x >>  7)))
#define getpiece(x)  ((int8_t)(0x07 & (x >>  4)))
#define getprom(x)   ((int8_t)(0x07 & (x >>  1)))
// getcol(x) defined earlier in file

#endif /* end of include guard: BOARD_H */
