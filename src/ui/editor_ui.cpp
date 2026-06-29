#include "imgui.h"
#include "imgui_internal.h"
#include "../../third_party/ImGuizmo/ImGuizmo.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
#include "editor_ui.h"
#include "../core/logger.h"
#include "../editor/viewport_camera.h"
#include "../editor/undo.h"
#include "../editor/selection.h"
#include "../platform/filesystem.h"
#include "../math/gw_math.h"
}

extern "C" void *scene_pass_get_imgui_id(void);
extern "C" void  scene_pass_request_resize(unsigned int w, unsigned int h);
extern "C" void  vk_set_camera_vp(const float *mat16);

/* ---- console ---------------------------------------------------------- */

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
            s_console.colors[idx] = {1.0f, 0.8f, 0.0f, 1.0f}; break;
        case LOG_LEVEL_ERROR:
        case LOG_LEVEL_FATAL:
            s_console.colors[idx] = {1.0f, 0.3f, 0.3f, 1.0f}; break;
        default:
            s_console.colors[idx] = {0.85f, 0.85f, 0.85f, 1.0f}; break;
    }
    s_console.scroll_to_bottom = true;
}

/* ---- ini -------------------------------------------------------------- */

static char s_ini_default[MAX_PATH];
static char s_ini_user[MAX_PATH];

