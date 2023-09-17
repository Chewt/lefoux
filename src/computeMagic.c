#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "bitHelpers.h"

#define MAGIC_TRIALS 1000000000

const int RBits[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

const int BBits[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

uint64_t rand64()
{
    uint64_t u1, u2, u3, u4;
    u1 = (uint64_t)(rand()) & 0xFFFFULL;
    u2 = (uint64_t)(rand()) & 0xFFFFULL;
    u3 = (uint64_t)(rand()) & 0xFFFFULL;
    u4 = (uint64_t)(rand()) & 0xFFFFULL;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

/*
 * Returns a random 64 bit integer with roughly 8 bits set
 */
uint64_t rand64FewBits()
{
    return rand64() & rand64() & rand64();
}

uint64_t rookOccupancyMask(int square)
{
    uint64_t up, down, left, right, mask=0;
    up = down = left = right = (1ULL << square);
    for (int i=0; i<8; i++){
        mask |= (up = ((up << 8) & ~RANK[7]));
        mask |= (down = ((down >> 8) & ~RANK[0]));
        mask |= (right = ((right << 1) & ~(HFILE | AFILE)));
        mask |= (left = ((left >> 1) & ~(HFILE | AFILE)));
    }
    // printBitboard(mask);
    return mask;
}

int computeMagic()
{
    Magic mRook[64] = {0};
    uint64_t mRookAttacks[64][4096] = {0};
    if (!computeRookMagic(mRook, mRookAttacks))
    {
        printf("const Magic magicRook[64] = {\n");
        for (int i=0; i<64; i++)
        {
            printf("    { 0x%lx, 0x%lx },\n", mRook[i].mask, mRook[i].magic);
        }
        printf("};\n\nconst uint64_t magicRookAttacks = {\n");
        for (int i=0; i<64; i++)
        {
            printf("    { ");
            for (int j=0; j<4096; j++)
            {
                printf("0x%lx, ", mRookAttacks[i][j]);
            }
            printf("},\n");
        }
        printf("};\n");
        return 0;
    }
    return 1;
}

uint64_t computeRookAttacks(uint64_t occupancy, int square)
{
    uint64_t attacks = 0;
    // Generate up attacks
    for (uint64_t sq = (1ULL << square) << 8; sq != 0; sq <<= 8){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate down attacks
    for (uint64_t sq = (1ULL << square) >> 8; sq != 0; sq >>= 8){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate right attacks
    for (uint64_t sq = (1ULL << square) << 1; sq & ~AFILE; sq <<= 1){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    // Generate left attacks
    for (uint64_t sq = (1ULL << square) >> 1; sq & ~HFILE; sq >>= 1){
        attacks |= sq;
        if (occupancy & sq) break;
    }
    return attacks;
}

uint64_t occupancyIdxToMask(int occupancyIdx, uint64_t moveMask)
{
    uint64_t occupancyMask = 0;
    int n = getNumBits(moveMask);
    for (int occBitIdx=0; occBitIdx<n; occBitIdx++)
    {
        // If this occupancy bit is set, set that bit on the mask
        if (occupancyIdx & (1 << occBitIdx)) {
            int maskBitIdx = bitScanForward(moveMask);
            occupancyMask |= 1ULL << maskBitIdx;
            moveMask ^= 1ULL << maskBitIdx;
        }
    }
    return occupancyMask;
}

int computeRookMagic(Magic mRook[64], uint64_t mRookAttacks[64][4096])
{
    for (int square=0; square < 64; square++){
        mRook[square].magic = 0;
        uint64_t mask = rookOccupancyMask(square);
        int numMaskBits = getNumBits(mask);
        // n should be small, 12 or less
        int biggestIndex = 1 << numMaskBits;

        // Populate the rookOccupancy and rookAttacks tables for this square.
        // We will compare magic number results to this for correctness
        uint64_t rookOccupancy[4096];
        uint64_t rookAttacks[4096];
        for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
        {
            // For this occupancy index, map it to the masked occupancy
            rookOccupancy[occupancyIdx] = occupancyIdxToMask(
                    occupancyIdx, mask
                    );

            // Now populate the rookAttacks table using the occupancyIdx
            // to compare magic number lookup results for collisions
            rookAttacks[occupancyIdx] = computeRookAttacks(
                    rookOccupancy[occupancyIdx], square
                    );
        }

        uint64_t magic;
        for (int i=0; i<MAGIC_TRIALS; i++)
        {
            magic = rand64FewBits();
            // Skip numbers that clearly won't index high enough
            if (getNumBits((mask * magic) & (0xFFULL << 56)) < 6) continue;
            // Clear the attacks for this magic number
            for (int i=0; i < 4096; i++) mRookAttacks[square][i] = 0ULL;
            for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
            {
                int testIdx = magic*mask >> numMaskBits;
                if (mRookAttacks[square][testIdx] == 0ULL)
                {
                    mRookAttacks[square][testIdx] = rookAttacks[testIdx];
                }
                else if (mRookAttacks[square][testIdx] != rookAttacks[testIdx])
                {
                    magic = 0;
                    break;
                }
            }
            if (magic) {
                mRook[square].mask = mask;
                mRook[square].magic = magic;
                break;
            }
        }
        if (mRook[square].magic == 0ULL)
        {
            fprintf(stderr, "Failed to find Rook's magic number for square %c%c\n",
                    (square%8) + 'a', (square/8) + '1');
            return 1;
        }
        else
        {
            fprintf(stderr, "Found Rook's magic number for square %c%c\n",
                    (square%8) + 'a', (square/8) + '1');
        }
    }
    return 0;
}

int computeBishopMagic()
{
    for (int square=0; square < 64; square++){
        uint64_t mask = 0;
        uint64_t up, down, left, right;
        up = down = left = right = (1ULL << square);
        for (int i=1; i<8; i++){
            left = ((left >> 1) & ~HFILE);
            right = ((right << 1) & ~AFILE);
            mask |= left << 8*i;
            mask |= left >> 8*i;
            mask |= right << 8*i;
            mask |= right >> 8*i;
        }
        // print_bitboard(mask);
    }
    return 0;
}
