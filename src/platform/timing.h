#pragma once
#include "../core/types.h"

typedef struct {
    f64  delta;
    f64  elapsed;
    u64  frame_count;
    i64  freq;
    i64  last_ticks;
} Timer;

void timer_init(Timer *t);
void timer_tick(Timer *t);