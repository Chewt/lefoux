#include <stdlib.h>

#include "board.h"
#include "bitHelpers.h"

#define MAGIC_TRIALS 1000000000

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

uint64_t rookMask(int square)
{
    uint64_t up, down, left, right, mask=0;
    up = down = left = right = (1ULL << square);
    for (int i=0; i<8; i++){
        mask |= (up <<= 8);
        mask |= (down >>= 8);
        mask |= (left = ((left >> 1) & ~HFILE));
        mask |= (right = ((right << 1) & ~AFILE));
    }
    // print_bitboard(mask);
    return mask;
}

int computeRookMagic()
{
    for (int square=0; square < 64; square++){
        uint64_t mask = rookMask(square);
        int n = getNumBits(mask);
        // n should be small, 12 or less
        int biggestIndex = 1 << n;

        // Populate the rookOccupancy table for this square
        // We will compare magic number results to this for
        // correctness
        uint64_t rookOccupancy[4096];
        uint64_t rookAttacks[4096];
        for (int occupancyIdx=0; occupancyIdx<biggestIndex; occupancyIdx++)
        {
            // For this occupancy index, map it to the masked occupancy
            rookOccupancy[occupancyIdx] = 0;
            for (int occBitIdx=0; occBitIdx<n; occBitIdx++)
            {
                // If this occupancy bit is set, set that bit on the mask
                if (occupancyIdx & (1 << occBitIdx)) {
                    int maskBitIdx = bitScanForward(mask);
                    rookOccupancy[occupancyIdx] |= 1 << maskBitIdx;
                }
            }

            // Now populate the rookAttacks table using the occupancyIdx
            // to compare magic number lookup results for collisions

        }

        uint64_t magic;
        for (int i=0; i<MAGIC_TRIALS; i++)
        {

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
