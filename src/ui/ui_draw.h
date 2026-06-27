#pragma once
#include "../core/types.h"
#include "font_atlas.h"
#include "../src/renderer/vulkan/vk_texture.h"
#include <stdbool.h>

typedef struct {
    f32 x, y;
    f32 u, v;
    f32 r, g, b, a;
} UIVertex;

typedef struct {
    FontAtlas  *atlas;
    GwTexture   texture;
    UIVertex   *verts;
    u32         vert_count;
    u32         vert_cap;
} UIDrawList;

bool  ui_draw_init(UIDrawList *dl, FontAtlas *atlas, void *cmd_pool);
void  ui_draw_shutdown(UIDrawList *dl);
void  ui_draw_clear(UIDrawList *dl);
void  ui_draw_text(UIDrawList *dl, f32 x, f32 y, const char *text,
                   f32 r, f32 g, f32 b, f32 a);