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
 * Castling: describes which castling options are still possible in the position.
 *
 * Color: to play describes whether it is black or white to play.
 *****************************************************************************/
typedef struct
{
    uint64_t pieces[12];
    uint16_t info;
} Board;


/******************************************************************************
 * Move information stored in a single 32 bit integer
 * 
 * Information is laid out like so:
 *
 * | Weight        | source      | destination | piece   | color | promotion
 * +-----------------------------------------------------------------------+
 * | 8 bits        | 6 bits      | 6 bits      | 3 bit   | 1 bit | 3 bit   |  
 * +-----------------------------------------------------------------------+
 *
 * Weight is the calculated strength of the move.
 *****************************************************************************/
typedef uint32_t Move;

#endif /* end of include guard: BOARD_H */
