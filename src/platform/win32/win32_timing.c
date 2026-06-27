#include "../timing.h"
#include <windows.h>

void timer_init(Timer *t) {
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    t->freq        = freq.QuadPart;
    t->last_ticks  = now.QuadPart;
    t->delta       = 0.0;
    t->elapsed     = 0.0;
    t->frame_count = 0;
}

void timer_tick(Timer *t) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    i64 ticks     = now.QuadPart - t->last_ticks;
    t->delta      = (f64)ticks / (f64)t->freq;
    t->elapsed   += t->delta;
    t->last_ticks = now.QuadPart;
    t->frame_count++;
}