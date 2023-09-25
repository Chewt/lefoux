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

void printInt(int x) { printf("%d", x); }

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
    perftRun(board, pi, depth);
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

void noFree(void* a) { return; }

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
     * @param testCode Code that resolves to a value to store into resultVar
     * @param resultType the type of the result of testCode and expected
     * @param expected the expected value, compared to resultVar
     * @param resultFmt a function that formats result of testCode, diff and 
     *   expected.
     * @param diff a function that computes the difference between the result 
     *   of testCode and expected. Returns 0 or NULL when there is no difference
     * @param diffFree a function to free result of diff if the result is some 
     *    sort of poiner. If no need to free, leave noFree
     */
#define RUN_TEST( label, testCode, resultType, expected, resultFmt, diff, diffFree ) { \
    { \
    name = label; \
    StartTimer(&t); \
    resultType resultVar = (testCode); \
    StopTimer(&t); \
    resultType resDiff = diff(resultVar, (expected)); \
    if ( !resDiff ) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s. Took %.6f seconds\n", \
                nameColor, name, clear, good, clear, t.time_taken); \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. Took %.6f seconds\n", \
                nameColor, name, clear, bad, clear, t.time_taken); \
        fprintf(stderr, "  Expected: %s\n", good); \
        resultFmt(expected); \
        fprintf(stderr, "%s  Actual:   %s\n", clear, bad); \
        resultFmt(resultVar); \
        fprintf(stderr, "%s  Diff:   %s\n", clear, bad); \
        resultFmt(resDiff); \
        fprintf(stderr, "%s\n", clear); \
        fail++; \
        if (resDiff) diffFree((void*)resDiff); \
    } \
    } \
}
#define RUN_PERFT_TEST( b, depth, expected ) { \
    PerftInfo pi = { 0 }; \
    StartTimer(&t); \
    perftRun(b, &pi, depth); \
    StopTimer(&t); \
    if (      (pi.nodes == expected.nodes) \
            && (pi.captures == expected.captures) \
            && (pi.enpassants == expected.enpassants) \
            && (pi.checks == expected.checks) \
            && (pi.checkmates == expected.checkmates) \
            && (pi.castles == expected.castles) \
            && (pi.promotions == expected.promotions)) \
    { \
        fprintf(stderr, "Test %sPerft depth %d%s %spassed%s. Took %.6f seconds\n", \
                nameColor, depth, clear, good, clear, t.time_taken);  \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %sPerft depth %d%s %sfailed%s. Took %.6f seconds\n", \
                nameColor, depth, clear, bad, clear, t.time_taken);  \
        fprintf(stderr, "  Expected: %s\n", good); \
        printPerft(expected); \
        fprintf(stderr, "%s  Actual:   %s\n", clear, bad); \
        printPerft(pi); \
        fprintf(stderr, "%s\n", clear); \
        fail++; \
    } \
}

    fprintf(stderr, "Bithelpers\n");
    RUN_TEST( "bitScanForward", bitScanForward(  0xF00  ), int, 8, printInt, 
              intDiff, noFree);
    RUN_TEST( "bitScanReverse", bitScanReverse(  0xF00  ), int, 11, printInt, 
              intDiff , noFree);
    RUN_TEST( "shiftWrapLeft", shiftWrapLeft( 0xFF00000000000000UL, 8 ), 
              uint64_t, 0xFFUL, printLongHex, xor64bit, noFree);
    RUN_TEST( "shiftWrapRight", shiftWrapRight( 0xFFUL, 8 ), 
              uint64_t, 0xFF00000000000000UL, printLongHex, xor64bit, noFree);
    RUN_TEST( "getNumBits", getNumBits( 0xFFUL ), int, 8, printInt, intDiff, noFree);

    fprintf(stderr, "Magic Lookup\n");
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

    /* boardMove tests */
    Board b = getDefaultBoard();
    Move m = _WHITE | (PAWN << 4) | (IE2 << 13) | (IE4 << 7);
    boardMove(&b, m);
    RUN_TEST("pawn e2e4 boardMove", b.pieces[PAWN], uint64_t, (RANK[1] ^ E2)|E4,
            printBitboard, xor64bit, noFree);
    RUN_TEST("pawn e2e4 board info", b.info, uint16_t,
                    ((0x3f & IE3) << 5) | 0xf << 1 | 0x1, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = _WHITE | (ROOK << 4) | (IA1 << 13) | (IB1 << 7);
    boardMove(&b, m);
    RUN_TEST("queenside white rook affect castling", b.info, uint16_t, 
             (0x7 << 1) | _BLACK, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = _WHITE | (ROOK << 4) | (IH1 << 13) | (IG1 << 7);
    boardMove(&b, m);
    RUN_TEST("kingside white rook affect castling", b.info, uint16_t, 
             (0xb << 1) | _BLACK, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = _WHITE | (KING << 4) | (IE1 << 13) | (IF1 << 7);
    boardMove(&b, m);
    RUN_TEST("white king affect castling", b.info, uint16_t,
             (0x3 << 1) | _BLACK, printBoardInfo, xorInt , noFree);

    b = getDefaultBoard();
    b.info |= _BLACK;
    m = _BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7);
    RUN_TEST("mgetpiece macro check", 
             (mgetpiece(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))), 
              int, PAWN, printInt, intDiff, noFree);
    RUN_TEST("mgetcol macro check", 
             (mgetcol(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))), 
              int, BLACK, printInt, intDiff, noFree);
    RUN_TEST("mgetcol + mgetpiece check", 
             (mgetcol(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))), 
              int, _PAWN, printInt, intDiff, noFree);
    boardMove(&b, m);
    RUN_TEST("pawn e7e5 boardMove", b.pieces[_PAWN], uint64_t, 
             (RANK[6] ^ E7) | E5, printBitboard, xor64bit, noFree);
    RUN_TEST("pawn e7e5 board info", b.info, uint16_t, 
             ((0x3f & IE6) << 5) | 0xf << 1 | 0x0, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.info |= 0x1;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = _BLACK | (ROOK << 4) | (IA8 << 13) | (IB8 << 7);
    boardMove(&b, m);
    RUN_TEST("queenside black rook affect castling", b.info, uint16_t,
            (0xd << 1) | _WHITE, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.info |= 0x1;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = _BLACK | (ROOK << 4) | (IH8 << 13) | (IG8 << 7);
    boardMove(&b, m);
    RUN_TEST("kingside rook affect castling", b.info, uint16_t,
            (0xe << 1) | _WHITE, printBoardInfo, xorInt, noFree);
    b = getDefaultBoard();
    b.info |= 0x1;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = _BLACK | (KING << 4) | (IE8 << 13) | (IF8 << 7);
    boardMove(&b, m);
    RUN_TEST("white king affect castling", b.info, uint16_t,
            (0xc << 1) | _WHITE, printBoardInfo, xorInt, noFree);
    
    // Test cases for genAllLegalMoves
    b = getDefaultBoard();
    Move allMoves[MAX_MOVES_PER_POSITION];
    RUN_TEST( "genAllLegalMoves from starting position",
              (genAllLegalMoves(&b, allMoves)), int, 20, printInt, intDiff , noFree);
    m = _WHITE | (PAWN << 4) | (IH2 << 13) | (IH4 << 7);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4",
              (genAllLegalMoves(&b, allMoves)), int, 20, printInt, intDiff , noFree);
    m = _BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4 e5",
              (genAllLegalMoves(&b, allMoves)), int, 21, printInt, intDiff , noFree);
    m = _WHITE | (PAWN << 4) | (IH4 << 13) | (IH5 << 7);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4 e5 2. h5",
              (genAllLegalMoves(&b, allMoves)), int, 29, printInt, intDiff , noFree);
    m = _BLACK | (PAWN << 4) | (IG7 << 13) | (IG5 << 7);
    boardMove(&b, m);
    RUN_TEST( "genAllLegalMoves from 1. h4 e5 2. h5 g5",
              (genAllLegalMoves(&b, allMoves)), int, 22, printInt, intDiff , noFree);

    /* FEN tests */
    fprintf(stderr, "FEN Tests\n");
    b = getDefaultBoard();
    Board fen_board;
    loadFen(&fen_board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    RUN_TEST("loadFen check default position", &fen_board, Board*, &b, 
              printBoard, boardDiff, free);
    m = _WHITE | (PAWN << 4) | (IE2 << 13) | (IE4 << 7);
    boardMove(&b, m);
    loadFen(&fen_board, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    RUN_TEST("loadFen check 1. e4", &fen_board, Board*, &b, 
              printBoard, boardDiff, free);

    /* Perft tests */
    fprintf(stderr, "Default board perft tests\n");
    b = getDefaultBoard();
    PerftInfo pi = {0};
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
              &((PerftInfo){4865609, 82719, 258, 0, 0, 809099 ,0}), 
              myPrintPerft, perftDiff, free);

    fprintf(stderr, "Position 2 perft tests\n");
    loadFen(&b, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    RUN_TEST("Perft depth 1 - position 2", runPerftTest(&b, &pi, 1), PerftInfo*, 
              &((PerftInfo){48, 8, 0, 2, 0, 0 ,0}), 
              myPrintPerft, perftDiff, free);
    RUN_TEST("Perft depth 2 - position 2", runPerftTest(&b, &pi, 2), PerftInfo*, 
              &((PerftInfo){2039, 351, 1, 91, 0, 3 ,0}), 
              myPrintPerft, perftDiff, free);

    fprintf(stderr, "Tests passed: %s%d%s of %d\n",
            good, pass, clear, pass + fail);
#undef RUN_TEST
#undef RUN_TEST_FMT
#undef RUN_TEST_CUSTOM

    return fail;
}
