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
 * - The square a1 is bitboard & 1, b1 is bitboard & 2, ...,
 *   a2 is bitboard & 0x0100, b2 is bitboard & 0x0200, ..., g8 is
 *   bitboard & 0x4000000000000000, h8 is bitboard & 0x8000000000000000
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


typedef enum {
   WHITE_PAWN = 0,
   WHITE_KNIGHT,
   WHITE_BISHOP,
   WHITE_ROOK,
   WHITE_QUEEN,
   WHITE_KING,
   BLACK_PAWN,
   BLACK_KNIGHT,
   BLACK_BISHOP,
   BLACK_ROOK,
   BLACK_QUEEN,
   BLACK_KING
} enumPiece;

typedef enum {
   WHITE = 0,
   BLACK = 7
} enumColor;

typedef enum {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
} enumSquare;

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
#define msetweight(m, v) ((Move)(m | ((0xFF & v) << 19)))

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

/* Constants for piece attacks */
const uint64_t RDIAG;
const uint64_t LDIAG;
const uint64_t VERT ;
const uint64_t HORZ ;
const uint64_t NMOV ;
const uint64_t KMOV ;
const uint64_t PATTK;

#define AFILE FILELIST[0]
#define HFILE FILELIST[7]
const uint64_t FILELIST[8];

const uint64_t RANK[8];

typedef struct {
    uint64_t mask;
    uint64_t magic;
} Magic;

Magic magicBishop[64];
Magic magicRook[64];

uint64_t magicBishopAttacks[64][512];
uint64_t magicRookAttacks[64][4096];

uint64_t magicLookupBishop(uint64_t occupancy, enumSquare square);
uint64_t magicLookupRook(uint64_t occupancy, enumSquare square);

uint64_t getMoves(enumPiece, uint64_t pieces, uint64_t friends, uint64_t foes);

void print_bitboard(uint64_t bb);

int computeRookMagic();
int computeBishopMagic();

#endif /* end of include guard: BOARD_H */
