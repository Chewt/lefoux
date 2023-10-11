#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
            fprintf(stderr, "%d", (data & (0x1 << ((i * 8) + j))) > 0);
            if (j == 4 || j == 0)
                fprintf(stderr, " ");
        }
    }
}

/*
 * Take in the info portion of a board struct and prints details to the screen
 */
void printBoardInfo(uint16_t info)
{
    fprintf(stderr, "raw info: 0x%04x, ", info);
    printBytesBinary(2, info);
    fprintf(stderr, "\n");
    char enpStr[2][5] = {
        "None",
        {bgetenpsquare(info) % 8 + 'A', bgetenpsquare(info) / 8 + '1', '\0','\0','\0'}
    };
    fprintf(stderr, "En passant: %s(%u)\nCastling: %c%c%c%c\nColor: %c\n",
            bgetenp(info) ? enpStr[1] : enpStr[0], bgetenpsquare(info),
            (bgetcas(info) & 0x8) ? 'Q' : '-',
            (bgetcas(info) & 0x4) ? 'K' : '-',
            (bgetcas(info) & 0x2) ? 'q' : '-',
            (bgetcas(info) & 0x1) ? 'k' : '-',
            (bgetcol(info) & 0x1) ? 'B' : 'W');
}

uint64_t genPassiveKingMoves(Board* board, int color)
{
    uint64_t passive_moves = 0;
    uint64_t friends = 0;
    uint64_t foes = 0;
    int i;
    for (i = 0; i < 6; ++i)
    {
        foes    |= board->pieces[i + (color ^ BLACK)];
        friends |= board->pieces[i + color];
    }
    if (color == WHITE)
    {
        if ((bgetcas(board->info) & 0x8) && !((B1 | C1 | D1) & (friends | foes)))
            passive_moves |= C1;
        if ((bgetcas(board->info) & 0x4) && !((F1 | G1) & (friends | foes)))
            passive_moves |= G1;
    }
    else
    {
        if ((bgetcas(board->info) & 0x2) && !((B8 | C8 | D8) & (friends | foes)))
            passive_moves |= C8;
        if ((bgetcas(board->info) & 0x1) && !((F8 | G8) & (friends | foes)))
            passive_moves |= G8;
    }
    return passive_moves;
}

uint64_t genPassivePawnMoves(Board* board, int color, int square)
{
    int i;
    uint64_t friends = 0;
    uint64_t foes = 0;
    for (i = 0; i < 6; ++i)
    {
        foes    |= board->pieces[i + (color ^ BLACK)];
        friends |= board->pieces[i + color];
    }
    uint64_t passiveMoves = 0;
    if (color == WHITE)
        passiveMoves |= 0x1UL << (square + 8);
    else
        passiveMoves |= 0x1UL << (square - 8);

    passiveMoves ^= (passiveMoves & (friends | foes));
    if (((square / 8) == 1) && (color == WHITE))
        passiveMoves |= passiveMoves << 8;
    else if (((square / 8) == 6) && (color == BLACK))
        passiveMoves |= passiveMoves >> 8;
    return passiveMoves ^ (passiveMoves & (friends | foes));
}

uint64_t genPieceAttackMap(Board* board, int pieceType, int color, int square)
{
    uint64_t attacks = 0;
    uint64_t bitmap = 0;
    int color_to_move = color;
    int file;
    int i;
    uint64_t friends = 0;
    uint64_t foes = 0;
    for (i = 0; i < 6; ++i)
    {
        foes    |= board->pieces[i + (color_to_move ^ BLACK)];
        friends |= board->pieces[i + color_to_move];
    }
    switch(pieceType) {
        case PAWN:
            file = square % 8 - C3 % 8;
            attacks = PATTK;
            // Mask attacks depending on color
            if (color_to_move == WHITE)
                attacks &= RANK[2];
            else
                attacks &= RANK[0];

            if (square < IB2)
                attacks >>= IB2 - square;
            if (square > IB2)
                attacks <<= square - IB2;

            if (file == 0)
                attacks &= ~(HFILE);
            if (file == 7)
                attacks &= ~(AFILE);

            bitmap |= attacks ^ (attacks & friends);
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
            attacks  = magicLookupRook((friends | foes), square);
            attacks |= magicLookupBishop((friends | foes), square);
            bitmap  |= attacks ^ (attacks & friends);
            break;

        case KING:
            file = square % 8 - B2 % 8;
            attacks = KMOV;

            if (square < IB2)
                attacks >>= IB2 - square;
            if (square > IB2)
                attacks <<= square - IB2;

            if (file == 0)
                attacks &= ~(HFILE);
            if (file == 7)
                attacks &= ~(AFILE);
            bitmap |= attacks ^ (attacks & friends);
            break;
    }
    return bitmap;
}

