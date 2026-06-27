#include "ui_draw.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>

bool ui_draw_init(UIDrawList *dl, FontAtlas *atlas, void *cmd_pool) {
    dl->atlas      = atlas;
    dl->vert_cap   = 4096;
    dl->vert_count = 0;
    dl->verts      = malloc(dl->vert_cap * sizeof(UIVertex));

    if (!gw_texture_create_r8(&dl->texture, atlas->bitmap,
                               atlas->width, atlas->height, cmd_pool)) {
        LOG_ERROR("[ui] font texture upload failed");
        return false;
    }
    return true;
}

void ui_draw_shutdown(UIDrawList *dl) {
    gw_texture_destroy(&dl->texture);
    free(dl->verts);
    memset(dl, 0, sizeof(*dl));
}

void ui_draw_clear(UIDrawList *dl) { dl->vert_count = 0; }

void ui_draw_text(UIDrawList *dl, f32 x, f32 y, const char *text,
                  f32 r, f32 g, f32 b, f32 a) {
    for (const char *p = text; *p; p++) {
        u8 c = (u8)*p;
        if (c < 32 || c > 126) continue;

        typedef struct {
            f32 x0, y0, x1, y1;
            f32 u0, v0, u1, v1;
            f32 advance_x;
            f32 offset_x, offset_y;
        } Glyph;

        const Glyph *glyph = (const Glyph *)&dl->atlas->glyphs[c];

        if (dl->vert_count + 6 > dl->vert_cap) break;

        f32 x0 = x + glyph->offset_x, y0 = y + glyph->offset_y;
        f32 x1 = x0 + (glyph->x1 - glyph->x0);
        f32 y1 = y0 + (glyph->y1 - glyph->y0);

        UIVertex *v = dl->verts + dl->vert_count;
        v[0] = (UIVertex){ x0, y0, glyph->u0, glyph->v0, r, g, b, a };
        v[1] = (UIVertex){ x1, y0, glyph->u1, glyph->v0, r, g, b, a };
        v[2] = (UIVertex){ x1, y1, glyph->u1, glyph->v1, r, g, b, a };
        v[3] = (UIVertex){ x0, y0, glyph->u0, glyph->v0, r, g, b, a };
        v[4] = (UIVertex){ x1, y1, glyph->u1, glyph->v1, r, g, b, a };
        v[5] = (UIVertex){ x0, y1, glyph->u0, glyph->v1, r, g, b, a };
        dl->vert_count += 6;
        x += glyph->advance_x;
    }
}