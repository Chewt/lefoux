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

uint64_t magicLookupBishop(uint64_t occupancy, enumIndexSquare square)
{
    occupancy &= magicBishop[square].mask;
    occupancy *= magicBishop[square].magic;
    occupancy >>= 64 - getNumBits(magicBishop[square].mask);
    return magicBishopAttacks[square][occupancy];
}

uint64_t magicLookupRook(uint64_t occupancy, enumIndexSquare square)
{
    occupancy &= magicRook[square].mask;
    occupancy *= magicRook[square].magic;
    occupancy >>= 64 - getNumBits(magicRook[square].mask);
    return magicRookAttacks[square][occupancy];
}

void printBitboard(uint64_t bb)
{
    for (int rank=7; rank >= 0; rank--)
    {
        printf("%c  ", '1' + rank);
        for (int file=0; file < 8; file++)
        {
            int bit = (bb >> (rank*8 + file)) & 1;
            printf("%d ", bit);
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h\n\n");
}

void printBytesBinary(int num_bytes, uint64_t data)
{
    int i, j;
    for (i = num_bytes - 1; i >= 0; --i)
    {
        for (j = 7; j >= 0; --j)
        {
            printf("%d", (data & (0x1 << ((i * 8) + j))) > 0);
            if (j == 4 || j == 0)
                printf(" ");
        }
    }
}

/*
 * Take in the info portion of a board struct and prints details to the screen
 */
void printBoardInfo(uint16_t info)
{
    printf("raw info: 0x%04x, ", info);
    printBytesBinary(2, info);
    printf("\n");
    printf("En passant: %c%c(%u)\nCastling: %c%c%c%c\nColor: %c\n",
            bgetenp(info) % 8 + 'A', bgetenp(info) / 8 + '1', bgetenp(info),
            (bgetcas(info) & 0x8) ? 'Q' : '-',
            (bgetcas(info) & 0x4) ? 'K' : '-',
            (bgetcas(info) & 0x2) ? 'q' : '-',
            (bgetcas(info) & 0x1) ? 'k' : '-',
            (bgetcol(info) & 0x1) ? 'B' : 'W');
}

/*
 * Populates the array moves with legal moves
 * and returns the number of legal moves.
 * Use MAX_MOVES_PER_POSITION as the max size
 * for moves
 */
int8_t genAllLegalMoves(Board *board, Move *moves)
{
    uint64_t piece;
    uint64_t attacks;
    uint64_t passiveMoves;
    int file;
    enumIndexSquare square;
    int i;
    int movecount = 0;
    int color_to_move = (bgetcol(board->info)) ? BLACK : WHITE;
    uint64_t friends = 0;
    uint64_t foes = 0;
    for (i = 0; i < 6; ++i)
    {
        foes |= board->pieces[i + (color_to_move ^ BLACK)];
        friends |= board->pieces[i + color_to_move];
    }
    for (int pieceType = 0; pieceType < 6; ++pieceType)
    {
        uint64_t pieces = board->pieces[pieceType + color_to_move];
        while ((piece = pieces & -pieces)) {
            uint64_t bitmap = 0;
            square = bitScanForward(piece);
            switch(pieceType) {
                case PAWN:
                    attacks = 0;
                    if (color_to_move == WHITE)
                    {
                        attacks |= square << 9 & ~AFILE;
                        attacks |= square << 7 & ~HFILE;
                    }
                    else
                    {
                        attacks |= square >> 9 & ~AFILE;
                        attacks |= square >> 7 & ~HFILE;
                    }
                    attacks &= foes | (0x1UL << bgetenp(board->info));
                    passiveMoves = 0;
                    if (color_to_move == WHITE)
                        passiveMoves |= 0x1UL << (square + 8);
                    else
                        passiveMoves |= 0x1UL << (square - 8);
                    if ((square / 8) == 1 && color_to_move == WHITE)
                        passiveMoves |= 0x1UL << (square + 16);
                    else if ((square / 8) == 6 && color_to_move == BLACK)
                        passiveMoves |= 0x1UL << (square - 16);
                    bitmap |= attacks ^ (attacks & friends);
                    bitmap |= passiveMoves ^ (passiveMoves & friends) 
                              ^ (passiveMoves & foes);
                    break;

                case KNIGHT:
                    file = square % 8 - C3 % 8;
                    attacks = NMOV;

                    if (square < IC3)
                        attacks >>= IC3 - square;
                    if (square > IC3)
                        attacks <<= square - IC3;

                    if (file == 0 || file == 1)
                        attacks &= ~(HFILE | FILELIST[6]);
                    if (file == 6 || file == 7)
                        attacks &= ~(AFILE | FILELIST[1]);
                    bitmap |= attacks ^ (attacks & friends);
                    break;

                case BISHOP:
                    attacks = magicLookupBishop((friends | foes), square);
                    bitmap |= attacks ^ (attacks & friends);
                    break;

                case ROOK:
                    attacks = magicLookupRook((friends | foes), square);
                    bitmap |= attacks ^ (attacks & friends);
                    break;

                case QUEEN:
                    attacks = magicLookupRook((friends | foes), square);
                    bitmap |= attacks ^ (attacks & friends);
                    attacks = magicLookupBishop((friends | foes), square);
                    bitmap |= attacks ^ (attacks & friends);
                    break;

                case KING:
                    file = square % 8 - B2 % 8;
                    attacks = KMOV;

                    if (square < IB2)
                        attacks >>= IB2 - square;
                    if (square > IB2)
                        attacks <<= square - IB2;

                    if (file == 0 || file == 1)
                        attacks &= ~(HFILE);
                    if (file == 6 || file == 7)
                        attacks &= ~(AFILE);
                    bitmap |= attacks ^ (attacks & friends);
                    break;
            }
            uint64_t dst;
            while ((dst = bitmap & -bitmap))
            {
                moves[movecount++] = (square << 13) 
                    | (bitScanForward(dst) << 7)
                    | (pieceType << 4)
                    | bgetcol(board->info);
                // Iterate to next move
                bitmap ^= dst;
            }
            // Iterate to next piece
            pieces ^= piece;
        }
    }
    return movecount;
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
        board->pieces[i] &= ~mgetdstbb(move);

    /* Remove src piece */
    board->pieces[mgetpiece(move) + mgetcol(move)] ^= mgetsrcbb(move);

    /* Add dst piece */
    board->pieces[mgetpiece(move) + mgetcol(move)] ^= mgetdstbb(move);

    /* Swap color */
    board->info ^= 0x1; 

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
    if (!(board->pieces[WHITE + ROOK] & H1))
        board->info &= ~(0x1 << 3);
    if (!(board->pieces[BLACK + ROOK] & A8))
        board->info &= ~(0x1 << 2);
    if (!(board->pieces[BLACK + ROOK] & H8))
        board->info &= ~(0x1 << 1);

    /* Update en passant */
    board->info &= ~(0x3f << 5);
    if (mgetpiece(move) == PAWN)
    {
        if ((mgetsrcbb(move) & RANK[1]) && (mgetdstbb(move) & RANK[3]))
            board->info = ((mgetsrc(move) + 8) << 5) | (0x1f & board->info);
        else if ((mgetsrcbb(move) & RANK[6]) && (mgetdstbb(move) & RANK[4]))
            board->info = ((mgetsrc(move) - 8) << 5) | (0x1f & board->info);
    }

    return;
}

Board getDefaultBoard()
{
    Board b;
    b.pieces[WHITE + PAWN]   = RANK[1];
    b.pieces[WHITE + KNIGHT] = B1 | G1;
    b.pieces[WHITE + BISHOP] = C1 | F1;
    b.pieces[WHITE + ROOK]   = A1 | H1;
    b.pieces[WHITE + QUEEN]  = D1;
    b.pieces[WHITE + KING]   = E1;
    b.pieces[BLACK + PAWN]   = RANK[6];
    b.pieces[BLACK + KNIGHT] = B8 | G8;
    b.pieces[BLACK + BISHOP] = C8 | F8;
    b.pieces[BLACK + ROOK]   = A8 | H8;
    b.pieces[BLACK + QUEEN]  = D8;
    b.pieces[BLACK + KING]   = E8;
    b.info = (0xf << 1) | 0x0;

    return b;
}

void printBoard(Board *board) 
{
    printBoardInfo(board->info);
    for (int rank=7; rank >= 0; rank--)
    {
        printf("%d  ", rank+1);
        for (int file=0; file < 8; file++)
        {
            int squareIdx = rank*8 + file;
            uint64_t square = 1UL << squareIdx;
            int pieceType;
            for (pieceType=0; pieceType<12; pieceType++)
                if (board->pieces[pieceType] & square) break;
            switch (pieceType) 
            {
                case PAWN + WHITE:
                    printf("p ");
                    break;
                case KNIGHT + WHITE:
                    printf("n ");
                    break;
                case BISHOP + WHITE:
                    printf("b ");
                    break;
                case ROOK + WHITE:
                    printf("r ");
                    break;
                case QUEEN + WHITE:
                    printf("q ");
                    break;
                case KING + WHITE:
                    printf("k ");
                    break;
                case PAWN + BLACK:
                    printf("P ");
                    break;
                case KNIGHT + BLACK:
                    printf("N ");
                    break;
                case BISHOP + BLACK:
                    printf("B ");
                    break;
                case ROOK + BLACK:
                    printf("R ");
                    break;
                case QUEEN + BLACK:
                    printf("Q ");
                    break;
                case KING + BLACK:
                    printf("K ");
                    break;
                default:
                    printf("_ ");
                    break;
            }
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h \n");
}

void printMove(Move move)
{
    printf("raw move: 0x%04x, ", move);
    printBytesBinary(4, move);
    printf("\n");
    printf("Color: ");
    if (mgetcol(move)) printf("Black\n"); else printf("White\n");
    printf("Promotion: ");
    switch (mgetprom(move)) 
    {
        case PAWN:
        case _PAWN:
            printf("Pawn\n");
            break;
        case KNIGHT:
        case _KNIGHT:
            printf("Knight\n");
            break;
        case BISHOP:
        case _BISHOP:
            printf("Bishop\n");
            break;
        case ROOK:
        case _ROOK:
            printf("Rook\n");
            break;
        case QUEEN:
        case _QUEEN:
            printf("Queen\n");
            break;
        case KING:
        case _KING:
            printf("King\n");
            break;
    }
    printf("Source: %c%c\n", mgetsrc(move) % 8 + 'A', mgetsrc(move) / 8 + '1');
    printf("Destination: %c%c\n", mgetdst(move) % 8 + 'A', mgetdst(move) / 8 + '1');
    printf("Weight: %d\n", mgetweight(move));
}