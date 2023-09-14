#ifndef BITHELPERS
#define BITHELPERS

#include <stdint.h>

int bitScanForward(uint64_t bb);
int bitScanReverse(uint64_t bb);

// Bit shifts that wrap around. Could use x86 asm instructions rol and ror
inline uint64_t shiftWrapLeft(uint64_t x, int s)
{
    return (x << s) | (x >> (64-s));
}

inline uint64_t shiftWrapRight(uint64_t x, int s)
{
    return (x >> s) | (x << (64-s));
}

// Counts the number of bits
inline int getNumBits(uint64_t x)
{
   int numBits = x ? 1 : 0;
   while (x = x & (x-1)) numBits++;
   return numBits;
}


#endif
