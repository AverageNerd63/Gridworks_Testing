#include "../timing.h"
#include "../../core/logger.h"
#include <time.h>

void timer_init(Timer *t) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t->freq        = 1000000000LL;
    t->last_ticks  = (i64)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    t->delta       = 0.0;
    t->elapsed     = 0.0;
    t->frame_count = 0;
}

void timer_tick(Timer *t) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    i64 now       = (i64)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    t->delta      = (f64)(now - t->last_ticks) / (f64)t->freq;
    t->elapsed   += t->delta;
    t->last_ticks = now;
    t->frame_count++;
}