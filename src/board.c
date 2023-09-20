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

/**
 * Pseudo rotate a bitboard 45 degree clockwise.
 * Main Diagonal is mapped to 1st rank
 * @param x any bitboard
 * @return bitboard x rotated
 */
uint64_t pseudoRotate45Clockwise (uint64_t x)
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

/*
 * Populates the array moves with legal moves
 * and returns the number of legal moves.
 * Use MAX_MOVES_PER_POSITION as the max size
 * for moves
 */
int8_t genAllLegalMoves(Board *board, Move *moves)
{
    return 0;
}

/*
 * Updates the board based on the data provided in
 * move. Assumes the move is legal
 * Idea: Maybe return a move that can undo the current
 * move utilizing the 5 unused bits to give info about undo.
 * This could save on memory by storing a move instead of
 * a whole board in minMax()
 */
void boardMove(Board *board, Move move)
{
    /* Remove enemy piece if possible */
    int i;
    for (i = 0; i < 12; ++i)
        board->pieces[i] &= ~mgetdst(move);

    /* Remove src piece */
    board->pieces[mgetpiece(move) + mgetcol(move)] ^= mgetsrc(move);

    /* Add dst piece */
    board->pieces[mgetpiece(move) + mgetcol(move)] ^= mgetdst(move);

    /* Swap color */
    board->info ^= 0x01; 

    /* Update castling */
    int src_color = mgetcol(move);
    if (mgetpiece(move) == KING)
    {
        if (src_color == WHITE)
            board->info &= 0xffe7;
        else if (src_color == BLACK)
            board->info &= 0xfff9;
    }
    if (!(board->pieces[WHITE + ROOK] & A1))
        board->info &= ~(0x1 << 4);
    if (!(board->pieces[WHITE + ROOK] & A8))
        board->info &= ~(0x1 << 3);
    if (!(board->pieces[BLACK + ROOK] & H1))
        board->info &= ~(0x1 << 2);
    if (!(board->pieces[BLACK + ROOK] & H8))
        board->info &= ~(0x1 << 1);

    /* Update en passant */

    return;
}
