#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "engine.h"
#include "bitHelpers.h"
#include "timer.h"

static char *good = "\e[32m";
static char *bad = "\e[31m";
static char *clear = "\e[0m";
static char *nameColor = "\e[33m";

void printInt(int x) { fprintf(stderr, "%d\n", x); }

int intDiff(int a, int b) { return a - b; }

void printLongHex(uint64_t x) { printf("0x%lx", x); }

uint64_t xor64bit(uint64_t a, uint64_t b) { return a ^ b; }

int xorInt(int a, int b) { return a ^ b; }

Board* boardDiff(Board *check, Board *ref)
{
    Board *diffBoard = malloc(sizeof(Board));
    memset(diffBoard, 0, sizeof(Board));
    int diffFlag = 0;
    if (check->info != ref->info)
    {
        diffBoard->info = check->info ^ ref->info;
        diffFlag = 1;
    }
    for (int pieceType = 0; pieceType < 12; pieceType++)
    {
        if (check->pieces[pieceType] != ref->pieces[pieceType])
        {
           diffBoard->pieces[pieceType] =
               check->pieces[pieceType] ^ ref->pieces[pieceType];
                diffFlag = 1;
        }
    }
    if (diffFlag)
        return diffBoard;
    free(diffBoard);
    return NULL;
}

PerftInfo* runPerftTest(Board *board, PerftInfo *pi, uint8_t depth)
{
    memset(pi, 0, sizeof(PerftInfo));
    perftRunThreaded(board, pi, depth);
    return pi;
}

PerftInfo* perftDiff(PerftInfo *check, PerftInfo *ref)
{
    PerftInfo *diff = malloc(sizeof(PerftInfo));
    int diffFlag = 0;
    diffFlag |= (diff->nodes = ref->nodes - check->nodes) ? 1 : 0;
    diffFlag |= (diff->captures = ref->captures - check->captures) ? 1 : 0;
    diffFlag |= (diff->enpassants = ref->enpassants - check->enpassants) ? 1 : 0;
    diffFlag |= (diff->castles = ref->castles - check->castles) ? 1 : 0;
    diffFlag |= (diff->promotions = ref->promotions - check->promotions) ? 1 : 0;
    diffFlag |= (diff->checks = ref->checks - check->checks) ? 1 : 0;
    diffFlag |= (diff->checkmates = ref->checkmates - check->checkmates) ? 1 : 0;
    if (diffFlag) return diff;
    free(diff);
    return NULL;
}

void myPrintPerft(PerftInfo *pi) { printPerft(*pi); }

Move moveDiff(Move check, Move ref)
{
    // Ignore upper bits full of nonsense
    return (check ^ ref) & 0x7ffff;
}

void noFree(void *a) { return; }

/*
 * tests
 * @brief runs a series of tests
 * @return number of tests that failed
 */