static void init_ini_paths(void) {
    CreateDirectoryA("config", NULL);
    snprintf(s_ini_default, MAX_PATH, "config\\editor_layout_default.ini");

    char appdata[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);
    char user_dir[MAX_PATH];
    snprintf(user_dir, MAX_PATH, "%s\\Gridworks", appdata);
    CreateDirectoryA(user_dir, NULL);
    snprintf(s_ini_user, MAX_PATH, "%s\\editor_layout_user.ini", user_dir);

    LOG_INFO("[editor] default ini: %s", s_ini_default);
    LOG_INFO("[editor] user ini:    %s", s_ini_user);
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

/* ---- editor state ----------------------------------------------------- */

static ViewportCamera    s_camera;
static SelectionState    s_selection;
static UndoStack         s_undo;
static const InputState *s_input = nullptr;
static f32               s_dt    = 0.0f;

static Mat4              s_object_matrix;
static Mat4              s_gizmo_xform_before;

static ImGuizmo::OPERATION s_gizmo_op   = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE      s_gizmo_mode = ImGuizmo::WORLD;
static bool                s_gizmo_was_using = false;

/* ---- gizmo undo ------------------------------------------------------- */

struct GizmoXformData { Mat4 before; Mat4 after; };

static void gizmo_exec(void *d) {
    s_object_matrix = ((GizmoXformData *)d)->after;
}
static void gizmo_revert(void *d) {
    s_object_matrix = ((GizmoXformData *)d)->before;
}
static void gizmo_free(void *d) { free(d); }

/* ---- viewport panel --------------------------------------------------- */

static void panel_viewport(void) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    ImVec2 vp_origin = ImGui::GetCursorScreenPos();
    ImVec2 size      = ImGui::GetContentRegionAvail();
    if (size.x < 1.0f) size.x = 1.0f;
    if (size.y < 1.0f) size.y = 1.0f;

    scene_pass_request_resize((unsigned int)size.x, (unsigned int)size.y);

    bool win_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (s_input)
        viewport_camera_update(&s_camera, s_input, s_dt, win_hovered);

    Mat4 view, proj;
    viewport_camera_get_vp(&s_camera, size.x / size.y, &view, &proj);

    /* VP for ray casting and gizmo setup */
    Mat4 vp = mat4_mul(proj, view);

    /* push MVP (proj * view * model) so the triangle reflects gizmo transforms */
    Mat4 mvp = mat4_mul(vp, s_object_matrix);
    vk_set_camera_vp(&mvp.m[0][0]);

    /* ImGuizmo needs the un-flipped projection — it does its own Y-flip internally */
    Mat4 proj_imgui  = proj;
    proj_imgui.m[1][1] = -proj_imgui.m[1][1];

    /* scene image */
    void *tex_id = scene_pass_get_imgui_id();
    if (tex_id) {
        ImGui::Image((ImTextureID)tex_id, size);
    } else {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(p, {p.x + size.x, p.y + size.y}, IM_COL32(30, 30, 46, 255));
        dl->AddText({p.x + 8, p.y + 8}, IM_COL32(120, 120, 140, 255), "No scene texture");
    }

    bool item_hovered = ImGui::IsItemHovered();

    /* left-click pick */
    if (item_hovered && s_input
        && input_button_pressed(s_input, GW_MOUSE_LEFT)
        && !ImGuizmo::IsOver()) {
        Mat4 inv_vp = mat4_invert(vp);
        f32  mx     = (f32)s_input->mouse_x - vp_origin.x;
        f32  my     = (f32)s_input->mouse_y - vp_origin.y;
        Ray  ray    = ray_from_screen(inv_vp, mx, my, size.x, size.y);

        f32 t;
        if (ray_intersect_sphere(ray, vec3(0.0f, 0.0f, 0.0f), 0.6f, &t))
            selection_set(&s_selection, 0);
        else
            selection_clear(&s_selection);
    }

    /* gizmo */
    if (selection_has(&s_selection)) {
        if (!s_gizmo_was_using)
        s_gizmo_xform_before = s_object_matrix;

        /* scale is always local-space in ImGuizmo */
        ImGuizmo::MODE effective_mode = (s_gizmo_op == ImGuizmo::SCALE)
            ? ImGuizmo::LOCAL
            : s_gizmo_mode;

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(vp_origin.x, vp_origin.y, size.x, size.y);

        ImGuizmo::Manipulate(
            &view.m[0][0], &proj_imgui.m[0][0],
            s_gizmo_op, effective_mode,
            &s_object_matrix.m[0][0]);

        bool is_using = ImGuizmo::IsUsing();
if (s_gizmo_was_using && !is_using) {
    GizmoXformData *data = (GizmoXformData *)malloc(sizeof(GizmoXformData));
    data->before = s_gizmo_xform_before;
    data->after  = s_object_matrix;
    UndoCmd cmd  = { gizmo_exec, gizmo_revert, gizmo_free, data };
    undo_stack_push_no_exec(&s_undo, cmd);
}
    s_gizmo_was_using = is_using;
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

/* ---- hierarchy panel -------------------------------------------------- */

static void panel_hierarchy(void) {
    ImGui::Begin("Hierarchy");

    bool selected = selection_has(&s_selection);
    if (ImGui::Selectable("Triangle", selected)) {
        if (selected)
            selection_clear(&s_selection);
        else
            selection_set(&s_selection, 0);
    }

    ImGui::End();
}

/* ---- inspector panel -------------------------------------------------- */

static void panel_inspector(void) {
    ImGui::Begin("Inspector");

    if (selection_has(&s_selection)) {
        ImGui::Text("Entity %d", selection_get(&s_selection));
        ImGui::Separator();

        float tra[3], rot[3], sca[3];
        ImGuizmo::DecomposeMatrixToComponents(&s_object_matrix.m[0][0], tra, rot, sca);

        ImGui::Text("Translation  %.2f  %.2f  %.2f", tra[0], tra[1], tra[2]);
        ImGui::Text("Rotation     %.1f  %.1f  %.1f", rot[0], rot[1], rot[2]);
        ImGui::Text("Scale        %.2f  %.2f  %.2f", sca[0], sca[1], sca[2]);
    } else {
        ImGui::TextDisabled("(no entity selected)");
    }

    ImGui::End();
}

/* ---- console panel ---------------------------------------------------- */

static void panel_console(void) {
    ImGui::Begin("Console");

    if (ImGui::SmallButton("Clear"))
        s_console.head = s_console.count = 0;

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
        if (ImGui::MenuItem("Reset to Default")) s_apply_default = true;
        if (ImGui::MenuItem("Save Layout"))
            ImGui::SaveIniSettingsToDisk(s_ini_user);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Gizmo")) {
        if (ImGui::MenuItem("Translate",     "W", s_gizmo_op == ImGuizmo::TRANSLATE))
            s_gizmo_op = ImGuizmo::TRANSLATE;
        if (ImGui::MenuItem("Rotate",        "E", s_gizmo_op == ImGuizmo::ROTATE))
            s_gizmo_op = ImGuizmo::ROTATE;
        if (ImGui::MenuItem("Scale (local)", "R", s_gizmo_op == ImGuizmo::SCALE))
            s_gizmo_op = ImGuizmo::SCALE;
        ImGui::Separator();
        if (ImGui::MenuItem("World", nullptr, s_gizmo_mode == ImGuizmo::WORLD))
            s_gizmo_mode = ImGuizmo::WORLD;
        if (ImGui::MenuItem("Local", nullptr, s_gizmo_mode == ImGuizmo::LOCAL))
            s_gizmo_mode = ImGuizmo::LOCAL;
        ImGui::EndMenu();
    }

    /* ---- centered play controls ---------------------------------------- */
    float bar_w   = ImGui::GetWindowWidth();
    float btn_w   = 64.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float group_w = btn_w * 3 + spacing * 2;
    ImGui::SetCursorPosX((bar_w - group_w) * 0.5f);

    bool playing = s_play_state == PLAY_STATE_PLAYING;
    bool paused  = s_play_state == PLAY_STATE_PAUSED;
    bool stopped = s_play_state == PLAY_STATE_STOPPED;

    /* Play */
    if (playing)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.65f, 0.2f, 1.0f));
    if (!stopped) ImGui::BeginDisabled();
    if (ImGui::Button("Play", ImVec2(btn_w, 0)))
        s_play_state = PLAY_STATE_PLAYING;
    if (!stopped) ImGui::EndDisabled();
    if (playing) ImGui::PopStyleColor();

    ImGui::SameLine();

    /* Pause */
    if (paused)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.55f, 0.1f, 1.0f));
    if (stopped) ImGui::BeginDisabled();
    if (ImGui::Button("Pause", ImVec2(btn_w, 0)))
        s_play_state = paused ? PLAY_STATE_PLAYING : PLAY_STATE_PAUSED;
    if (stopped) ImGui::EndDisabled();
    if (paused) ImGui::PopStyleColor();

    ImGui::SameLine();

    /* Stop */
    if (stopped) ImGui::BeginDisabled();
    if (ImGui::Button("Stop", ImVec2(btn_w, 0)))
        s_play_state = PLAY_STATE_STOPPED;
    if (stopped) ImGui::EndDisabled();

    ImGui::EndMainMenuBar();
}

