#pragma once
#include "../core/types.h"
#include <stdbool.h>

typedef struct {
    u8  *bitmap;     /* R8 grayscale, width × height */
    u32  width, height;
    /* per-codepoint glyph data for ASCII 32–126 */
    struct {
        f32 x0, y0, x1, y1;   /* pixel rect in atlas */
        f32 u0, v0, u1, v1;   /* UV rect */
        f32 advance_x;
        f32 offset_x, offset_y;
    } glyphs[128];
    f32 line_height;
} FontAtlas;

bool font_atlas_build(FontAtlas *atlas, const char *ttf_path, f32 size_px,
                      u32 atlas_w, u32 atlas_h);
void font_atlas_destroy(FontAtlas *atlas);