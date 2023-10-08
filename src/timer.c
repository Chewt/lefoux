#include <sys/time.h>
#include <time.h>

#include "timer.h"

void StartTimer(Timer* t)
{
    t->c_t = clock();
    gettimeofday(&(t->timecheck), NULL);
    t->t = (long)(t->timecheck.tv_sec) * 1000 +
           (long)(t->timecheck.tv_usec) / 1000;
}

void StopTimer(Timer* t)
{
    t->cpu_time_taken = ((double) (clock() - t->c_t) / CLOCKS_PER_SEC);
    gettimeofday(&(t->timecheck), NULL);
    t->t = (long)(t->timecheck.tv_sec) * 1000 +
           (long)(t->timecheck.tv_usec) / 1000 - t->t;
    t->time_taken = (double)t->t / 1000;
}
