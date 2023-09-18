#ifndef BITHELPERS
#define BITHELPERS

#include <stdint.h>

int bitScanForward(uint64_t bb);
int bitScanReverse(uint64_t bb);

/*
 * shiftWrapLeft
 * @param x integer to be shifted
 * @param s number of bits to shift
 * @return x shifted to the left with overflow bits wrapping around
 */
inline uint64_t shiftWrapLeft(uint64_t x, int s)
{
    return (x << s) | (x >> (64-s));
}

/*
 * shiftWrapRight
 * @param x integer to be shifted
 * @param s number of bits to shift
 * @return x shifted to the right with underflow bits wrapping around
 */
inline uint64_t shiftWrapRight(uint64_t x, int s)
{
    return (x >> s) | (x << (64-s));
}

/*
 * getNumBits
 * @param x integer to count the bits of
 * @return number of set bits in x
 */
inline int getNumBits(uint64_t x)
{
   int numBits = x ? 1 : 0;
   while (x &= (x-1)) numBits++;
   return numBits;
}


#endif
