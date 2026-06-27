#include "font_atlas.h"
#include "../core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

bool font_atlas_build(FontAtlas *atlas, const char *ttf_path, f32 size_px,
                      u32 atlas_w, u32 atlas_h) {
    FILE *f = fopen(ttf_path, "rb");
    if (!f) { LOG_ERROR("[font] cannot open %s", ttf_path); return false; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f); rewind(f);
    u8 *ttf_data = malloc((usize)sz);
    if (!ttf_data) { fclose(f); LOG_ERROR("[font] OOM"); return false; }
    fread(ttf_data, 1, (usize)sz, f);
    fclose(f);

    atlas->bitmap = calloc(atlas_w * atlas_h, 1);
    atlas->width  = atlas_w;
    atlas->height = atlas_h;

    stbtt_fontinfo info;
    stbtt_InitFont(&info, ttf_data, stbtt_GetFontOffsetForIndex(ttf_data, 0));

    f32 scale = stbtt_ScaleForPixelHeight(&info, size_px);
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    atlas->line_height = (ascent - descent + line_gap) * scale;

    stbtt_pack_context ctx;
    if (!stbtt_PackBegin(&ctx, atlas->bitmap, (int)atlas_w, (int)atlas_h, 0, 1, NULL)) {
        LOG_ERROR("[font] stbtt_PackBegin failed");
        free(atlas->bitmap); atlas->bitmap = NULL;
        free(ttf_data);
        return false;
    }
    stbtt_PackSetOversampling(&ctx, 2, 2);

    stbtt_packedchar packed[95];
    stbtt_PackFontRange(&ctx, ttf_data, 0, size_px, 32, 95, packed);
    stbtt_PackEnd(&ctx);

    for (int c = 32; c < 127; c++) {
        stbtt_packedchar *pc = &packed[c - 32];
        atlas->glyphs[c].x0 = pc->x0; atlas->glyphs[c].y0 = pc->y0;
        atlas->glyphs[c].x1 = pc->x1; atlas->glyphs[c].y1 = pc->y1;
        atlas->glyphs[c].u0 = (f32)pc->x0 / atlas_w;
        atlas->glyphs[c].v0 = (f32)pc->y0 / atlas_h;
        atlas->glyphs[c].u1 = (f32)pc->x1 / atlas_w;
        atlas->glyphs[c].v1 = (f32)pc->y1 / atlas_h;
        atlas->glyphs[c].advance_x = pc->xadvance;
        atlas->glyphs[c].offset_x  = pc->xoff;
        atlas->glyphs[c].offset_y  = pc->yoff;
    }

    free(ttf_data);
    LOG_INFO("[font] atlas built %ux%u @ %.0fpx", atlas_w, atlas_h, size_px);
    return true;
}

void font_atlas_destroy(FontAtlas *atlas) {
    free(atlas->bitmap);
    memset(atlas, 0, sizeof(*atlas));
}