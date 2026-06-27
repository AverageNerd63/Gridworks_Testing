#include "renderer.h"
#include "../core/logger.h"

/* forward declarations — each backend implements these */
extern const GwRendererAPI gw_renderer_vulkan;
extern const GwRendererAPI gw_renderer_dx12;
extern const GwRendererAPI gw_renderer_opengl;

static GwRendererAPI s_api;

bool gw_renderer_init(const GwRendererDesc *desc) {
    switch (desc->backend) {
        case GW_RENDERER_BACKEND_VULKAN:  s_api = gw_renderer_vulkan;  break;
        case GW_RENDERER_BACKEND_DX12:    s_api = gw_renderer_dx12;    break;
        case GW_RENDERER_BACKEND_OPENGL:  s_api = gw_renderer_opengl;  break;
        default:
            LOG_ERROR("renderer: unknown backend %d", desc->backend);
            return false;
    }
    return s_api.init(desc);
}

void gw_renderer_shutdown(void)              { s_api.shutdown(); }
bool gw_renderer_begin_frame(void)           { return s_api.begin_frame(); }
void gw_renderer_end_frame(void)             { s_api.end_frame(); }
void gw_renderer_wait_idle(void)             { s_api.wait_idle(); }
void gw_renderer_on_resize(u32 w, u32 h)    { s_api.on_resize(w, h); }