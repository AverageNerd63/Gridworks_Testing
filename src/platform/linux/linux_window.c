#include "../window.h"
#include "../../core/logger.h"

struct GwWindowNative { int placeholder; };

bool gw_window_create(GwWindow *w, const char *title, i32 width, i32 height) {
    (void)w; (void)title; (void)width; (void)height;
    LOG_ERROR("gw_window_create: Linux not yet implemented");
    return false;
}
void gw_window_poll(GwWindow *w)    { (void)w; }
void gw_window_destroy(GwWindow *w) { (void)w; }