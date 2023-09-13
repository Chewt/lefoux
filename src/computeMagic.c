/*
 * Magic move numbers take a given bitboard with one piece on it and can
 * multiply with that bitboard to generate a bitboard with all possible moves.
 * This code will compute these magic move numbers.
 */
#include <stdio.h>      // printf()
#include <stdint.h>     // fancy int types
#include <stdlib.h>     // exit()
#include <omp.h>

uint64_t magicMoves() {
   uint64_t positions[64];
   uint64_t pawnMoves[64];
   // Populate positions and targets for fast lookup
   for (uint64_t i=0; i<64; i++) {
      positions[i] = 1 << i;
      // Pawn target results
      pawnMoves[i] = 0;
      if (i >= 8) {
         pawnMoves[i] |= positions[i] << 8;
         if (i < 16) {
            pawnMoves[i] |= positions[i] << 16;
         }
      }
   }
   printf("Searching\n");
   uint64_t result;
   #pragma omp parallel for default(none) shared(positions, pawnMoves, stderr)
   for (uint64_t magic=0; magic<~0; magic++){
#ifdef DEBUG
      if (magic % 0xABCDEFF == 0) {
         fprintf(stderr, "0x%lx\n", magic);
      }
#endif
      uint64_t j;
      for (j=0; j<64; j++) {
         if (magic * positions[j] != pawnMoves[j]) {
            break;
         }
      }
      if (j >= 64){
         fprintf(stderr, "RESULT FOUND!! %lx", magic);
         exit(1);
      }
   }
   return 0;
}
