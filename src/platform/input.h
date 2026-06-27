#pragma once
#include "../core/types.h"
#include <stdbool.h>

typedef enum {
    GW_KEY_UNKNOWN = 0,

    GW_KEY_SPACE = 32,
    GW_KEY_0 = 48, GW_KEY_1, GW_KEY_2, GW_KEY_3, GW_KEY_4,
    GW_KEY_5,      GW_KEY_6, GW_KEY_7, GW_KEY_8, GW_KEY_9,
    GW_KEY_A = 65, GW_KEY_B, GW_KEY_C, GW_KEY_D, GW_KEY_E,
    GW_KEY_F,      GW_KEY_G, GW_KEY_H, GW_KEY_I, GW_KEY_J,
    GW_KEY_K,      GW_KEY_L, GW_KEY_M, GW_KEY_N, GW_KEY_O,
    GW_KEY_P,      GW_KEY_Q, GW_KEY_R, GW_KEY_S, GW_KEY_T,
    GW_KEY_U,      GW_KEY_V, GW_KEY_W, GW_KEY_X, GW_KEY_Y, GW_KEY_Z,

    GW_KEY_ESCAPE = 256,
    GW_KEY_ENTER,
    GW_KEY_TAB,
    GW_KEY_BACKSPACE,
    GW_KEY_DELETE,
    GW_KEY_INSERT,
    GW_KEY_HOME,
    GW_KEY_END,
    GW_KEY_PAGE_UP,
    GW_KEY_PAGE_DOWN,
    GW_KEY_LEFT,
    GW_KEY_RIGHT,
    GW_KEY_UP,
    GW_KEY_DOWN,
    GW_KEY_F1,  GW_KEY_F2,  GW_KEY_F3,  GW_KEY_F4,
    GW_KEY_F5,  GW_KEY_F6,  GW_KEY_F7,  GW_KEY_F8,
    GW_KEY_F9,  GW_KEY_F10, GW_KEY_F11, GW_KEY_F12,
    GW_KEY_LEFT_SHIFT,  GW_KEY_RIGHT_SHIFT,
    GW_KEY_LEFT_CTRL,   GW_KEY_RIGHT_CTRL,
    GW_KEY_LEFT_ALT,    GW_KEY_RIGHT_ALT,

    GW_KEY_COUNT
} GwKey;

#define GW_MAX_BUTTONS 5

typedef enum {
    GW_MOUSE_LEFT   = 0,
    GW_MOUSE_RIGHT  = 1,
    GW_MOUSE_MIDDLE = 2,
} GwMouseButton;

typedef struct InputState {
    bool keys_cur[GW_KEY_COUNT];
    bool keys_prev[GW_KEY_COUNT];

    i32  mouse_x,      mouse_y;
    i32  mouse_dx,     mouse_dy;
    i32  mouse_prev_x, mouse_prev_y;
    i32  scroll_delta;

    bool buttons_cur[GW_MAX_BUTTONS];
    bool buttons_prev[GW_MAX_BUTTONS];
} InputState;

void input_init(InputState *s);
void input_update(InputState *s);

bool input_key_down(const InputState *s, GwKey key);
bool input_key_pressed(const InputState *s, GwKey key);
bool input_key_released(const InputState *s, GwKey key);

bool input_button_down(const InputState *s, GwMouseButton btn);
bool input_button_pressed(const InputState *s, GwMouseButton btn);
bool input_button_released(const InputState *s, GwMouseButton btn);

void input_on_key(InputState *s, GwKey key, bool down);
void input_on_mouse_move(InputState *s, i32 x, i32 y);
void input_on_mouse_button(InputState *s, GwMouseButton btn, bool down);
void input_on_scroll(InputState *s, i32 delta);