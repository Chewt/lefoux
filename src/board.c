#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "bitHelpers.h"
#include "magic.h"

/* Constants for piece attacks */
const uint64_t RDIAG = 0x0102040810204080UL;
const uint64_t LDIAG = 0x8040201008040201UL;
const uint64_t VERT  = 0x0101010101010101UL;
const uint64_t HORZ  = 0x00000000000000FFUL;
const uint64_t NMOV  = 0x0000000A1100110AUL;   // Nc3
const uint64_t KMOV  = 0x0000000000070507UL;   // Kb2
const uint64_t PATTK = 0x0000000000050005UL;   // b2

const uint64_t FILELIST[8] = {
    0x0101010101010101UL,
    0x0202020202020202UL,
    0x0404040404040404UL,
    0x0808080808080808UL,
    0x1010101010101010UL,
    0x2020202020202020UL,
    0x4040404040404040UL,
    0x8080808080808080UL
};

const uint64_t RANK[8] = {
    0xFFUL,
    0xFF00UL,
    0xFF0000UL,
    0xFF000000UL,
    0xFF00000000UL,
    0xFF0000000000UL,
    0xFF000000000000UL,
    0xFF00000000000000UL
};

/*
const Magic magicRook[64] = {0};
const uint64_t magicRookAttacks[64][4096] = {0};
const Magic magicBishop[64] = {0};
const uint64_t magicBishopAttacks[64][512] = {0};
*/

/**
 * Pseudo rotate a bitboard 45 degree clockwise.
 * Main Diagonal is mapped to 1st rank
 * @param x any bitboard
 * @return bitboard x rotated
 */
uint64_t pseudoRotate45clockwise (uint64_t x)
{
   const uint64_t k1 = 0xAAAAAAAAAAAAAAAAUL;
   const uint64_t k2 = 0xCCCCCCCCCCCCCCCCUL;
   const uint64_t k4 = 0xF0F0F0F0F0F0F0F0UL;
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
   const uint64_t k1 = 0x5555555555555555UL;
   const uint64_t k2 = 0x3333333333333333UL;
   const uint64_t k4 = 0x0F0F0F0F0F0F0F0FUL;
   x ^= k1 & (x ^ shiftWrapRight(x,  8));
   x ^= k2 & (x ^ shiftWrapRight(x, 16));
   x ^= k4 & (x ^ shiftWrapRight(x, 32));
   return x;
}

uint64_t magicLookupBishop(uint64_t occupancy, enumSquare square)
{
    occupancy &= magicBishop[square].mask;
    occupancy *= magicBishop[square].magic;
    occupancy >>= 64 - getNumBits(magicBishop[square].mask);
    return magicBishopAttacks[square][occupancy];
}

uint64_t magicLookupRook(uint64_t occupancy, enumSquare square)
{
    occupancy &= magicRook[square].mask;
    occupancy *= magicRook[square].magic;
    occupancy >>= 64 - getNumBits(magicRook[square].mask);
    return magicRookAttacks[square][occupancy];
}

// Returns an attack bitmap for the piece
// type is the type of piece
// pieces is a bitmap of all of that type of piece
uint64_t getMoves(enumPiece type, uint64_t pieces, uint64_t friends, uint64_t foes)
{
    uint64_t bitmap = 0;
    uint64_t piece;
    uint64_t attacks;
    int rank;
    int file;
    int i;
    enumSquare square;
    // Isolate a piece to process, loop while there is a piece
    while ((piece = pieces & -pieces)) {
        square = bitScanForward(piece);
        switch(type) {
          case PAWN + WHITE:
          case PAWN + BLACK:
            break;

          case KNIGHT + WHITE:
          case KNIGHT + BLACK:
            rank = square / 8 - C3 / 8;
            file = square % 8 - C3 % 8;
            attacks = NMOV;
            for (i=C3/8; i<rank; i++)
                attacks <<= 8;
            for (i=C3/8; i>rank; i--)
                attacks >>= 8;
            // ~HFILE and ~AFILE prevent wrap-around
            for (i=C3%8; i<file; i++)
                attacks = (attacks << 1) & ~HFILE;
            for (i=C3%8; i>file; i--)
                attacks = (attacks >> 1) & ~AFILE;
            bitmap |= attacks ^ (attacks & friends);
            break;

          case BISHOP + WHITE:
          case BISHOP + BLACK:
            attacks = magicLookupBishop((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            break;

          case ROOK + WHITE:
          case ROOK + BLACK:
            attacks = magicLookupRook((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            break;

          case QUEEN + WHITE:
          case QUEEN + BLACK:
            attacks = magicLookupRook((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            attacks = magicLookupBishop((friends | foes), square);
            bitmap |= attacks ^ (attacks & friends);
            break;

          case KING + WHITE:
          case KING + BLACK:
            rank = square / 8 - B2 / 8;
            file = square % 8 - B2 % 8;
            attacks = KMOV;
            for (i=B2/8; i<rank; i++)
                attacks <<= 8;
            for (i=B2/8; i>rank; i--)
                attacks >>= 8;
            // ~HFILE and ~AFILE prevent wrap-around
            for (i=B2%8; i<file; i++)
                attacks = (attacks << 1) & ~HFILE;
            for (i=B2%8; i>file; i--)
                attacks = (attacks >> 1) & ~AFILE;
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

void printBitboard(uint64_t bb)
{
    for (int rank=7; rank >= 0; rank--)
    {
        printf("%c  ", 'a' + rank);
        for (int file=0; file < 8; file++)
        {
            int bit = (bb >> (rank*8 + file)) & 1;
            printf("%d ", bit);
        }
        printf("\n");
    }
    printf("\n   1 2 3 4 5 6 7 8\n\n");
}
