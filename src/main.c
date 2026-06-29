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
#include "ui/editor_ui.h"
#include "ui/project_ui.h"
#include "project/project.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <objbase.h>

#define ENGINE_RCONFIG \
    "managed_src/Gridworks.Engine/bin/Debug/net9.0/Gridworks.Engine.runtimeconfig.json"
#define ENGINE_DLL \
    "managed_src/Gridworks.Engine/bin/Debug/net9.0/Gridworks.Engine.dll"
#define ENGINE_TYPE \
    "Gridworks.Engine.Host.EngineHost, Gridworks.Engine"

typedef void (*managed_void_fn)(void *);
typedef void (*managed_start_fn)(void *);
typedef void (*managed_update_fn)(float);
typedef void (*managed_stop_fn)(void *);

static managed_start_fn  s_managed_start  = NULL;
static managed_update_fn s_managed_update = NULL;
static managed_stop_fn   s_managed_stop   = NULL;

static managed_void_fn s_managed_load   = NULL;
static managed_void_fn s_managed_unload = NULL;
static atomic_bool     s_reload_pending = false;

static ProjectConfig s_project;
static bool          s_project_loaded = false;

static void on_file_changed(FwEvent ev, void *user) {
    (void)user;
    if (strstr(ev.path, "UserProject.dll"))
        atomic_store(&s_reload_pending, true);
}

static void on_source_changed(FwEvent ev, void *user) {
    (void)user;
    const char *ext = strrchr(ev.path, '.');
    if (!ext || strcmp(ext, ".cs") != 0) return;
    LOG_INFO("[host] source changed, rebuilding...");
    char cmd[PROJECT_PATH_MAX + 64];
    snprintf(cmd, sizeof cmd,
             "dotnet build \"%s\" -c Debug --nologo -v quiet",
             s_project.user_csproj);
    gw_process_spawn(cmd);
}

int main(void) {
    logger_init();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    LOG_INFO("Gridworks starting...");

    /* ---- CoreCLR host -------------------------------------------------- */
    if (!gw_host_init(ENGINE_RCONFIG)) return 1;

    managed_void_fn managed_init = NULL;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "Initialize", (void **)&managed_init))     return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "LoadUserAssembly", (void **)&s_managed_load))   return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "UnloadUserAssembly", (void **)&s_managed_unload)) return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "StartGame",  (void **)&s_managed_start))  return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "UpdateGame", (void **)&s_managed_update)) return 1;
    if (!gw_host_get_fn(ENGINE_DLL, ENGINE_TYPE, "StopGame",   (void **)&s_managed_stop))   return 1;

    GwApi api = gw_api_get();
    managed_init(&api);

    char eng_dll_abs[MAX_PATH];
    GetFullPathNameA(ENGINE_DLL, MAX_PATH, eng_dll_abs, NULL);
    project_set_engine_dll(eng_dll_abs);

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
    editor_ui_init();

    project_ui_open();

    FileWatcher fw     = {0};
    FileWatcher fw_src = {0};

    Timer timer;
    timer_init(&timer);

    static PlayState s_prev_play = PLAY_STATE_STOPPED;

    i32 prev_w = window.width, prev_h = window.height;
    while (window.running) {
        gw_window_poll(&window);
        timer_tick(&timer);

        if (input_key_pressed(&input, GW_KEY_ESCAPE) && s_project_loaded)
            window.running = false;

        if (s_project_loaded && atomic_exchange(&s_reload_pending, false)) {
            if (s_prev_play != PLAY_STATE_STOPPED) {
                if (s_managed_stop) s_managed_stop(NULL);
                    s_prev_play = PLAY_STATE_STOPPED;
                    editor_ui_set_play_state(PLAY_STATE_STOPPED);
                }
            LOG_INFO("[host] hot reload triggered");
            s_managed_unload(NULL);
            s_managed_load((void *)s_project.user_dll);
        }

        if (window.width != prev_w || window.height != prev_h) {
            gw_renderer_on_resize((u32)window.width, (u32)window.height);
            prev_w = window.width;
            prev_h = window.height;
        }

        if (!gw_renderer_begin_frame()) continue;

        if (!s_project_loaded) {
            if (project_ui_draw(&s_project)) {
                /* build first if DLL is missing (new project or clean checkout) */
                if (GetFileAttributesA(s_project.user_dll) == INVALID_FILE_ATTRIBUTES) {
                    if (!project_build(&s_project)) {
                        LOG_ERROR("[main] initial build failed");
                        project_ui_open();
                        goto end_frame;
                    }
                }
                LOG_INFO("[project] loading: %s", s_project.user_dll);
                s_managed_load((void *)s_project.user_dll);
                fw_start(&fw,     s_project.user_watch_dir, on_file_changed,   NULL);
                fw_start(&fw_src, s_project.user_src_dir,   on_source_changed, NULL);
                s_project_loaded = true;
            }
        } else {
            editor_ui_build(&input, (f32)timer.delta);
            /* play state transitions */
            PlayState cur_play = editor_ui_get_play_state();
                if (cur_play != s_prev_play) {
                    if (cur_play == PLAY_STATE_PLAYING && s_prev_play == PLAY_STATE_STOPPED) {
                        LOG_INFO("[main] Play pressed — building user project...");
                        if (!project_build(&s_project)) {
                            LOG_ERROR("[main] build failed — play cancelled");
                            editor_ui_set_play_state(PLAY_STATE_STOPPED);
                            cur_play = PLAY_STATE_STOPPED;
                        } else {
                            LOG_INFO("[main] build ok — reloading and starting");
                            s_managed_unload(NULL);
                            s_managed_load((void *)s_project.user_dll);
                            if (s_managed_start) s_managed_start(NULL);
                        }
                    }
                    if (cur_play == PLAY_STATE_STOPPED && s_prev_play != PLAY_STATE_STOPPED)
                        if (s_managed_stop) s_managed_stop(NULL);
                    s_prev_play = cur_play;
                }

                /* tick game logic only when playing */
                if (cur_play == PLAY_STATE_PLAYING && s_managed_update)
                     s_managed_update((f32)timer.delta);
        }

end_frame:
        gw_renderer_end_frame();
    }

    if (s_project_loaded) {
        fw_stop(&fw);
        fw_stop(&fw_src);
    }
    editor_ui_shutdown();
    gw_renderer_wait_idle();
    gw_renderer_shutdown();
    if (s_project_loaded) s_managed_unload(NULL);
    gw_host_shutdown();
    LOG_INFO("Shutdown clean.");
    gw_window_destroy(&window);
    CoUninitialize();
    logger_shutdown();
    return 0;
}