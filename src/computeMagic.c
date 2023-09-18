#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#include "board.h"
#include "bitHelpers.h"
#include "magic.h"

#define MAGIC_TRIALS 100000000

uint64_t rand64()
{
    uint64_t u1, u2, u3, u4;
    u1 = (uint64_t)(rand()) & 0xFFFFUL;
    u2 = (uint64_t)(rand()) & 0xFFFFUL;
    u3 = (uint64_t)(rand()) & 0xFFFFUL;
    u4 = (uint64_t)(rand()) & 0xFFFFUL;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

uint64_t rand64FewBits()
{
    return rand64() & rand64() & rand64();
}

uint64_t occupancyIdxToMask(int occupancyIdx, uint64_t mask)
{
    uint64_t occupancyMask = 0;
    int n = getNumBits(mask);
    for (int occBitIdx=0; occBitIdx<n; occBitIdx++)
    {
        int maskBitIdx = bitScanForward(mask);
        // If this occupancy bit is set, set that bit on the mask
        if (occupancyIdx & (1 << occBitIdx)) occupancyMask |= 1UL << maskBitIdx;
        // Unset bit in mask to move on to next bit
        mask ^= 1UL << maskBitIdx;
    }
    return occupancyMask;
}

/*
 * Generates a mask of relevant rook occupancies for generating an attack mask
 */
uint64_t rookOccupancyMask(int square)
{
    uint64_t up, down, left, right, mask=0;
    up = down = left = right = (1UL << square);
    for (int i=0; i<8; i++){
        // Move 1 row up and down and 1 column left and right until the edge
        // of the borad is reached. Don't include the edge since we don't care
        // if there is a piece there, we attack there anyways
        mask |= (up = ((up << 8) & ~RANK[7]));
        mask |= (down = ((down >> 8) & ~RANK[0]));
        mask |= (right = ((right << 1) & ~(HFILE | AFILE)));
        mask |= (left = ((left >> 1) & ~(HFILE | AFILE)));
    }
    return mask;
}

/*
 * Manually compute a bitmap with possible rook attacks for a given occupancy
 * bitmap and a starting square
 */
uint64_t computeRookAttacks(uint64_t occupancy, int square)
{
    uint64_t attacks = 0;
    // Generate up attacks
    for (uint64_t sq = (1UL << square) << 8; sq != 0; sq <<= 8){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate down attacks
    for (uint64_t sq = (1UL << square) >> 8; sq != 0; sq >>= 8){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate right attacks
    for (uint64_t sq = (1UL << square) << 1; sq & ~AFILE; sq <<= 1){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate left attacks
    for (uint64_t sq = (1UL << square) >> 1; sq & ~HFILE; sq >>= 1){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    return attacks;
}

/*
 * Populate mRook and mRookAttacks with values for magic bitboards
 */
int computeRookMagic(Magic mRook[64], uint64_t mRookAttacks[64][4096])
{
    // Keep track of return since omp doesn't like early returns
    int ret = 0;
    // Give each thread a square to work on. We don't mark mRook and mRookAttacks
    // as shared since threads won't have overlapping work on those arrays so
    // they don't need to be protected
    #pragma omp parallel for shared(stdout, stderr, ret)
    for (int square=0; square < 64; square++)
    {
        mRook[square].magic = 0;
        uint64_t mask = rookOccupancyMask(square);
        int numMaskBits = getNumBits(mask);
        // n should be small, 12 or less
        int biggestIndex = 1 << numMaskBits;

        // Populate the rookOccupancy and rookAttacks tables for this square.
        // We will compare magic number results to this for correctness
        uint64_t rookOccupancy[4096];
        uint64_t rookAttacks[4096];
        // Iterate through every possible occupancy configuration for this mask
        for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
        {
            // For this occupancy index, map it to the masked occupancy
            rookOccupancy[occupancyIdx] = occupancyIdxToMask(
                    occupancyIdx, mask
                    );

            // Now populate the rookAttacks table using the occupancyIdx
            // for comparing magic number lookup results later
            rookAttacks[occupancyIdx] = computeRookAttacks(
                    rookOccupancy[occupancyIdx], square
                    );
        }

        for (int i=0; i<MAGIC_TRIALS; i++)
        {
            uint64_t magic;
            magic = rand64FewBits();
            // Skip numbers that clearly won't index high enough
            if (getNumBits((mask * magic) & (0xFFUL << 56)) < 6) continue;
            // Clear the attacks for this magic number
            for (int j=0; j < 4096; j++) mRookAttacks[square][j] = 0UL;
            // Iterate through every possible occupancy for this mask
            for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
            {
                // Relying on 64 - numMaskBits > 32 so assigning to an int
                // doesn't truncate
                int testIdx = magic * rookOccupancy[occupancyIdx]
                              >> (64 - numMaskBits);
                // If 0, this is a new index so we set it
                if (mRookAttacks[square][testIdx] == 0UL)
                {
                    mRookAttacks[square][testIdx] = rookAttacks[occupancyIdx];
                }
                // If not 0 and it's not the attack pattern we want, we have
                // a collision so fail this magic number
                else if (mRookAttacks[square][testIdx] != rookAttacks[occupancyIdx])
                {
                    magic = 0;
                    break;
                }
            }
            // If magic is nonzero, it must've filled the table without
            // collisions so store it and stop checking for more on this square!
            if (magic) {
                mRook[square].mask = mask;
                mRook[square].magic = magic;
                break;
            }
        }
        if (mRook[square].magic == 0UL)
        {
            fprintf(stderr, "Failed to find Rook's magic number for square %c%c\n",
                    (square%8) + 'a', (square/8) + '1');
            ret = 1;
        }
        else
        {
            fprintf(stderr, "Found Rook's magic number for square %c%c\n",
                    (square%8) + 'a', (square/8) + '1');
        }
    }
    return ret;
}

/*
 * Generates a mask of relevant bishop occupancies for generating an attack mask
 */
uint64_t bishopOccupancyMask(int square)
{
    uint64_t mask = 0;
    uint64_t left, right;
    left = right = (1UL << square);
    for (int i=1; i<8; i++){
        // Keep track of left and right. Don't include the first and last file,
        // we don't care if a piece is there since we'll attack there anyways
        left = ((left >> 1) & ~(AFILE | HFILE));
        right = ((right << 1) & ~(AFILE | HFILE));
        // Move left and right up and down to create a diagonal. Don't include
        // the first and last rank, we don't care if a piece is there since we'll
        // attack there anyways
        mask |= (left << 8*i) & ~RANK[7];
        mask |= (left >> 8*i) & ~RANK[0];
        mask |= (right << 8*i) & ~RANK[7];
        mask |= (right >> 8*i) & ~RANK[0];
    }
    return mask;
}

uint64_t computeBishopAttacks(uint64_t occupancy, int square)
{
    uint64_t attacks = 0;
    // Generate up-right attacks
    for (uint64_t sq = (1UL << square) << 9; sq != 0 && sq & ~AFILE; sq <<= 9){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate up-left attacks
    for (uint64_t sq = (1UL << square) << 7; sq != 0 && sq & ~HFILE; sq <<= 7){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate down-left attacks
    for (uint64_t sq = (1UL << square) >> 9; sq != 0 && sq & ~HFILE; sq >>= 9){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate down-right attacks
    for (uint64_t sq = (1UL << square) >> 7; sq != 0 && sq & ~AFILE; sq >>= 7){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    return attacks;
}

int computeBishopMagic(Magic mBishop[64], uint64_t mBishopAttacks[64][512])
{
    // Keep track of return since omp doesn't like early returns
    int ret = 0;
    // Give each thread a square to work on. We don't mark mRook and mRookAttacks
    // as shared since threads won't have overlapping work on those arrays so
    // they don't need to be protected
    #pragma omp parallel for shared(stdout, stderr, ret)
    for (int square=0; square < 64; square++)
    {
        mBishop[square].magic = 0;
        uint64_t mask = bishopOccupancyMask(square);
        int numMaskBits = getNumBits(mask);
        // n should be small, 12 or less
        int biggestIndex = 1 << numMaskBits;

        // Populate the bishopOccupancy and bishopAttacks tables for this square.
        // We will compare magic number results to this for correctness
        uint64_t bishopOccupancy[512];
        uint64_t bishopAttacks[512];
        for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
        {
            // For this occupancy index, map it to the masked occupancy
            bishopOccupancy[occupancyIdx] = occupancyIdxToMask(
                    occupancyIdx, mask
                    );

            // Now populate the bishopAttacks table using the occupancyIdx
            // to compare magic number lookup results for collisions
            bishopAttacks[occupancyIdx] = computeBishopAttacks(
                    bishopOccupancy[occupancyIdx], square
                    );
        }

        for (int i=0; i<MAGIC_TRIALS; i++)
        {
            uint64_t magic;
            magic = rand64FewBits();
            // Skip numbers that clearly won't index high enough
            if (getNumBits((mask * magic) & (0xFFUL << 56)) < 6) continue;
            // Clear the attacks for this magic number
            for (int j=0; j < 512; j++) mBishopAttacks[square][j] = 0UL;
            for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
            {
                // Relying on 64 - numMaskBits > 32 so assigning to an int
                // doesn't truncate
                int testIdx = magic * bishopOccupancy[occupancyIdx]
                              >> (64 - numMaskBits);
                if (mBishopAttacks[square][testIdx] == 0UL)
                {
                    mBishopAttacks[square][testIdx] =
                        bishopAttacks[occupancyIdx];
                }
                else if (mBishopAttacks[square][testIdx] !=
                         bishopAttacks[occupancyIdx])
                {
                    magic = 0;
                    break;
                }
            }
            if (magic) {
                mBishop[square].mask = mask;
                mBishop[square].magic = magic;
                break;
            }
        }
        if (mBishop[square].magic == 0UL)
        {
            fprintf(stderr, "Failed to find Bishop's magic number for square %c%c\n",
                    (square%8) + 'a', (square/8) + '1');
            ret = 1;
        }
        else
        {
            fprintf(stderr, "Found Bishop's magic number for square %c%c\n",
                    (square%8) + 'a', (square/8) + '1');
        }
    }
    return ret;
}

/*
 * Prints to stdout a source file to populate magicBishop, magicRook,
 * magicBishopAttacks, and magicRookAttacks.
 */
int computeMagic()
{
    Magic mBishop[64] = {0};
    Magic mRook[64] = {0};
    uint64_t mBishopAttacks[64][512] = {0};
    uint64_t mRookAttacks[64][4096] = {0};
    // Compute the magic here and only print if they are both successful
    if ( !computeBishopMagic(mBishop, mBishopAttacks) &&
         !computeRookMagic(mRook, mRookAttacks) )
    {
        printf("#include \"magic.h\"\n\n");
        // Bishop magic
        printf("const Magic magicBishop[64] = {\n");
        for (int i=0; i<64; i++)
        {
            printf("    { 0x%lxUL, 0x%lxUL },\n", mBishop[i].mask, mBishop[i].magic);
        }
        printf("};\n\nconst uint64_t magicBishopAttacks[64][512] = {\n");
        for (int i=0; i<64; i++)
        {
            printf("    { ");
            for (int j=0; j<512; j++)
            {
                printf("0x%lxUL, ", mBishopAttacks[i][j]);
            }
            printf("},\n");
        }
        printf("};\n\n");

        // Rook magic
        printf("const Magic magicRook[64] = {\n");
        for (int i=0; i<64; i++)
        {
            printf("    { 0x%lxUL, 0x%lxUL },\n", mRook[i].mask, mRook[i].magic);
        }
        printf("};\n\nconst uint64_t magicRookAttacks[64][4096] = {\n");
        for (int i=0; i<64; i++)
        {
            printf("    { ");
            for (int j=0; j<4096; j++)
            {
                printf("0x%lxUL, ", mRookAttacks[i][j]);
            }
            printf("},\n");
        }
        printf("};\n");
        return 0;
    }
    return 1;
}
