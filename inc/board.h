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
 * | 4 bits      | 4 bits    | 1 bit |
 * +---------------------------------+
 * The first 7 bits are NOT TO BE USED.
 *
 * - En passant describes where an en passant play can happen if at all.
 *   msb is set when en passant is possible, lower three bits indicate the
 *   file that en passant is possible, rank can be computed by color to move
 * - Castling describes which castling options are still possible.
 *   0 0 0 0 <- lsb
 *   ^ ^ ^ ^
 *   | | | |
 *   | | | black kingside
 *   | | black queenside
 *   | white kingside
 *   white queenside
 * - Color to play describes whether it is black or white to play.
 * - Square enums below describe squares in two ways such as A2 and IA2. A2 is
 *   the bitboard with just A2 masked, while IA2 is the index of the square A2.
 *   As such, A2 = 1 << IA2.
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
    BLACK = 6,
    _WHITE = 0x00,
    _BLACK = 0x01
} enumColor;

/*
 * @brief enum for piece squares
 */
typedef enum {
    A1=0x1, B1=0x2, C1=0x4, D1=0x8, E1=0x10, F1=0x20, G1=0x40, H1=0x80,
    A2=0x100, B2=0x200, C2=0x400, D2=0x800, E2=0x1000, F2=0x2000, G2=0x4000,
    H2=0x8000, A3=0x10000, B3=0x20000, C3=0x40000, D3=0x80000, E3=0x100000,
    F3=0x200000, G3=0x400000, H3=0x800000, A4=0x1000000, B4=0x2000000,
    C4=0x4000000, D4=0x8000000, E4=0x10000000, F4=0x20000000, G4=0x40000000,
    H4=0x80000000, A5=0x100000000, B5=0x200000000, C5=0x400000000,
    D5=0x800000000, E5=0x1000000000, F5=0x2000000000, G5=0x4000000000,
    H5=0x8000000000, A6=0x10000000000, B6=0x20000000000, C6=0x40000000000,
    D6=0x80000000000, E6=0x100000000000, F6=0x200000000000, G6=0x400000000000,
    H6=0x800000000000, A7=0x1000000000000, B7=0x2000000000000,
    C7=0x4000000000000, D7=0x8000000000000, E7=0x10000000000000,
    F7=0x20000000000000, G7=0x40000000000000, H7=0x80000000000000,
    A8=0x100000000000000, B8=0x200000000000000, C8=0x400000000000000,
    D8=0x800000000000000, E8=0x1000000000000000, F8=0x2000000000000000,
    G8=0x4000000000000000, H8=0x8000000000000000,
} enumSquare;
/*
 * @brief enum for piece squares in index form
 */
typedef enum {
    IA1=0, IB1, IC1, ID1, IE1, IF1, IG1, IH1,
    IA2, IB2, IC2, ID2, IE2, IF2, IG2, IH2,
    IA3, IB3, IC3, ID3, IE3, IF3, IG3, IH3,
    IA4, IB4, IC4, ID4, IE4, IF4, IG4, IH4,
    IA5, IB5, IC5, ID5, IE5, IF5, IG5, IH5,
    IA6, IB6, IC6, ID6, IE6, IF6, IG6, IH6,
    IA7, IB7, IC7, ID7, IE7, IF7, IG7, IH7,
    IA8, IB8, IC8, ID8, IE8, IF8, IG8, IH8
} enumIndexSquare;

// Extract en passant from info
// Does en passant exist?
#define bgetenp(x) ((uint8_t)(0x1 & (x >> 8)))
#define bgetenpsquare(x) ((uint8_t)(((x >> 5) & 0x7) + ((bgetcol(x)) ? IA3 : IA6)))

// Extract castling from info
#define bgetcas(x) ((uint8_t)(0x0F & (x >> 1)))

// Extract color to play from info.
#define bgetcol(x) ((uint8_t)(0x01 & x))

/******************************************************************************
 * Move information stored in a single 32 bit integer
 *
 * Information is laid out like so:
 *
 * Pre-Weight generation:
 *
 * | Prev BoardInfo
 * | w/ taken piece | Source      | Destination | Piece   | Promote | Color
 * +-----------------------------------------------------------------------+
 * | 9 & 3 bits     | 6 bits      | 6 bits      | 3 bit   | 3 bit   | 1 bit |
 * +-----------------------------------------------------------------------+
 *
 * Post Weight generation:
 *
 * | Weight        | Source      | Destination | Piece   | Promote | Color
 * +-----------------------------------------------------------------------+
 * | 8 bits        | 6 bits      | 6 bits      | 3 bit   | 3 bit   | 1 bit |
 * +-----------------------------------------------------------------------+
 *
 * The first 5 bits are not used.
 *
 * - Weight is the calculated strength of the move.
 * - Source is the source location of the piece that will move. Lower three
 *   bits are file, upper 3 rank
 * - Destination is the destination of the piece that will move. Lower three
 *   bits are file, upper 3 rank
 * - Piece is the type of piece that is moving
 * - Color is the color of the piece that is moving
 * - Promote is the piece that a pawn will promote to
 *****************************************************************************/
typedef uint32_t Move;

// Extract move weight
#define mgetweight(x) ((int8_t)(0xFF & (x >> 19)))

// Returns a move with the weight set
#define msetweight(m, v) ((Move)((m & 0x7FFFF) | ((0xFF & v) << 19)))

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
#define mgetpiece(x)  ((uint8_t)(0x07 & ((x) >>  4)))

// Extract promotion piece from move
#define mgetprom(x)   ((uint8_t)(0x07 & ((x) >>  1)))

// Extract color to play from move.
#define mgetcol(x)    ((0x01 & (x)))

// Extract the taken piece
#define mgettaken(x)  ((x >> 19) & 0x07)

// Extract previous board info
#define mgetprevinfo(x)  ((uint16_t)((x >> 22) & 0x1ff))

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
uint64_t magicLookupBishop(uint64_t occupancy, enumIndexSquare square);
/*
 * magicLookupRook
 * @param occupancy a bitboard of all of the pieces
 * @param square the square the attacking rook is attacking from
 * @return bitboard of all attacks that rook can make
 */
uint64_t magicLookupRook(uint64_t occupancy, enumIndexSquare square);

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

/*
 * genAllLegalMoves
 * @param board a pointer to a Board struct
 * @param moves a pointer to a preallocated array of type Move
 * @return number of moves in the array
 */
int8_t genAllLegalMoves(Board *board, Move *moves);

/*
 * boardMove
 * @param board a pointer to a Board struct
 * @param move a Move to make on the board
 */
uint16_t boardMove(Board *board, Move move);

/*
 * getDefaultBoard
 * @return A Board initialized to untouched chess board
 */
Board getDefaultBoard();

/*
 * printBoardInfo
 * @param info the info section of a Board struct
 */
void printBoardInfo(uint16_t info);

/*
 * @param board the board to print to stdout
 */
void printBoard(Board *board);

/*
 * @param move move to pretty print
 */
void printMove(Move move);

/*
 * @param move move to print in SAN
 */
void printMoveSAN(Move move);

/* TODO Stuff
 */
uint64_t genPieceAttackMap(Board* board, int pieceType, int color, int square);

/* TODO Stuff
 */
void undoMove(Board* board, Move move);

/* TODO Stuff
 */
uint64_t genAllAttackMap(Board* board, int color);

/* TODO Stuff
 */
int loadFen(Board* board, char* fen);

/* TODO Stuff
 */
void printFen(Board* board);

#endif /* end of include guard: BOARD_H */
