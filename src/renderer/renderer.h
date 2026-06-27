#pragma once
#include "../core/types.h"
#include "../platform/window.h"
#include <stdbool.h>

typedef enum {
    GW_RENDERER_BACKEND_VULKAN = 0,
    GW_RENDERER_BACKEND_DX12,
    GW_RENDERER_BACKEND_OPENGL,
} GwRendererBackend;

typedef struct GwRendererDesc {
    GwWindow         *window;
    GwRendererBackend backend;
} GwRendererDesc;

typedef struct GwRendererAPI {
    bool (*init)        (const GwRendererDesc *desc);
    void (*shutdown)    (void);
    bool (*begin_frame) (void);
    void (*end_frame)   (void);
    void (*wait_idle)   (void);
    void (*on_resize)   (u32 width, u32 height);
} GwRendererAPI;

bool gw_renderer_init(const GwRendererDesc *desc);
void gw_renderer_shutdown(void);
bool gw_renderer_begin_frame(void);
void gw_renderer_end_frame(void);
void gw_renderer_wait_idle(void);
void gw_renderer_on_resize(u32 width, u32 height);