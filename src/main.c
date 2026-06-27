#include "core/logger.h"
#include "platform/window.h"
#include "platform/input.h"
#include "platform/timing.h"
#include "platform/filewatcher.h"
#include "platform/process.h"
#include "renderer/renderer.h"
#include "ecs/ecs.h"
#include "ecs/ecs_export.h"
#include "host/gw_host.h"

#include <stdatomic.h>
#include <string.h>

#define ENGINE_RCONFIG "managed_src/Gridworks.Engine/bin/Debug/net9.0/Gridworks.Engine.runtimeconfig.json"
#define ENGINE_DLL     "managed_src/Gridworks.Engine/bin/Debug/net9.0/Gridworks.Engine.dll"
#define ENGINE_TYPE    "Gridworks.Engine.Host.EngineHost, Gridworks.Engine"
#define USER_DLL       "managed_src/UserProject/bin/Debug/net9.0/UserProject.dll"
#define USER_WATCH_DIR "managed_src/UserProject/bin/Debug/net9.0"

typedef void (*managed_void_fn)(void *);

static managed_void_fn s_managed_load   = NULL;
static managed_void_fn s_managed_unload = NULL;
static atomic_bool     s_reload_pending = false;

static void on_file_changed(FwEvent ev, void *user) {
    (void)user;
    if (strstr(ev.path, "UserProject.dll"))
        atomic_store(&s_reload_pending, true);
}

static void on_source_changed(FwEvent ev, void *user) {
    (void)user;
    const char *ext = strrchr(ev.path, '.');
    if (!ext || strcmp(ext, ".cs") != 0) return;
    LOG_INFO("[host] source changed, rebuilding UserProject...");
    gw_process_spawn(
        "dotnet build managed_src\\UserProject\\UserProject.csproj"
        " -c Debug --nologo -v quiet");
}

int main(void) {
    logger_init();
    LOG_INFO("Gridworks starting...");

    /* ---- CoreCLR host -------------------------------------------------- */
    if (!gw_host_init(ENGINE_RCONFIG)) return 1;

    managed_void_fn managed_init = NULL;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "Initialize",         (void **)&managed_init))      return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "LoadUserAssembly",   (void **)&s_managed_load))    return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "UnloadUserAssembly", (void **)&s_managed_unload))  return 1;

    GwEcsApi api = gw_ecs_api_get();
    managed_init(&api);
    s_managed_load((void *)USER_DLL);

    /* ---- Hot reload watcher -------------------------------------------- */
    FileWatcher fw      = {0};
    FileWatcher fw_src  = {0};
    fw_start(&fw,     USER_WATCH_DIR,               on_file_changed,  NULL);
    fw_start(&fw_src, "managed_src/UserProject",    on_source_changed, NULL);

    /* ---- Platform + renderer ------------------------------------------- */
    InputState input;
    input_init(&input);

    GwWindow window = { .input = &input };
    if (!gw_window_create(&window, "Gridworks", 1280, 720)) return 1;

    GwRendererDesc rdesc = {
        .window  = &window,
        .backend = GW_RENDERER_BACKEND_VULKAN,
    };
    if (!gw_renderer_init(&rdesc)) return 1;

    Timer timer;
    timer_init(&timer);

    i32 prev_w = window.width, prev_h = window.height;
    while (window.running) {
        gw_window_poll(&window);
        timer_tick(&timer);

        if (input_key_pressed(&input, GW_KEY_ESCAPE)) window.running = false;

        if (atomic_exchange(&s_reload_pending, false)) {
            LOG_INFO("[host] hot reload triggered");
            s_managed_unload(NULL);
            s_managed_load((void *)USER_DLL);
        }

        if (window.width != prev_w || window.height != prev_h) {
            gw_renderer_on_resize((u32)window.width, (u32)window.height);
            prev_w = window.width;
            prev_h = window.height;
        }

        if (!gw_renderer_begin_frame()) continue;
        gw_renderer_end_frame();
    }

    fw_stop(&fw);
    fw_stop(&fw_src);
    gw_renderer_wait_idle();
    gw_renderer_shutdown();
    s_managed_unload(NULL);
    gw_host_shutdown();
    LOG_INFO("Shutdown clean.");
    gw_window_destroy(&window);
    logger_shutdown();
    return 0;
}