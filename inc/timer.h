#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <time.h>

typedef struct
{
    struct timeval timecheck;
    long t;
    clock_t c_t;
    double time_taken;
    double cpu_time_taken;
} Timer; 

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

#endif
