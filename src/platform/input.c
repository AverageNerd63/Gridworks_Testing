#include "input.h"
#include <string.h>

void input_init(InputState *s)   { memset(s, 0, sizeof(*s)); }

void input_update(InputState *s) {
    memcpy(s->keys_prev,    s->keys_cur,    sizeof(s->keys_cur));
    memcpy(s->buttons_prev, s->buttons_cur, sizeof(s->buttons_cur));
    s->mouse_dx     = s->mouse_x - s->mouse_prev_x;
    s->mouse_dy     = s->mouse_y - s->mouse_prev_y;
    s->mouse_prev_x = s->mouse_x;
    s->mouse_prev_y = s->mouse_y;
    s->scroll_delta = 0;
}

bool input_key_down(const InputState *s, GwKey k)     { return s->keys_cur[k]; }
bool input_key_pressed(const InputState *s, GwKey k)  { return  s->keys_cur[k] && !s->keys_prev[k]; }
bool input_key_released(const InputState *s, GwKey k) { return !s->keys_cur[k] &&  s->keys_prev[k]; }

bool input_button_down(const InputState *s, GwMouseButton b)     { return s->buttons_cur[b]; }
bool input_button_pressed(const InputState *s, GwMouseButton b)  { return  s->buttons_cur[b] && !s->buttons_prev[b]; }
bool input_button_released(const InputState *s, GwMouseButton b) { return !s->buttons_cur[b] &&  s->buttons_prev[b]; }

void input_on_key(InputState *s, GwKey k, bool down)              { s->keys_cur[k] = down; }
void input_on_mouse_move(InputState *s, i32 x, i32 y)             { s->mouse_x = x; s->mouse_y = y; }
void input_on_mouse_button(InputState *s, GwMouseButton b, bool d) { s->buttons_cur[b] = d; }
void input_on_scroll(InputState *s, i32 delta)                    { s->scroll_delta += delta; }