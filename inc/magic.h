#ifndef MAGIC
#define MAGIC

#include <stdint.h>

/*
 * @brief stores the magic number computed for that occupancy mask
 */
typedef struct {
    uint64_t mask;
    uint64_t magic;
} Magic;

/*
 * @brief stores the magic numbers and occupancy masks for bishops. Indexed by
 * the square the bishop is on.
 */
extern const Magic magicBishop[64];
/*
 * @brief stores the magic numbers and occupancy masks for rooks. Indexed by
 * the square the rooks is on.
 */
extern const Magic magicRook[64];
/*
 * @brief stores the attack patterns for bishops. Indexed first by square, then
 * by an index computed by the magic number stored in magicBishop[square]
 */
extern const uint64_t magicBishopAttacks[64][512];
/*
 * @brief stores the attack patterns for rooks. Indexed first by square, then
 * by an index computed by the magic number stored in magicRook[square]
 */
extern const uint64_t magicRookAttacks[64][4096];

/*
 * computeMagic
 * @brief prints to stdout a source file to populate magicBishop, magicRook,
 * magicBishopAttacks, and magicRookAttacks.
 * @return 0 if magic numbers were successfully found, 1 otherwise
 */
int computeMagic();

#endif
