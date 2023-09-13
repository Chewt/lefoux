#include <stdio.h>
#include <omp.h>

#include "computeMagic.h"

#define NUM_THREADS 6

int main()
{
#ifdef _OPENMP
	fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
#else
    fprintf( stderr, "No OpenMP support!\n" );
    return 1;
#endif

    omp_set_num_threads(NUM_THREADS);
    magicMoves();
    return 0;
}
