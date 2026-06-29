#include "imgui.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <string.h>
#include <stdio.h>

extern "C" {
#include "project_ui.h"
#include "../core/logger.h"
}

static bool s_visible = false;
static bool s_new_mode = false;

static char s_recent[RECENT_MAX][PROJECT_PATH_MAX];
static int  s_recent_count = 0;

static char s_new_folder[PROJECT_PATH_MAX] = {0};
static char s_new_name[PROJECT_NAME_MAX]   = {0};
static char s_error[256]                   = {0};

/* ---- native pickers --------------------------------------------------- */

static bool pick_folder(char *out, int max) {
    IFileOpenDialog *dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg))))
        return false;

    DWORD opts;
    dlg->GetOptions(&opts);
    dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dlg->SetTitle(L"Select project folder");

    bool ok = false;
    if (SUCCEEDED(dlg->Show(nullptr))) {
        IShellItem *item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR wpath = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &wpath))) {
                WideCharToMultiByte(CP_ACP, 0, wpath, -1, out, max, nullptr, nullptr);
                CoTaskMemFree(wpath);
                ok = true;
            }
            item->Release();
        }
    }
    dlg->Release();
    return ok;
}

static bool pick_gwproj(char *out, int max) {
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.lpstrFilter    = "Gridworks Project\0*.gwproj\0All Files\0*.*\0";
    ofn.lpstrFile      = out;
    ofn.nMaxFile       = (DWORD)max;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt    = "gwproj";
    ofn.lpstrTitle     = "Open Gridworks Project";
    return (bool)GetOpenFileNameA(&ofn);
}

/* ---- public API ------------------------------------------------------- */

void project_ui_open(void) {
    s_visible    = true;
    s_new_mode   = false;
    s_error[0]   = '\0';
    recent_load(s_recent, &s_recent_count);
}

bool project_ui_draw(ProjectConfig *cfg) {
    if (!s_visible) return false;

    ImGuiViewport *mv = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mv->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(580, 420), ImGuiCond_Always);

    constexpr ImGuiWindowFlags wf =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Gridworks — Project Manager", nullptr, wf)) {
        ImGui::End();
        return false;
    }

    bool result = false;

    if (!s_new_mode) {
        /* ---- main panel ----------------------------------------------- */
        ImGui::Spacing();
        if (ImGui::Button("New Project", ImVec2(160, 0))) {
            s_new_mode      = true;
            s_new_folder[0] = '\0';
            s_new_name[0]   = '\0';
            s_error[0]      = '\0';
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Project...", ImVec2(160, 0))) {
            char path[PROJECT_PATH_MAX] = {0};
            if (pick_gwproj(path, sizeof path)) {
                if (project_open(path, cfg)) {
                    recent_add(cfg->proj_file);
                    s_visible = false;
                    result    = true;
                } else {
                    snprintf(s_error, sizeof s_error, "Failed to open: %s", path);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Recent Projects");
        ImGui::Spacing();

        ImGui::BeginChild("##recent", ImVec2(0, -30), true);
        if (s_recent_count == 0) {
            ImGui::TextDisabled("(no recent projects)");
        }
        for (int i = 0; i < s_recent_count; i++) {
            ImGui::PushID(i);
            /* derive display name: strip to last path component */
            const char *proj   = s_recent[i];
            const char *fname  = strrchr(proj, '\\');
            fname = fname ? fname + 1 : proj;
            /* strip .gwproj for display */
            char label[PROJECT_NAME_MAX];
            strncpy(label, fname, sizeof label - 1);
            char *dot = strrchr(label, '.');
            if (dot) *dot = '\0';

            if (ImGui::Selectable(label, false, 0, ImVec2(0, 0))) {
                if (project_open(proj, cfg)) {
                    recent_add(proj);
                    s_visible = false;
                    result    = true;
                } else {
                    snprintf(s_error, sizeof s_error,
                             "Project not found: %s", proj);
                    recent_remove(proj);
                    recent_load(s_recent, &s_recent_count);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", proj);
            ImGui::PopID();
        }
        ImGui::EndChild();

        if (s_error[0]) {
            ImGui::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "%s", s_error);
        }

    } else {
        /* ---- new project panel ---------------------------------------- */
        ImGui::Spacing();
        ImGui::Text("Project Name");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##pname", s_new_name, sizeof s_new_name);

        ImGui::Spacing();
        ImGui::Text("Location");
        ImGui::SetNextItemWidth(-130);
        ImGui::InputText("##pfolder", s_new_folder, sizeof s_new_folder,
                         ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("Browse...", ImVec2(120, 0)))
            pick_folder(s_new_folder, sizeof s_new_folder);

        if (s_new_folder[0] && s_new_name[0]) {
            ImGui::Spacing();
            ImGui::TextDisabled("Path: %s\\%s", s_new_folder, s_new_name);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool can_create = s_new_folder[0] != '\0' && s_new_name[0] != '\0';
        if (!can_create) ImGui::BeginDisabled();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            char root[PROJECT_PATH_MAX];
            snprintf(root, sizeof root, "%s\\%s", s_new_folder, s_new_name);
            if (project_new(root, s_new_name, cfg)) {
                recent_add(cfg->proj_file);
                s_visible = false;
                result    = true;
            } else {
                snprintf(s_error, sizeof s_error,
                         "Failed to create project '%s'", s_new_name);
            }
        }
        if (!can_create) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Back", ImVec2(120, 0))) {
            s_new_mode = false;
            s_error[0] = '\0';
        }

        if (s_error[0]) {
            ImGui::Spacing();
            ImGui::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "%s", s_error);
        }
    }

    ImGui::End();
    return result;
}