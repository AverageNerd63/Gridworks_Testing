#pragma once
#include "../core/types.h"
#include <stdbool.h>

typedef struct InputState    InputState;
typedef struct GwWindowNative GwWindowNative;  /* opaque, defined per platform */

typedef struct {
    GwWindowNative *native;
    i32             width;
    i32             height;
    bool            running;
    InputState     *input;
} GwWindow;

bool gw_window_create(GwWindow *w, const char *title, i32 width, i32 height);
void gw_window_poll(GwWindow *w);
void gw_window_destroy(GwWindow *w);

void *gw_window_get_hwnd(const GwWindow *w);