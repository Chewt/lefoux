#include <stdio.h>
#include <stdint.h>

#include "board.h"
#include "bitHelpers.h"

/*
 * tests
 * @brief runs a series of tests
 * @return number of tests that failed
 */
int tests()
{
    int pass = 0;
    int fail = 0;

    char *good = "\e[32m";
    char *bad = "\e[31m";
    char *clear = "\e[0m";
    char *nameColor = "\e[33m";

    char *name;
    int int_res;
    uint64_t uint64_t_res;
    uint16_t uint16_t_res;

    /*
     * RUN_TEST
     * @brief macro to quickly create a test.
     * @param testCode literal code to run in the test
     * @param resultVar an already declared variable to store result of testCode
     *   into.
     * @param resultFmt a printf format specifier for resultVar
     * @param expected the expected value, compared to resultVar
     */
#define RUN_TEST( testCode, resultVar, resultFmt, expected ) { \
    name = #testCode; \
    if (( resultVar = (testCode) ) == ( expected )) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s.\n", \
                nameColor, name, clear, good, clear); \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. \n" \
                "  Expected: %s" resultFmt "%s\n" \
                "  Actual:   %s" resultFmt "%s\n" \
                "  Diff:     %s" resultFmt "%s\n", \
                nameColor, name, clear, bad, clear, \
                good, (expected), clear, \
                bad, resultVar, clear, \
                bad, resultVar ^ (expected), clear); \
        fail++; \
    } \
}

    /*
     * RUN_TEST_FMT
     * @brief macro to quickly create a test with custom resultVar formatting
     * @param testCode literal code to run in the test
     * @param resultVar an already declared variable to store result of testCode
     *   into.
     * @param resultFmt a function that formats resultVar and expected.
     * @param expected the expected value, compared to resultVar
     */
#define RUN_TEST_FMT( testCode, resultVar, resultFmt, expected ) { \
    name = #testCode; \
    if (( resultVar = (testCode) ) == (expected) ) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s.\n", \
                nameColor, name, clear, good, clear); \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. \n", \
                nameColor, name, clear, bad, clear); \
        fprintf(stderr, "  Expected: %s\n", good); \
        resultFmt(expected); \
        fprintf(stderr, "%s  Actual:   %s\n", clear, bad); \
        resultFmt(resultVar); \
        fprintf(stderr, "%s\n", clear); \
        fail++; \
    } \
}
    /*
     * RUN_TEST_CUSTOM
     * @brief macro to quickly create a test with custom resultVar formatting
     * @param testVar Test value to compare against
     * @param resultVar an already declared variable to store result of testCode
     *   into.
     * @param resultFmt a function that formats resultVar and expected.
     * @param expected the expected value, compared to resultVar
     */
