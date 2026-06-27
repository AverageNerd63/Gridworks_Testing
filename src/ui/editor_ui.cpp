#include "imgui.h"
#include "imgui_internal.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "editor_ui.h"
#include "../core/logger.h"
}

/* ---- console ring buffer ---------------------------------------------- */

#define CONSOLE_CAP      512
#define CONSOLE_LINE_MAX 256

static struct {
    char   lines[CONSOLE_CAP][CONSOLE_LINE_MAX];
    ImVec4 colors[CONSOLE_CAP];
    u32    head, count;
    bool   scroll_to_bottom;
} s_console;

extern "C" void editor_console_push(LogLevel level, const char *line, void *) {
    u32 idx = (s_console.head + s_console.count) % CONSOLE_CAP;
    if (s_console.count == CONSOLE_CAP)
        s_console.head = (s_console.head + 1) % CONSOLE_CAP;
    else
        s_console.count++;

    strncpy(s_console.lines[idx], line, CONSOLE_LINE_MAX - 1);
    s_console.lines[idx][CONSOLE_LINE_MAX - 1] = '\0';

    switch (level) {
        case LOG_LEVEL_WARN:
            s_console.colors[idx] = { 1.0f, 0.8f, 0.0f, 1.0f }; break;
        case LOG_LEVEL_ERROR:
        case LOG_LEVEL_FATAL:
            s_console.colors[idx] = { 1.0f, 0.3f, 0.3f, 1.0f }; break;
        default:
            s_console.colors[idx] = { 0.85f, 0.85f, 0.85f, 1.0f }; break;
    }
    s_console.scroll_to_bottom = true;
}

/* ---- ini paths -------------------------------------------------------- */

static char s_ini_default[MAX_PATH];
static char s_ini_user[MAX_PATH];

static void init_ini_paths(void) {
    /* default layout: config/ relative to working directory (commit this) */
    CreateDirectoryA("config", NULL);
    snprintf(s_ini_default, MAX_PATH, "config\\editor_layout_default.ini");

    /* user layout: %APPDATA%\Gridworks\ (per-user, not committed) */
    char appdata[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);

    char user_dir[MAX_PATH];
    snprintf(user_dir, MAX_PATH, "%s\\Gridworks", appdata);
    CreateDirectoryA(user_dir, NULL);

    snprintf(s_ini_user, MAX_PATH, "%s\\editor_layout_user.ini", user_dir);

    LOG_INFO("[editor] default ini: %s", s_ini_default);
    LOG_INFO("[editor] user ini:    %s", s_ini_user);
}

static bool file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    fclose(f);
    return true;
}

/* ---- layout ----------------------------------------------------------- */

static bool s_apply_default = false;

static void build_default_layout(ImGuiID dock_id, ImVec2 size) {
    ImGui::DockBuilderRemoveNode(dock_id);
    ImGui::DockBuilderAddNode(dock_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dock_id, size);

    ImGuiID left, center, right, bottom;
    ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Left,  0.18f, &left,   &center);
    ImGui::DockBuilderSplitNode(center,  ImGuiDir_Right, 0.22f, &right,  &center);
    ImGui::DockBuilderSplitNode(center,  ImGuiDir_Down,  0.28f, &bottom, &center);

    ImGui::DockBuilderDockWindow("Hierarchy", left);
    ImGui::DockBuilderDockWindow("Viewport",  center);
    ImGui::DockBuilderDockWindow("Console",   bottom);
    ImGui::DockBuilderDockWindow("Inspector", right);

    ImGui::DockBuilderFinish(dock_id);
}

/* ---- panels ----------------------------------------------------------- */

static void panel_viewport(void) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(p, { p.x + size.x, p.y + size.y }, IM_COL32(30, 30, 46, 255));
    dl->AddText({ p.x + 8, p.y + 8 }, IM_COL32(120, 120, 140, 255),
                "Scene viewport (Phase 7)");
    ImGui::End();
    ImGui::PopStyleVar();
}

static void panel_hierarchy(void) {
    ImGui::Begin("Hierarchy");
    ImGui::TextDisabled("(no scene loaded)");
    ImGui::End();
}

static void panel_inspector(void) {
    ImGui::Begin("Inspector");
    ImGui::TextDisabled("(no entity selected)");
    ImGui::End();
}

static void panel_console(void) {
    ImGui::Begin("Console");
    if (ImGui::SmallButton("Clear")) { s_console.head = s_console.count = 0; }
    ImGui::Separator();
    ImGui::BeginChild("##lines", ImVec2(0, 0), false,
                       ImGuiWindowFlags_HorizontalScrollbar);
    for (u32 i = 0; i < s_console.count; i++) {
        u32 idx = (s_console.head + i) % CONSOLE_CAP;
        ImGui::PushStyleColor(ImGuiCol_Text, s_console.colors[idx]);
        ImGui::TextUnformatted(s_console.lines[idx]);
        ImGui::PopStyleColor();
    }
    if (s_console.scroll_to_bottom) {
        ImGui::SetScrollHereY(1.0f);
        s_console.scroll_to_bottom = false;
    }
    ImGui::EndChild();
    ImGui::End();
}

/* ---- menu bar --------------------------------------------------------- */

static void menu_bar(void) {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("Layout")) {
        if (ImGui::MenuItem("Reset to Default"))
            s_apply_default = true;

        if (ImGui::MenuItem("Save Layout"))
            ImGui::SaveIniSettingsToDisk(s_ini_user);

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

/* ---- public API ------------------------------------------------------- */

extern "C" bool editor_ui_init(void) {
    memset(&s_console, 0, sizeof(s_console));

    init_ini_paths();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    if (file_exists(s_ini_user))
        ImGui::LoadIniSettingsFromDisk(s_ini_user);
    else if (file_exists(s_ini_default))
        ImGui::LoadIniSettingsFromDisk(s_ini_default);
    else
        s_apply_default = true;

    logger_add_sink(editor_console_push, nullptr);
    LOG_INFO("[editor] UI initialized");
    return true;
}

extern "C" void editor_ui_shutdown(void) {
    ImGui::SaveIniSettingsToDisk(s_ini_user);
    LOG_INFO("[editor] layout saved to %s", s_ini_user);
    logger_remove_sink(editor_console_push);
}

extern "C" void editor_ui_build(void) {
    menu_bar();

    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImVec2 pos  = vp->WorkPos;
    ImVec2 size = vp->WorkSize;

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar      | ImGuiWindowFlags_NoCollapse  |
        ImGuiWindowFlags_NoResize        | ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking       | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0, 0));
    ImGui::Begin("##dockspace_host", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dock_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    
    if (s_apply_default) {
        build_default_layout(dock_id, size);
        if (!file_exists(s_ini_default))
            ImGui::SaveIniSettingsToDisk(s_ini_default);
        s_apply_default = false;
    }

    ImGui::End();

    panel_viewport();
    panel_hierarchy();
    panel_inspector();
    panel_console();
} 