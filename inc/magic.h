#ifndef MAGIC
#define MAGIC

#include <stdint.h>

typedef struct {
    uint64_t mask;
    uint64_t magic;
} Magic;

extern const Magic magicBishop[64];
extern const Magic magicRook[64];
extern const uint64_t magicRookAttacks[64][4096];
extern const uint64_t magicBishopAttacks[64][512];

int computeMagic();

int computeRookMagic();
int computeBishopMagic();

#endif