#define RUN_TEST_CUSTOM( label, testVar, resultVar, resultFmt, expected ) { \
    name = label; \
    if (( resultVar = (testVar) ) == (expected) ) \
    { \
        fprintf(stderr, "Test %s%s%s %spassed%s.\n", \
                nameColor, name, clear, good, clear); \
        pass++; \
    } \
    else \
    { \
        fprintf(stderr, "Test %s%s%s %sfailed%s. \n", \
                nameColor, name, clear, bad, clear); \
        fprintf(stderr, "  Expected: %s\n", good); \
        resultFmt(expected); \
        fprintf(stderr, "%s  Actual:   %s\n", clear, bad); \
        resultFmt(resultVar); \
        fprintf(stderr, "%s\n", clear); \
        fail++; \
    } \
}

    RUN_TEST( bitScanForward(  0xF00  ), int_res, "%d", 8 );
    RUN_TEST( bitScanReverse(  0xF00  ), int_res, "%d", 11 );
    RUN_TEST( shiftWrapLeft( 0xFF00000000000000UL, 8 ), uint64_t_res, "%lx",
            0xFFUL);
    RUN_TEST( shiftWrapRight( 0xFFUL, 8 ), uint64_t_res, "%lx",
            0xFF00000000000000UL);
    RUN_TEST( getNumBits( 0xFFUL ), int_res, "%d", 8 );
    RUN_TEST( getNumBits( 0UL ), int_res, "%d", 0 );
    RUN_TEST_FMT( magicLookupRook( 0x103UL , IA1 ), uint64_t_res,
            printBitboard, 0x102UL);
    RUN_TEST_FMT( magicLookupRook( 0xFFFFUL , IA1 ), uint64_t_res,
            printBitboard, 0x102UL);
    RUN_TEST_FMT( magicLookupRook( 0xFFFFUL << 48, IA1 ), uint64_t_res,
            printBitboard, ((AFILE | RANK[0]) & ~1UL) & ~RANK[7] );

   RUN_TEST_FMT( magicLookupBishop( 0xFFFFFFUL , IB2 ), uint64_t_res,
   printBitboard, 0x50005UL);
   RUN_TEST_FMT( magicLookupBishop( 0xFFFFUL << 48, IB2 ), uint64_t_res,
   printBitboard, 0x0040201008050005 );

    /* boardMove tests */
    Board b = getDefaultBoard();
    Move m = _WHITE | (PAWN << 4) | (IE2 << 13) | (IE4 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("pawn e2e4 boardMove", b.pieces[PAWN], uint64_t_res,
            printBitboard, (RANK[1] ^ E2) | E4);
    RUN_TEST_CUSTOM("pawn e2e4 board info", b.info, uint16_t_res,
            printBoardInfo, ((0x3f & IE3) << 5) | 0xf << 1 | 0x1);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = _WHITE | (ROOK << 4) | (IA1 << 13) | (IB1 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("queenside white rook affect castling", b.info, uint16_t_res,
            printBoardInfo, (0x7 << 1) | _BLACK);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = _WHITE | (ROOK << 4) | (IH1 << 13) | (IG1 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("kingside white rook affect castling", b.info, uint16_t_res,
            printBoardInfo, (0xb << 1) | _BLACK);
    b = getDefaultBoard();
    b.pieces[KNIGHT] = 0UL;
    b.pieces[BISHOP] = 0UL;
    m = _WHITE | (KING << 4) | (IE1 << 13) | (IF1 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("white king affect castling", b.info, uint16_t_res,
            printBoardInfo, (0x3 << 1) | _BLACK);

    b = getDefaultBoard();
    b.info |= _BLACK;
    m = _BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7);
    RUN_TEST((mgetpiece(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))), 
            int_res, "%d", PAWN );
    RUN_TEST((mgetcol((_BLACK) | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))), 
            int_res, "%d", BLACK );
    RUN_TEST((mgetpiece(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7)) 
                + mgetcol(_BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7))), 
            int_res, "%d", _PAWN );
        boardMove(&b, m);
        RUN_TEST_CUSTOM("pawn e7e5 boardMove", b.pieces[_PAWN], uint64_t_res,
                printBitboard, (RANK[6] ^ E7) | E5);
    RUN_TEST_CUSTOM("pawn e7e5 board info", b.info, uint16_t_res,
            printBoardInfo, ((0x3f & IE6) << 5) | 0xf << 1 | 0x0);
    b = getDefaultBoard();
    b.info |= 0x1;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = _BLACK | (ROOK << 4) | (IA8 << 13) | (IB8 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("queenside black rook affect castling", b.info, uint16_t_res,
            printBoardInfo, (0xd << 1) | _WHITE);
    b = getDefaultBoard();
    b.info |= 0x1;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = _BLACK | (ROOK << 4) | (IH8 << 13) | (IG8 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("kingside rook affect castling", b.info, uint16_t_res,
            printBoardInfo, (0xe << 1) | _WHITE);
    b = getDefaultBoard();
    b.info |= 0x1;
    b.pieces[_KNIGHT] = 0UL;
    b.pieces[_BISHOP] = 0UL;
    m = _BLACK | (KING << 4) | (IE8 << 13) | (IF8 << 7);
    boardMove(&b, m);
    RUN_TEST_CUSTOM("white king affect castling", b.info, uint16_t_res,
            printBoardInfo, (0xc << 1) | _WHITE);
    
    // Test cases for genAllLegalMoves
    b = getDefaultBoard();
    Move allMoves[MAX_MOVES_PER_POSITION];
    RUN_TEST( (genAllLegalMoves(&b, allMoves)), int_res, "%d", 20 );
    m = _WHITE | (PAWN << 4) | (IH2 << 13) | (IH4 << 7);
    boardMove(&b, m);
    RUN_TEST( (genAllLegalMoves(&b, allMoves)), int_res, "%d", 20 );
    m = _BLACK | (PAWN << 4) | (IE7 << 13) | (IE5 << 7);
    boardMove(&b, m);
    RUN_TEST( (genAllLegalMoves(&b, allMoves)), int_res, "%d", 21 );
    m = _WHITE | (PAWN << 4) | (IH4 << 13) | (IH5 << 7);
    boardMove(&b, m);
    RUN_TEST( (genAllLegalMoves(&b, allMoves)), int_res, "%d", 29 );

    for (int i=0; i<int_res; i++) 
    {
        printMove(allMoves[i]);
    }

    fprintf(stderr, "Tests passed: %s%d%s of %d\n",
            good, pass, clear, pass + fail);
#undef RUN_TEST
#undef RUN_TEST_FMT
#undef RUN_TEST_CUSTOM

    return fail;
}