uint64_t genAllAttackMap(Board* board, int color)
{
    uint64_t piece;
    enumIndexSquare square;
    int color_to_move = color;
    uint64_t bitmap = 0;
    for (int pieceType = 0; pieceType < 6; ++pieceType)
    {
        uint64_t pieces = board->pieces[pieceType + color_to_move];
        while ((piece = pieces & -pieces)) {
            square = bitScanForward(piece);

            // GET ATTACK BITMAP FOR CURRENT PIECE
            bitmap |= genPieceAttackMap(board, pieceType, color, square);

            // Iterate to next piece
            pieces ^= piece;
        }
    }
    return bitmap;

}

void undoMove(Board* board, Move move)
{
    // Restore boardinfo
    board->info = mgetprevinfo(move);
    int move_color = (mgetcol(move)) ? BLACK : WHITE;

    // Undo move
    board->pieces[move_color + mgetpiece(move)] ^= (mgetsrcbb(move)
                                                  | mgetdstbb(move));
    /* Move rooks when castling */
    /* White Kingside */
    if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE1)
                                  && (mgetdst(move) == IG1))
        board->pieces[ROOK + WHITE] ^= H1 | F1;
    /* White Queenside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE1)
                                       && (mgetdst(move) == IC1))
        board->pieces[ROOK + WHITE] ^= A1 | D1;
    /* Black Kingside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE8)
                                       && (mgetdst(move) == IG8))
        board->pieces[ROOK + BLACK] ^= H8 | F8;
    /* Black Queenside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE8)
                                       && (mgetdst(move) == IC8))
        board->pieces[ROOK + BLACK] ^= A8 | D8;

    // restore taken piece
    if (mgettaken(move) != 0x7) {
        if (bgetenp(board->info) && bgetenpsquare(board->info) == mgetdst(move)
            && mgettaken(move) == PAWN)
            board->pieces[(move_color^BLACK) + mgettaken(move)]
                ^= move_color == WHITE ?
                mgetdstbb(move) >> 8 : mgetdstbb(move) << 8;
        else
            board->pieces[(move_color^BLACK) + mgettaken(move)] ^=
                mgetdstbb(move);
    }
}

int checkIfLegal(Board* board, Move* move)
{
    *move = boardMove(board, *move);
    uint16_t prev_info = mgetprevinfo(*move);
    int color_to_move = (bgetcol(board->info)) ? BLACK : WHITE;
    // Generate bitmap of all attacks from the opposite color
    uint64_t all_attacks = 0UL;
    for (int pieceType = 0; pieceType < 6; ++pieceType)
    {
        uint64_t piece;
        uint64_t pieces = board->pieces[pieceType + color_to_move];
        while ((piece = pieces & -pieces)) {
            int square = bitScanForward(piece);

            // GET ATTACK BITMAP FOR CURRENT PIECE
            all_attacks |=
                genPieceAttackMap(board, pieceType, color_to_move, square);

            // Iterate to next piece
            pieces ^= piece;
        }
    }
    int is_legal = 1;
    // Check if the move was a castle move, if so add square to check against
    // attack bitmap
    uint64_t castle_square = 0;
    if (mgetpiece(*move) == KING)
    {
        if (mgetcol(*move) == _WHITE)
        {
            if ((bgetcas(prev_info) & 0x8) && (mgetdst(*move) == IC1))
                castle_square |= D1 | E1;
            else if ((bgetcas(prev_info) & 0x4) && (mgetdst(*move) == IG1))
                castle_square |= F1 | E1;
        }
        else if (mgetcol(*move) == _BLACK)
        {
            if ((bgetcas(prev_info) & 0x2) && (mgetdst(*move) == IC8))
                castle_square |= D8 | E8;
            else if ((bgetcas(prev_info) & 0x1) && (mgetdst(*move) == IG8))
                castle_square |= F8 | E8;
        }
    }

    // If king is under attack or the castle square was under attack,
    // move was not legal
    if (all_attacks &
       (board->pieces[(color_to_move ^ BLACK) + KING] | castle_square))
        is_legal = 0;
    undoMove(board, *move);
    return is_legal;
}

/*
 * Populates the array moves with legal moves that works with undoMove()
 * and returns the number of legal moves. Use MAX_MOVES_PER_POSITION
 * as the max size for moves
 */
