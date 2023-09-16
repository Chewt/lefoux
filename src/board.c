#include <stdint.h>

#include "board.h"
#include "bitHelpers.h"

/* Constants for piece attacks */
const uint64_t RDIAG = 0x0102040810204080ULL;
const uint64_t LDIAG = 0x8040201008040201ULL;
const uint64_t VERT  = 0x0101010101010101ULL;
const uint64_t HORZ  = 0x00000000000000FFULL;
const uint64_t NMOV  = 0x0000000A1100110AULL;   // Nc3
const uint64_t KMOV  = 0x0000000000070507ULL;   // Kb2
const uint64_t PATTK = 0x0000000000050005ULL;   // b2

const uint64_t FILELIST[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL
};

const uint64_t RANK[8] = {
    0xFFULL,
    0xFF00ULL,
    0xFF0000ULL,
    0xFF000000ULL,
    0xFF00000000ULL,
    0xFF0000000000ULL,
    0xFF000000000000ULL,
    0xFF00000000000000ULL
};

const Magic magicBishop[64] = {0};
const Magic magicRook[64] = {0};
const uint64_t magicRookAttacks[64][4096] = {0};
const uint64_t magicBishopAttacks[64][512] = {0};

/**
 * Pseudo rotate a bitboard 45 degree clockwise.
 * Main Diagonal is mapped to 1st rank
 * @param x any bitboard
 * @return bitboard x rotated
 */
uint64_t pseudoRotate45clockwise (uint64_t x)
{
   const uint64_t k1 = 0xAAAAAAAAAAAAAAAAULL;
   const uint64_t k2 = 0xCCCCCCCCCCCCCCCCULL;
   const uint64_t k4 = 0xF0F0F0F0F0F0F0F0ULL;
   x ^= k1 & (x ^ shiftWrapRight(x,  8));
   x ^= k2 & (x ^ shiftWrapRight(x, 16));
   x ^= k4 & (x ^ shiftWrapRight(x, 32));
   return x;
}

/**
 * Pseudo rotate a bitboard 45 degree anti clockwise.
 * Main AntiDiagonal is mapped to 1st rank
 * @param x any bitboard
 * @return bitboard x rotated
 */
uint64_t pseudoRotate45antiClockwise (uint64_t x)
{
   const uint64_t k1 = 0x5555555555555555ULL;
   const uint64_t k2 = 0x3333333333333333ULL;
   const uint64_t k4 = 0x0F0F0F0F0F0F0F0FULL;
   x ^= k1 & (x ^ shiftWrapRight(x,  8));
   x ^= k2 & (x ^ shiftWrapRight(x, 16));
   x ^= k4 & (x ^ shiftWrapRight(x, 32));
   return x;
}

uint64_t magicLookupBishop(uint64_t occupancy, enumSquare square)
{
    occupancy &= magicBishop[square].mask;
    occupancy *= magicBishop[square].magic;
    occupancy >>= 64-9;
    return magicBishopAttacks[square][occupancy];
}

uint64_t magicLookupRook(uint64_t occupancy, enumSquare square)
{
    occupancy &= magicRook[square].mask;
    occupancy *= magicRook[square].magic;
    occupancy >>= 64-12;
    return magicRookAttacks[square][occupancy];
}

// Returns an attack bitmap for the piece
// type is the type of piece
// pieces is a bitmap of all of that type of piece
uint64_t getMoves(enumPiece type, uint64_t pieces, uint64_t friends, uint64_t foes)
{
    uint64_t bitmap;
    uint64_t piece;
    uint64_t attacks;
    int rank;
    int file;
    int i;
    enumSquare square;
    // Isolate a piece to process, loop while there is a piece
    while (piece &= -pieces) {
        switch(type) {
          case WHITE_PAWN:
          case BLACK_PAWN:
            return bitmap;

          case WHITE_KNIGHT:
          case BLACK_KNIGHT:
            rank = square / 8 - C3 / 8;
            file = square % 8 - C3 % 8;
            attacks = 1 << C3;
            for (i=C3/8; i<rank; i++){
                attacks <<= 8;
            }
            for (i=C3/8; i>rank; i--){
                attacks >>= 8;
            }
            // ~HFILE and ~AFILE prevent wrap-around
            for (i=C3%8; i<file; i++){
                attacks = (attacks << 1) & ~HFILE;
            }
            for (i=C3%8; i>file; i--){
                attacks = (attacks >> 1) & ~AFILE;
            }
            bitmap |= attacks ^ (attacks & friends);
            return bitmap;

          case WHITE_BISHOP:
          case BLACK_BISHOP:
            square = bitScanForward(piece);
            attacks = magicLookupBishop((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            break;

          case WHITE_ROOK:
          case BLACK_ROOK:
            square = bitScanForward(piece);
            attacks = magicLookupRook((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            break;

          case WHITE_QUEEN:
          case BLACK_QUEEN:
            square = bitScanForward(piece);
            attacks = magicLookupRook((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            attacks = magicLookupBishop((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            break;

          case WHITE_KING:
          case BLACK_KING:
            rank = square / 8 - B2 / 8;
            file = square % 8 - B2 % 8;
            attacks = 1 << B2;
            for (i=B2/8; i<rank; i++){
                attacks <<= 8;
            }
            for (i=B2/8; i>rank; i--){
                attacks >>= 8;
            }
            // ~HFILE and ~AFILE prevent wrap-around
            for (i=B2%8; i<file; i++){
                attacks = (attacks << 1) & ~HFILE;
            }
            for (i=B2%8; i>file; i--){
                attacks = (attacks >> 1) & ~AFILE;
            }
            bitmap |= attacks ^ (attacks & friends);
            break;

          defualt:
            break;
        }
        // Mask away piece that was already processed
        pieces ^= piece;
    }
    return bitmap;
}
