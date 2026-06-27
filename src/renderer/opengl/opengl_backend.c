#include "../renderer.h"
#include "../../core/logger.h"

static bool gl_init(const GwRendererDesc *desc) {
    (void)desc;
    LOG_WARN("[opengl] backend not yet implemented");
    return false;
}

static void gl_shutdown(void)            {}
static bool gl_begin_frame(void)        { return false; }
static void gl_end_frame(void)          {}
static void gl_wait_idle(void)          {}
static void gl_on_resize(u32 w, u32 h) { (void)w; (void)h; }

const GwRendererAPI gw_renderer_opengl = {
    .init        = gl_init,
    .shutdown    = gl_shutdown,
    .begin_frame = gl_begin_frame,
    .end_frame   = gl_end_frame,
    .wait_idle   = gl_wait_idle,
    .on_resize   = gl_on_resize,
};