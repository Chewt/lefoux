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

void StartTimer(Timer* t);

void StopTimer(Timer* t);

#endif