int8_t genAllLegalMoves(Board *board, Move *moves)
{
    uint64_t piece;
    enumIndexSquare square;
    int movecount = 0;
    int color_to_move = (bgetcol(board->info)) ? BLACK : WHITE;
    uint64_t foes = 0;
    for (int i = 0; i < 6; ++i)
        foes    |= board->pieces[i + (color_to_move ^ BLACK)];
    for (int pieceType = PAWN; pieceType <= KING; ++pieceType)
    {
        uint64_t pieces = board->pieces[pieceType + color_to_move];
        while ((piece = pieces & -pieces)) {
            square = bitScanForward(piece);
            uint64_t dst;

            // GET ATTACK BITMAP FOR CURRENT PIECE
            uint64_t bitmap =
                genPieceAttackMap(board, pieceType, color_to_move, square);
            if (pieceType == PAWN)
            {
                // Add en passant
                bitmap &= foes | (bgetenp(board->info) ?
                              0x1UL << bgetenpsquare(board->info) : 0);
                bitmap |= genPassivePawnMoves(board, color_to_move, square);
            }
            if (pieceType == KING)
                bitmap |= genPassiveKingMoves(board, color_to_move);

            while ((dst = bitmap & -bitmap))
            {
                moves[movecount++] = (square << 13)
                    | (bitScanForward(dst) << 7)
                    | (pieceType << 4)
                    | bgetcol(board->info);

                // Check if move is legal, if not decrement movecount
                if (!checkIfLegal(board, (moves + movecount - 1)))
                {
                    movecount--;
                }

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
 * move. Assumes the move is legal. Returns a move
 * that can be used with undoMove to undo the move
 */
Move boardMove(Board *board, Move move)
{
    /* Remove enemy piece if possible */
    int enemy_piece = 7;
    uint16_t prev_info = board->info;
    int i;
    int enemy_color = (bgetcol(board->info) == _BLACK) ? WHITE : BLACK;
    uint64_t enemy_piece_dstbb = mgetdstbb(move);
    if ((mgetpiece(move) == PAWN) && bgetenp(board->info) &&
        (mgetdst(move) == bgetenpsquare(board->info)))
    {
        if (enemy_color == BLACK)
            enemy_piece_dstbb >>= 8;
        else
            enemy_piece_dstbb <<= 8;
    }
    for (i = 0; i < 6; ++i)
    {
        if (board->pieces[i + enemy_color] & enemy_piece_dstbb)
            enemy_piece = i;
        board->pieces[i + enemy_color] &= ~enemy_piece_dstbb;
    }
    prev_info = (prev_info << 3) | (enemy_piece & 0x7);

    /* Remove src piece */
    board->pieces[mgetpiece(move) + (enemy_color ^ BLACK)] ^= mgetsrcbb(move);

    /* Add dst piece */
    board->pieces[mgetpiece(move) + (enemy_color ^ BLACK)] ^= mgetdstbb(move);

    /* Move rooks when castling */
    /* White Kingside */
    if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE1)
                                  && (mgetdst(move) == IG1))
        board->pieces[ROOK + WHITE] ^= H1 | F1;
    /* White Queenside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE1)
                                       && (mgetdst(move) == IC1))
        board->pieces[ROOK + WHITE] ^= A1 | D1;
    /* Black Kingside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE8)
                                       && (mgetdst(move) == IG8))
        board->pieces[ROOK + BLACK] ^= H8 | F8;
    /* Black Queenside */
    else if ((mgetpiece(move) == KING) && (mgetsrc(move) == IE8)
                                       && (mgetdst(move) == IC8))
        board->pieces[ROOK + BLACK] ^= A8 | D8;

    /* Swap color */
    board->info ^= 0x1;

    /* Update castling */
    int src_color = (mgetcol(move)) ? BLACK : WHITE;
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
    // Unset any en passant info that was there before
    board->info &= ~(0xf << 5);
    if (mgetpiece(move) == PAWN)
    {
        // If move was a pawn moving two spaces, set the file that the en
        // passant move is on
        if ((mgetsrcbb(move) & RANK[1]) && (mgetdstbb(move) & RANK[3]))
        {
            board->info |= 0x8 << 5;
            board->info |= ((mgetsrc(move) % 8) << 5);
        }
        else if ((mgetsrcbb(move) & RANK[6]) && (mgetdstbb(move) & RANK[4]))
        {
            board->info |= 0x8 << 5;
            board->info |= ((mgetsrc(move) % 8) << 5);
        }
    }

    return (prev_info << 19) | (move & 0x7ffff);
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
                    printf("P ");
                    break;
                case KNIGHT + WHITE:
                    printf("N ");
                    break;
                case BISHOP + WHITE:
                    printf("B ");
                    break;
                case ROOK + WHITE:
                    printf("R ");
                    break;
                case QUEEN + WHITE:
                    printf("Q ");
                    break;
                case KING + WHITE:
                    printf("K ");
                    break;
                case PAWN + BLACK:
                    printf("p ");
                    break;
                case KNIGHT + BLACK:
                    printf("n ");
                    break;
                case BISHOP + BLACK:
                    printf("b ");
                    break;
                case ROOK + BLACK:
                    printf("r ");
                    break;
                case QUEEN + BLACK:
                    printf("q ");
                    break;
                case KING + BLACK:
                    printf("k ");
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
    fprintf(stderr, "raw move: 0x%04x, ", move);
    printBytesBinary(4, move);
    fprintf(stderr, "\n");
    fprintf(stderr, "Color: ");
    if (mgetcol(move)) fprintf(stderr, "Black\n"); else fprintf(stderr, "White\n");
    fprintf(stderr, "Piece: ");
    switch (mgetpiece(move))
    {
        case PAWN:
        case _PAWN:
            fprintf(stderr, "Pawn\n");
            break;
        case KNIGHT:
        case _KNIGHT:
            fprintf(stderr, "Knight\n");
            break;
        case BISHOP:
        case _BISHOP:
            fprintf(stderr, "Bishop\n");
            break;
        case ROOK:
        case _ROOK:
            fprintf(stderr, "Rook\n");
            break;
        case QUEEN:
        case _QUEEN:
            fprintf(stderr, "Queen\n");
            break;
        case KING:
        case _KING:
            fprintf(stderr, "King\n");
            break;
    }
    fprintf(stderr, "Promotion: ");
    switch (mgetprom(move))
    {
        case PAWN:
        case _PAWN:
            fprintf(stderr, "None\n");
            break;
        case KNIGHT:
        case _KNIGHT:
            fprintf(stderr, "Knight\n");
            break;
        case BISHOP:
        case _BISHOP:
            fprintf(stderr, "Bishop\n");
            break;
        case ROOK:
        case _ROOK:
            fprintf(stderr, "Rook\n");
            break;
        case QUEEN:
        case _QUEEN:
            fprintf(stderr, "Queen\n");
            break;
        case KING:
        case _KING:
            fprintf(stderr, "King\n");
            break;
    }
    fprintf(stderr, "Source: %c%c(%d)\n", mgetsrc(move) % 8 + 'A',
           mgetsrc(move) / 8 + '1', mgetsrc(move));
    fprintf(stderr, "Destination: %c%c(%d)\n", mgetdst(move) % 8 + 'A',
           mgetdst(move) / 8 + '1', mgetdst(move));
    fprintf(stderr, "Weight: %d\n", mgetweight(move));
    fprintf(stderr, "Taken Piece: ");
    switch (mgettaken(move))
    {
        case PAWN:
            fprintf(stderr, "Pawn\n");
            break;
        case KNIGHT:
            fprintf(stderr, "Knight\n");
            break;
        case BISHOP:
            fprintf(stderr, "Bishop\n");
            break;
        case ROOK:
            fprintf(stderr, "Rook\n");
            break;
        case QUEEN:
            fprintf(stderr, "Queen\n");
            break;
        case KING:
            fprintf(stderr, "King\n");
            break;
        default:
            fprintf(stderr, "None\n");
    }
    fprintf(stderr, "Previous Board Info: ");
    printBoardInfo(mgetprevinfo(move));
}

void printMoveSAN(Move move)
{
    fprintf(stderr, "%c%c", mgetsrc(move) % 8 + 'a',
            mgetsrc(move) / 8 + '1');
    fprintf(stderr, "%c%c\n", mgetdst(move) % 8 + 'a',
            mgetdst(move) / 8 + '1');
}

int loadFen(Board* board, char* fen)
{
    memset(board, 0, sizeof(Board));
    char* fen_copy = malloc(strlen(fen) + 1);
    strcpy(fen_copy, fen);
    char* token = NULL;
    /* Piece positions */
    if (!(token = strtok(fen_copy, " "))) return 0;
    int i = 0;
    char curr_char = token[i];
    int rank = 7;
    int file = 0;
    while (curr_char)
    {
        if (curr_char == 'p')
            board->pieces[BLACK + PAWN] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'b')
            board->pieces[BLACK + BISHOP] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'n')
            board->pieces[BLACK + KNIGHT] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'r')
            board->pieces[BLACK + ROOK] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'q')
            board->pieces[BLACK + QUEEN] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'k')
            board->pieces[BLACK + KING] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'P')
            board->pieces[WHITE + PAWN] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'B')
            board->pieces[WHITE + BISHOP] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'N')
            board->pieces[WHITE + KNIGHT] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'R')
            board->pieces[WHITE + ROOK] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'Q')
            board->pieces[WHITE + QUEEN] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char == 'K')
            board->pieces[WHITE + KING] |= 0x1ULL << (rank * 8 + file++);
        else if (curr_char <= '9' && curr_char >= '0')
            file += curr_char - '0';
        else if (curr_char <= '/')
        {
            rank--;
            file = 0;
        }
        i++;
        curr_char = token[i];
    }

    /* Color to move */
    if (!(token = strtok(NULL, " "))) return 0;
    if (token[0] == 'w')
        board->info = _WHITE;
    else if (token[0] == 'b')
        board->info = _BLACK;

    /* Castling */
    if (!(token = strtok(NULL, " "))) return 0;
    i = 0;
    curr_char = token[i];
    while (curr_char)
    {
        if      (curr_char == 'Q')
            board->info |= 0x8 << 1;
        else if (curr_char == 'K')
            board->info |= 0x4 << 1;
        else if (curr_char == 'q')
            board->info |= 0x2 << 1;
        else if (curr_char == 'k')
            board->info |= 0x1 << 1;
        else if (curr_char == '-')
            board->info |= 0x0 << 1;
        curr_char = token[++i];
    }

    /* En passant */
    if (!(token = strtok(NULL, " "))) return 0;
    if (!strcmp(token, "-"))
        board->info |= 0x0UL << 5;
    else
        board->info |= (((token[0] - 'a') | 8)) << 5;

    /* Half move counter */
    if (!(token = strtok(NULL, " "))) return 0;
    /* Full move counter */
    if (!(token = strtok(NULL, " "))) return 0;
    return token - fen_copy;
}

