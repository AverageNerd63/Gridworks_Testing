#include "../renderer.h"
#include "../../core/logger.h"

static bool dx12_init(const GwRendererDesc *desc) {
    (void)desc;
    LOG_WARN("[dx12] backend not yet implemented");
    return false;
}

static void dx12_shutdown(void)            {}
static bool dx12_begin_frame(void)        { return false; }
static void dx12_end_frame(void)          {}
static void dx12_wait_idle(void)          {}
static void dx12_on_resize(u32 w, u32 h) { (void)w; (void)h; }

const GwRendererAPI gw_renderer_dx12 = {
    .init        = dx12_init,
    .shutdown    = dx12_shutdown,
    .begin_frame = dx12_begin_frame,
    .end_frame   = dx12_end_frame,
    .wait_idle   = dx12_wait_idle,
    .on_resize   = dx12_on_resize,
};