/* ---- public API ------------------------------------------------------- */

extern "C" bool editor_ui_init(void) {
    memset(&s_console, 0, sizeof(s_console));

    init_ini_paths();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    if (fs_exists(s_ini_user))
        ImGui::LoadIniSettingsFromDisk(s_ini_user);
    else if (fs_exists(s_ini_default))
        ImGui::LoadIniSettingsFromDisk(s_ini_default);
    else
        s_apply_default = true;

    viewport_camera_init(&s_camera);
    selection_init(&s_selection);
    undo_stack_init(&s_undo);
    s_object_matrix = mat4_identity();

    logger_add_sink(editor_console_push, nullptr);
    LOG_INFO("[editor] UI initialized");
    return true;
}

extern "C" void editor_ui_shutdown(void) {
    undo_stack_clear(&s_undo);
    ImGui::SaveIniSettingsToDisk(s_ini_user);
    LOG_INFO("[editor] layout saved to %s", s_ini_user);
    logger_remove_sink(editor_console_push);
}

extern "C" void editor_ui_build(const InputState *input, f32 dt) {
    s_input = input;
    s_dt    = dt;

    /* global shortcuts */
    ImGuiIO &io = ImGui::GetIO();

    /* use KeyCtrl + IsKeyPressed — more reliable than Shortcut() in docking branch */
    bool ctrl = io.KeyCtrl
         || ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
         || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) undo_stack_undo(&s_undo);
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) undo_stack_redo(&s_undo);

    if (!io.WantCaptureKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) s_gizmo_op = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) s_gizmo_op = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) s_gizmo_op = ImGuizmo::SCALE;
    }

    menu_bar();

    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar          | ImGuiWindowFlags_NoCollapse      |
        ImGuiWindowFlags_NoResize            | ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus    |
        ImGuiWindowFlags_NoDocking           | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0, 0));
    ImGui::Begin("##dockspace_host", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dock_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    if (s_apply_default) {
        build_default_layout(dock_id, vp->WorkSize);
        if (!fs_exists(s_ini_default))
            ImGui::SaveIniSettingsToDisk(s_ini_default);
        s_apply_default = false;
    }

    ImGui::End();

    panel_viewport();
    panel_hierarchy();
    panel_inspector();
    panel_console();
}