void printFen(Board *board)
{
    /* Piece placement */
    char piece_chars[] = "PNBRQKpnbrqk";
    uint64_t piece_bb;
    uint64_t all_pieces = 0UL;
    for (int i=PAWN; i<=_KING; i++) {
        all_pieces |= board->pieces[i];
    }
    int skip_counter = 0;
    for (int rank=7; rank >= 0; rank--) {
        for (int file=0; file < 8; file++) {
            int sq_idx = rank*8 + file;
            if ((file == 0) && rank != 7) {
                if (skip_counter) fprintf(stderr, "%d", skip_counter);
                skip_counter = 0;
                fprintf(stderr, "/");
            }
            if (!(piece_bb = all_pieces & (1UL << sq_idx))) {
                skip_counter++;
                continue;
            }
            for (int i=PAWN; i<=_KING; i++) {
                if (board->pieces[i] & piece_bb) {
                    if (skip_counter) fprintf(stderr, "%d", skip_counter);
                    fprintf(stderr, "%c", piece_chars[i]);
                    skip_counter = 0;
                }
            }
        }
    }
    /* Active color */
    if (bgetcol(board->info) == _WHITE) fprintf(stderr, " w ");
    else fprintf(stderr, " b ");
    /* Castling */
    /*
    if (bgetcas(board->info) & 4) fprintf(stderr, "K");
    if (bgetcas(board->info) & 8) fprintf(stderr, "Q");
    if (bgetcas(board->info) & 1) fprintf(stderr, "k");
    if (bgetcas(board->info) & 2) fprintf(stderr, "q");
    if (bgetcas(board->info) == 0) fprintf(stderr, "-");
    */
    if (1) fprintf(stderr, "-");
    /* En passant */
    if (bgetenp(board->info)) {
        int sq_idx = bgetenpsquare(board->info);
        fprintf(stderr, " %c%c ", 'a' + (sq_idx % 8), '1' + (sq_idx / 8));
    } else fprintf(stderr, " - ");
    /* Since Lefoux doesn't count number of moves, we'll fill these fields
     * with 0's. They aren't super duper important right now.
     */
    /* Halfmove clock and fullmove number */
    fprintf(stderr, "0 0\n");
}