int tests()
{
    int pass = 0;
    int fail = 0;
    char *name;
    Timer t;

    /*
     * RUN_TEST
     * @brief macro to quickly create a test with custom resultVar formatting
     * @param label Label string for the current test
     * @param testCode Code to test
     * @param resultType the type of the result of testCode and expected
     * @param expected the expected value, compared to resultVar
     * @param resultFmt a function that formats result of testCode, diff and
     *   expected.
     * @param diff a function that computes the difference between the result
     *   of testCode and expected. Returns 0 or NULL when there is no difference
     * @param diffFree a function to free result of diff if the result is some
     *    sort of poiner. If no need to free, leave noFree
     */
#define RUN_TEST( label, testCode, resultType, expected, resultFmt, diff, diffFree ) \
    { \
    name = label; \
    StartTimer(&t); \
    resultType resultVar = (testCode); \
    StopTimer(&t); \
    resultType resDiff = diff(resultVar, (expected)); \
    if ( !resDiff ) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s. ", \
                nameColor, name, clear, good, clear); \
        if (t.time_taken > 0.0001)\
            fprintf(stderr, "Took %.6f seconds\n", t.time_taken);\
        else \
            fprintf(stderr, "\n");\
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. ", \
                nameColor, name, clear, bad, clear); \
        if (t.time_taken > 0.0001)\
            fprintf(stderr, "Took %.6f seconds\n", t.time_taken);\
        else \
            fprintf(stderr, "\n");\
        fprintf(stderr, "  Expected: %s", good); \
        resultFmt(expected); \
        fprintf(stderr, "%s  Actual:   %s", clear, bad); \
        resultFmt(resultVar); \
        fprintf(stderr, "%s  Diff:   %s", clear, bad); \
        resultFmt(resDiff); \
        fprintf(stderr, "%s\n", clear); \
        fail++; \
        if (resDiff) diffFree((void*)resDiff); \
    } \
    }

    Board b;
    Board fen_board;
    Board target;
    Move m;
    Move undoM;
    Move allMoves[MAX_MOVES_PER_POSITION];
    PerftInfo pi;

    /* Bithelpers Tests */
    fprintf(stderr, " -- Bithelpers -- \n");
    RUN_TEST( "bitScanForward", bitScanForward(  0xF00  ), int, 8, printInt,
              intDiff, noFree);
    RUN_TEST( "bitScanReverse", bitScanReverse(  0xF00  ), int, 11, printInt,
              intDiff , noFree);
    RUN_TEST( "shiftWrapLeft", shiftWrapLeft( 0xFF00000000000000UL, 8 ),
              uint64_t, 0xFFUL, printLongHex, xor64bit, noFree);
    RUN_TEST( "shiftWrapRight", shiftWrapRight( 0xFFUL, 8 ),
              uint64_t, 0xFF00000000000000UL, printLongHex, xor64bit, noFree);
    RUN_TEST( "getNumBits", getNumBits( 0xFFUL ), int, 8, printInt, intDiff, noFree);

    /* Magic Lookup Tests */
    fprintf(stderr, " -- Magic Lookup -- \n");
    RUN_TEST( "magicLookupRook few occupancies",
              magicLookupRook( 0x103UL , IA1 ), uint64_t, 0x102UL,
              printBitboard, xor64bit, noFree);
    RUN_TEST( "magicLookupRook many obstructions",
              magicLookupRook( 0xFFFFUL , IA1 ), uint64_t, 0x102UL,
              printBitboard, xor64bit, noFree);
    RUN_TEST( "magicLookupRook many occupancies, few obstructions",
              magicLookupRook( 0xFFFFUL << 48, IA1 ), uint64_t,
              (((AFILE | RANK[0]) & ~1UL) & ~RANK[7]), printBitboard,
              xor64bit, noFree);

    RUN_TEST( "magicLookupBishop many obstructions",
             magicLookupBishop( 0xFFFFFFUL , IB2 ), uint64_t, 0x50005UL,
             printBitboard, xor64bit, noFree);
    RUN_TEST( "magicLookupBishop few obstructions",
             magicLookupBishop( 0xFFFFUL << 48, IB2 ), uint64_t,
             0x0040201008050005, printBitboard, xor64bit , noFree);

    /* boardMove Tests */
    b = getDefaultBoard();
    fprintf(stderr, " -- Board Moves -- \n");
    m = mcreate(0, IE2, IE4, PAWN, 0, _WHITE);
    boardMove(&b, m);
    RUN_TEST("pawn e2e4 boardMove", b.pieces[PAWN], uint64_t, (RANK[1] ^ E2)|E4,
            printBitboard, xor64bit, noFree);
    RUN_TEST("pawn e2e4 board info", b.info, uint16_t,
                    (((0x7 & (IE3 % 8)) | 8) << 5) | 0xf << 1 | 0x1, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = mcreate(0, IA1, IB1, ROOK, 0, _WHITE);
    boardMove(&b, m);
    RUN_TEST("queenside white rook affect castling", b.info, uint16_t,
             (0x7 << 1) | _BLACK, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = mcreate(0, IH1, IG1, ROOK, 0, _WHITE);
    boardMove(&b, m);
    RUN_TEST("kingside white rook affect castling", b.info, uint16_t,
             (0xb << 1) | _BLACK, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = mcreate(0, IE1, IF1, KING, 0, _WHITE);
    boardMove(&b, m);
    RUN_TEST("white king affect castling", b.info, uint16_t,
             (0x3 << 1) | _BLACK, printBoardInfo, xorInt , noFree);

    b = getDefaultBoard();
    b.info |= _BLACK;
    m = mcreate(0, IE7, IE5, PAWN, 0, _BLACK);
    RUN_TEST("mgetpiece macro check",
             (mgetpiece(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))),
              int, PAWN, printInt, intDiff, noFree);
    RUN_TEST("mgetcol macro check",
             (mgetcol(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))),
              int, _BLACK, printInt, intDiff, noFree);
    boardMove(&b, m);
    RUN_TEST("pawn e7e5 boardMove", b.pieces[_PAWN], uint64_t,
             (RANK[6] ^ E7) | E5, printBitboard, xor64bit, noFree);
    RUN_TEST("pawn e7e5 board info", b.info, uint16_t,
             (((0x7 & (IE6 % 8)) | 8) << 5) | 0xf << 1 | 0x0, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.info |= _BLACK;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = mcreate(0, IA8, IB8, ROOK, 0, _BLACK);
    boardMove(&b, m);
    RUN_TEST("queenside black rook affect castling", b.info, uint16_t,
            (0xd << 1) | _WHITE, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.info |= _BLACK;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = mcreate(0, IH8, IG8, ROOK, 0, _BLACK);
    boardMove(&b, m);
    RUN_TEST("kingside black rook affect castling", b.info, uint16_t,
            (0xe << 1) | _WHITE, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.info |= _BLACK;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = mcreate(0, IE8, IF8, KING, 0, _BLACK);
    boardMove(&b, m);
    RUN_TEST("black king affect castling", b.info, uint16_t,
            (0xc << 1) | _WHITE, printBoardInfo, xorInt, noFree);

    b = getDefaultBoard();
    b.info |= _WHITE;
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = mcreate(0, IE1, IG1, KING, 0, _WHITE);
    boardMove(&b, m);
    target = getDefaultBoard();
    target.info |= _BLACK;
    target.info &= ~0x18;
    target.pieces[KNIGHT] = 0UL;
    target.pieces[BISHOP] = 0UL;
    target.pieces[KING] = G1;
    target.pieces[ROOK] = A1 | F1;
    RUN_TEST("Castling white kingside", &b, Board*,
            &target, printBoard, boardDiff, free);

    b = getDefaultBoard();
    b.info |= _WHITE;
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    b.pieces[QUEEN] = 0UL;
    m = mcreate(0, IE1, IC1, KING, 0, _WHITE);
    boardMove(&b, m);
    target = getDefaultBoard();
    target.info |= _BLACK;
    target.info &= ~0x18;
    target.pieces[KNIGHT] = 0UL;
    target.pieces[BISHOP] = 0UL;
    target.pieces[QUEEN] = 0UL;
    target.pieces[KING] = C1;
    target.pieces[ROOK] = D1 | H1;
    RUN_TEST("Castling white queenside", &b, Board*,
            &target, printBoard, boardDiff, free);

    b = getDefaultBoard();
    b.info |= _BLACK;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = mcreate(0, IE8, IG8, KING, 0, _BLACK);
    boardMove(&b, m);
    target = getDefaultBoard();
    target.info |= _WHITE;
    target.info &= ~0x6;
    target.pieces[_KNIGHT] = 0UL;
    target.pieces[_BISHOP] = 0UL;
    target.pieces[_KING] = G8;
    target.pieces[_ROOK] = A8 | F8;
    RUN_TEST("Castling black kingside", &b, Board*,
            &target, printBoard, boardDiff, free);

    b = getDefaultBoard();
    b.info |= _BLACK;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    b.pieces[_QUEEN] = 0UL;
    m = mcreate(0, IE8, IC8, KING, 0, _BLACK);
    boardMove(&b, m);
    target = getDefaultBoard();
    target.info |= _WHITE;
    target.info &= ~0x6;
    target.pieces[_KNIGHT] = 0UL;
    target.pieces[_BISHOP] = 0UL;
    target.pieces[_QUEEN] = 0UL;
    target.pieces[_KING] = C8;
    target.pieces[_ROOK] = D8 | H8;
    RUN_TEST("Castling black queenside", &b, Board*,
            &target, printBoard, boardDiff, free);

    /* genAllLegalMoves Tests */
    fprintf(stderr, " -- genAllLegalMoves() -- \n");
    b = getDefaultBoard();
    RUN_TEST( "genAllLegalMoves from starting position",
              (genAllLegalMoves(&b, allMoves)), int, 20, printInt, intDiff , noFree);
    m = mcreate(0, IH2, IH4, PAWN, 0, _WHITE);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4",
              (genAllLegalMoves(&b, allMoves)), int, 20, printInt, intDiff , noFree);
    m = mcreate(0, IE7, IE5, PAWN, 0, _BLACK);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4 e5",
              (genAllLegalMoves(&b, allMoves)), int, 21, printInt, intDiff , noFree);
    m = mcreate(0, IH4, IH5, PAWN, 0, _WHITE);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4 e5 2. h5",
              (genAllLegalMoves(&b, allMoves)), int, 29, printInt, intDiff , noFree);
    m = mcreate(0, IG7, IG5, PAWN, 0, _BLACK);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4 e5 2. h5 g5",
              (genAllLegalMoves(&b, allMoves)), int, 23, printInt, intDiff , noFree);

    /* FEN tests */
    fprintf(stderr, " -- FEN Tests -- \n");
    b = getDefaultBoard();
    loadFen(&fen_board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    RUN_TEST("loadFen check default position", &fen_board, Board*, &b,
              printBoard, boardDiff, free);
    m = mcreate(0, IE2, IE4, PAWN, 0, _WHITE);
    boardMove(&b, m);
    loadFen(&fen_board, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    RUN_TEST("loadFen check 1. e4", &fen_board, Board*, &b,
              printBoard, boardDiff, free);

    /* Undo tests */
    fprintf(stderr, "-- Undo Tests --\n");
    loadFen(&fen_board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/Pp2P3/2N2Q1p/1PPBBPPP/R3K2R b KQkq a3 0 1");
    loadFen(&b, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/Pp2P3/2N2Q1p/1PPBBPPP/R3K2R b KQkq a3 0 1");
    m = mcreate(0, IC7, IC5, PAWN, 0, _BLACK);
    undoM = boardMove(&b, m);
    m = mcreate(0, ID5, IC6, PAWN, 0, _WHITE);
    m = boardMove(&b, m);
    undoMove(&b, m);
    undoMove(&b, undoM);
    RUN_TEST("Undo with en passant", &b, Board*, &fen_board,
              printBoard, boardDiff, free);

    /* Evaluate Board tests */
    fprintf(stderr, " -- Evaluate Board Tests -- \n");
    loadFen(&b, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    RUN_TEST("evaluate default board", (evaluateBoard(&b)), int, 0, printInt,
             intDiff, noFree);
    loadFen(&b, "rnbqkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    RUN_TEST("evaluate board 1 black pawn missing", (evaluateBoard(&b)), int, 1,
             printInt, intDiff, noFree);
    loadFen(&b, "rnbqkbnr/pppppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1");
    RUN_TEST("evaluate board 1 white pawn missing", (evaluateBoard(&b)), int,
             -1, printInt, intDiff, noFree);

    /* Perft tests */
    fprintf(stderr, " -- Default board perft tests -- \n");
    b = getDefaultBoard();
    RUN_TEST("Perft depth 0", runPerftTest(&b, &pi, 0), PerftInfo*,
              &((PerftInfo){1, 0, 0, 0, 0, 0 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 1", runPerftTest(&b, &pi, 1), PerftInfo*,
              &((PerftInfo){20, 0, 0, 0, 0, 0 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 2", runPerftTest(&b, &pi, 2), PerftInfo*,
              &((PerftInfo){400, 0, 0, 0, 0, 0 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 3", runPerftTest(&b, &pi, 3), PerftInfo*,
              &((PerftInfo){8902, 34, 0, 0, 0, 12 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 4", runPerftTest(&b, &pi, 4), PerftInfo*,
              &((PerftInfo){197281, 1576, 0, 0, 0, 469 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 5", runPerftTest(&b, &pi, 5), PerftInfo*,
              &((PerftInfo){4865609, 82719, 258, 0, 0, 27351 ,0}),
              myPrintPerft, perftDiff, free);
    /* Long test, takes about 30 seconds on 1 thread */
/*
    RUN_TEST("Perft depth 6", runPerftTest(&b, &pi, 6), PerftInfo*,
              &((PerftInfo){119060324, 2812008, 5248, 0, 0, 809099 ,0}),
              myPrintPerft, perftDiff, free);
*/

    /* Position 2 Peft Tests */
    fprintf(stderr, " -- Position 2 Perft Tests -- \n");
    loadFen(&b, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    RUN_TEST("Perft depth 1 - position 2", runPerftTest(&b, &pi, 1), PerftInfo*,
              &((PerftInfo){48, 8, 0, 2, 0, 0 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 2 - position 2", runPerftTest(&b, &pi, 2), PerftInfo*,
              &((PerftInfo){2039, 351, 1, 91, 0, 3 ,0}),
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 3 - position 2", runPerftTest(&b, &pi, 3), PerftInfo*,
              &((PerftInfo){97862, 17102, 45, 3162, 0, 993 ,0}),
              myPrintPerft, perftDiff, free);

    /* Puzzle Proficiency */
    fprintf(stderr, "-- Puzzle Proficiency --\n");
    loadFen(&b, "1k6/6R1/1K6/8/8/8/8/8 w - - 0 0");
    m = mcreate(0, IG7, IG8, ROOK, 0, _WHITE);
    RUN_TEST("Mate in one: White King and Rook", findBestMove(&b, 1), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "1K6/6r1/1k6/8/8/8/8/8 b - - 0 0");
    m = mcreate(0, IG7, IG8, ROOK, 0, _BLACK);
    RUN_TEST("Mate in one: Black King and Rook", findBestMove(&b, 1), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "1k6/6R1/7R/K7/8/8/8/8 w - - 0 0");
    m = mcreate(0, IH6, IH8, ROOK, 0, _WHITE);
    RUN_TEST("Mate in one: White Rook ladder", findBestMove(&b, 1), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "1K6/6r1/7r/k7/8/8/8/8 b - - 0 0");
    m = mcreate(0, IH6, IH8, ROOK, 0, _BLACK);
    RUN_TEST("Mate in one: Black Rook ladder", findBestMove(&b, 1), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "8/1k6/6R1/K6R/8/8/8/8 w - - 0 0");
    m = mcreate(0, IH5, IH7, ROOK, 0, _WHITE);
    RUN_TEST("Mate in two: Rook ladder", findBestMove(&b, 3), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "8/1K6/6r1/k6r/8/8/8/8 b - - 0 0");
    m = mcreate(0, IH5, IH7, ROOK, 0, _BLACK);
    RUN_TEST("Mate in two: Black Rook ladder", findBestMove(&b, 3), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "7k/6b1/5Q1p/3P4/2pP4/1pP4P/1r1q2P1/4R1K1 w - - 4 36");
    m = mcreate(0, IE1, IE8, ROOK, 0, _WHITE);
    RUN_TEST("Puzzle 1w: Mate in two", findBestMove(&b, 4), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "1k1r4/1p2Q1R1/p4pP1/4pP2/4p3/P1q5/1B6/K7 b - - 0 1");
    m = mcreate(0, ID8, ID1, ROOK, 0, _BLACK);
    RUN_TEST("Puzzle 1b: Mate in two", findBestMove(&b, 4), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "r4rk1/pp3ppp/2n5/3p4/4nB2/2qBP3/P1Q2PPP/R4RK1 w - - 0 17");
    m = mcreate(0, ID3, IE4, BISHOP, 0, _WHITE);
    RUN_TEST("Puzzle 2w: remove the defender", findBestMove(&b, 4), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "1kr4r/ppp2q1p/3pbQ2/2bN4/4P3/5N2/PPP3PP/1KR4R b - - 0 1");
    m = mcreate(0, IE6, ID5, BISHOP, 0, _BLACK);
    RUN_TEST("Puzzle 2b: remove the defender", findBestMove(&b, 4), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "rn1qk2r/pp3ppp/4p3/2bn4/6b1/4PN2/PP3PPP/RNBQKB1R w KQkq - 0 1");
    m = mcreate(0, ID1, IA4, QUEEN, 0, _WHITE);
    RUN_TEST("Puzzle 3w: Fork with check", findBestMove(&b, 4), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "rnbqkb1r/pp3ppp/4pn2/6B1/2BN4/4P3/PP3PPP/RN1QK2R b KQkq - 0 7");
    m = mcreate(0, ID8, IA5, QUEEN, 0, _BLACK);
    RUN_TEST("Puzzle 3b: Fork with check", findBestMove(&b, 4), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "3k4/5pQ1/3p4/1P2pP2/1Pr5/8/6PK/q7 w - - 0 32");
    m = mcreate(0, IG7, IF8, QUEEN, 0, _WHITE);
    RUN_TEST("Puzzle 4w: Fork in the future", findBestMove(&b, 5), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "7Q/kp6/8/5Rp1/2pP2p1/4P3/1qP5/4K3 b - - 0 1");
    m = mcreate(0, IB2, IC1, QUEEN, 0, _BLACK);
    RUN_TEST("Puzzle 4b: Fork in the future", findBestMove(&b, 5), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "4Q3/r4rkp/2p3p1/3p4/3P1P2/8/pq3PK1/3R3R w - - 8 33");
    m = mcreate(0, IH1, IH7, ROOK, 0, _WHITE);
    RUN_TEST("Puzzle 5w: SACK THE ROOOOKKKKK!!!", findBestMove(&b, 5), Move, m,
        printMoveSAN, moveDiff, noFree);

    loadFen(&b, "r3r3/1kp3QP/8/2p1p3/4P3/1P3P2/PKR4R/3q4 b - - 0 1");
    m = mcreate(0, IA8, IA2, ROOK, 0, _BLACK);
    RUN_TEST("Puzzle 5b: SACK THE ROOOOKKKKK!!!", findBestMove(&b, 5), Move, m,
        printMoveSAN, moveDiff, noFree);

    /* Print output */
    fprintf(stderr, "Tests passed: %s%d%s of %d\n",
            good, pass, clear, pass + fail);
#undef RUN_TEST

    return fail;
}
