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
 *   In order, it goes white queenside, white kingside, black queenside, black
 *   kingside.
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

/*
 * @brief enum for piece types. Can be combined with color using '+' i.e.
 * PAWN + WHITE
 */
typedef enum {
   PAWN = 0,
   KNIGHT,
   BISHOP,
   ROOK,
   QUEEN,
   KING,
   _PAWN,
   _KNIGHT,
   _BISHOP,
   _ROOK,
   _QUEEN,
   _KING,
} enumPiece;

/*
 * @brief enum for piece colors. Can be combined with piece type using '+' i.e.
 * PAWN + WHITE
 */
typedef enum {
   WHITE = 0,
   BLACK = 6
} enumColor;

/*
 * @brief enum for piece squares
 */
typedef enum {
    A1=0x1,
    B1=0x2,
    C1=0x4,
    D1=0x8,
    E1=0x10,
    F1=0x20,
    G1=0x40,
    H1=0x80,
    A2=0x100,
    B2=0x200,
    C2=0x400,
    D2=0x800,
    E2=0x1000,
    F2=0x2000,
    G2=0x4000,
    H2=0x8000,
    A3=0x10000,
    B3=0x20000,
    C3=0x40000,
    D3=0x80000,
    E3=0x100000,
    F3=0x200000,
    G3=0x400000,
    H3=0x800000,
    A4=0x1000000,
    B4=0x2000000,
    C4=0x4000000,
    D4=0x8000000,
    E4=0x10000000,
    F4=0x20000000,
    G4=0x40000000,
    H4=0x80000000,
    A5=0x100000000,
    B5=0x200000000,
    C5=0x400000000,
    D5=0x800000000,
    E5=0x1000000000,
    F5=0x2000000000,
    G5=0x4000000000,
    H5=0x8000000000,
    A6=0x10000000000,
    B6=0x20000000000,
    C6=0x40000000000,
    D6=0x80000000000,
    E6=0x100000000000,
    F6=0x200000000000,
    G6=0x400000000000,
    H6=0x800000000000,
    A7=0x1000000000000,
    B7=0x2000000000000,
    C7=0x4000000000000,
    D7=0x8000000000000,
    E7=0x10000000000000,
    F7=0x20000000000000,
    G7=0x40000000000000,
    H7=0x80000000000000,
    A8=0x100000000000000,
    B8=0x200000000000000,
    C8=0x400000000000000,
    D8=0x800000000000000,
    E8=0x1000000000000000,
    F8=0x2000000000000000,
    G8=0x4000000000000000,
    H8=0x8000000000000000,
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

// Convert source or destination index to bitboard
#define indextobb(x)  ((uint64_t)(0x1UL << x))

// Extract source location from move
#define mgetsrc(x)    ((uint8_t)(0x3F & (x >> 13)))

// Extract destination location from move
#define mgetdst(x)    ((uint8_t)(0x3F & (x >>  7)))

// Extract source location as bitboard
#define mgetsrcbb(x)  (indextobb(mgetsrc(x)))

// Extract destination location as bitboard
#define mgetdstbb(x)  (indextobb(mgetdst(x)))

// Extract piece type from move
#define mgetpiece(x)  ((uint8_t)(0x07 & (x >>  4)))

// Extract promotion piece from move
#define mgetprom(x)   ((uint8_t)(0x07 & (x >>  1)))

// Extract color to play from move.
#define mgetcol(x)    ((uint8_t)(0x01 & x))

extern const uint64_t RDIAG;
extern const uint64_t LDIAG;
extern const uint64_t VERT;
extern const uint64_t HORZ;
extern const uint64_t NMOV;  // Nc3
extern const uint64_t KMOV;  // Kb2
extern const uint64_t PATTK;  // b2

#define AFILE FILELIST[0]
#define HFILE FILELIST[7]
extern const uint64_t FILELIST[8];
extern const uint64_t RANK[8];

/*
 * magicLookupBishop
 * @param occupancy a bitboard of all of the pieces
 * @param square the square the attacking bishop is attacking from
 * @return bitboard of all attacks that bishop can make
 */
uint64_t magicLookupBishop(uint64_t occupancy, enumSquare square);
/*
 * magicLookupRook
 * @param occupancy a bitboard of all of the pieces
 * @param square the square the attacking rook is attacking from
 * @return bitboard of all attacks that rook can make
 */
uint64_t magicLookupRook(uint64_t occupancy, enumSquare square);

/*
 * getMoves
 * @param type the type of piece in parameter pieces
 * @param pieces a bitboard of all of the friendly pieces of the type specified
 *   in parameter type
 * @param friends a bitboard of all of the friendly pieces
 * @param foes a bitboard of all of the enemy pieces
 * @return bitboard of all possible moves this piece can make
 */
uint64_t getMoves(enumPiece type, uint64_t pieces, uint64_t friends, uint64_t foes);

/*
 * printBitboard
 * @param bb a bitboard to print to stdout
 */
void printBitboard(uint64_t bb);

#endif /* end of include guard: BOARD_H */