Move parseLANMove(Board *board, char *movestr)
{
    Move m = {0};
    if (!movestr || strlen(movestr) < 4) return m;
    // Set source
    m = ((movestr[0] - 'a') + (movestr[1] - '1')*8) << 13;
    // Set destination
    m |= ((movestr[2] - 'a') + (movestr[3] - '1')*8) << 7;
    // Find the piece type
    int piece_type;
    int color;
    for (color = _WHITE; color <= _BLACK; color++)
        for (piece_type=PAWN; piece_type<=KING; piece_type++)
            if (mgetsrcbb(m) & board->pieces[piece_type + color]) break;
    m |= (piece_type << 4) | (color);
    // Check for promotions
    switch (movestr[4])
    {
        case 'n': m |= KNIGHT << 1; break;
        case 'b': m |= BISHOP << 1; break;
        case 'r': m |= ROOK   << 1; break;
        case 'q': m |= QUEEN  << 1; break;
    }
    return m;
}

void sprintLANMove(char *s, Move m)
{
    s[0] = (mgetsrc(m) % 8) + 'a';
    s[1] = (mgetsrc(m) / 8) + '1';
    s[2] = (mgetdst(m) % 8) + 'a';
    s[3] = (mgetdst(m) / 8) + '1';
    switch (mgetprom(m))
    {
        case KNIGHT: s[4] = 'n'; s[5] = ' '; s[6] = '\0'; break;
        case BISHOP: s[4] = 'b'; s[5] = ' '; s[6] = '\0'; break;
        case ROOK  : s[4] = 'r'; s[5] = ' '; s[6] = '\0'; break;
        case QUEEN : s[4] = 'q'; s[5] = ' '; s[6] = '\0'; break;
        default    : s[4] = ' '; s[5] = '\0';
    }
}