#include "project.h"
#include "../core/logger.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* ---- internal helpers ------------------------------------------------- */

static void make_dirs(const char *path) {
    char tmp[PROJECT_PATH_MAX];
    strncpy(tmp, path, sizeof(tmp) - 1);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            char c = *p; *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = c;
        }
    }
    CreateDirectoryA(tmp, NULL);
}

static void recent_file_path(char *out, int max) {
    char appdata[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);
    snprintf(out, max, "%s\\Gridworks\\recent.txt", appdata);
}

static void engine_exe_dir(char *out, int max) {
    GetModuleFileNameA(NULL, out, max);
    char *sep = strrchr(out, '\\');
    if (sep) *sep = '\0';
}

static void fill_derived(ProjectConfig *cfg) {
    const char *r = cfg->root;
    const char *n = cfg->name;
    snprintf(cfg->proj_file,      sizeof cfg->proj_file,
             "%s\\%s.gwproj", r, n);
    snprintf(cfg->user_src_dir,   sizeof cfg->user_src_dir,
             "%s\\UserProject", r);
    snprintf(cfg->user_csproj,    sizeof cfg->user_csproj,
             "%s\\UserProject\\UserProject.csproj", r);
    snprintf(cfg->user_dll,       sizeof cfg->user_dll,
             "%s\\UserProject\\bin\\Debug\\net9.0\\UserProject.dll", r);
    snprintf(cfg->user_watch_dir, sizeof cfg->user_watch_dir,
             "%s\\UserProject\\bin\\Debug\\net9.0", r);
}

/* ---- project_open ----------------------------------------------------- */

bool project_open(const char *proj_file, ProjectConfig *cfg) {
    FILE *f = fopen(proj_file, "r");
    if (!f) { LOG_ERROR("[project] cannot open: %s", proj_file); return false; }

    memset(cfg, 0, sizeof *cfg);
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *val = eq + 1;
        val[strcspn(val, "\r\n")] = '\0';
        if (!strcmp(line, "name"))
            strncpy(cfg->name, val, PROJECT_NAME_MAX - 1);
    }
    fclose(f);

    strncpy(cfg->proj_file, proj_file, PROJECT_PATH_MAX - 1);
    strncpy(cfg->root, proj_file, PROJECT_PATH_MAX - 1);
    char *sep = strrchr(cfg->root, '\\');
    if (!sep) sep = strrchr(cfg->root, '/');
    if (sep) *sep = '\0';

    fill_derived(cfg);
    LOG_INFO("[project] opened '%s' at %s", cfg->name, cfg->root);
    return true;
}

/* ---- project_new ------------------------------------------------------ */

static bool write_text(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) { LOG_ERROR("[project] write failed: %s", path); return false; }
    fputs(content, f);
    fclose(f);
    return true;
}

bool project_new(const char *root_dir, const char *name, ProjectConfig *cfg) {
    memset(cfg, 0, sizeof *cfg);
    strncpy(cfg->root, root_dir, PROJECT_PATH_MAX - 1);
    strncpy(cfg->name, name,     PROJECT_NAME_MAX - 1);
    fill_derived(cfg);

    make_dirs(cfg->root);
    make_dirs(cfg->user_src_dir);
    make_dirs(cfg->user_watch_dir);

    char assets[PROJECT_PATH_MAX];
    snprintf(assets, sizeof assets, "%s\\Assets", cfg->root);
    make_dirs(assets);

    /* .gwproj */
    char gwproj[256];
    snprintf(gwproj, sizeof gwproj, "name=%s\nversion=1\n", name);
    if (!write_text(cfg->proj_file, gwproj)) return false;

    /* resolve engine dll path from exe location */
    char exe_dir[PROJECT_PATH_MAX];
    engine_exe_dir(exe_dir, sizeof exe_dir);
    char eng_dll[PROJECT_PATH_MAX];
    snprintf(eng_dll, sizeof eng_dll,
             "%s\\managed_src\\Gridworks.Engine\\bin\\Debug\\net9.0\\Gridworks.Engine.dll",
             exe_dir);

    /* UserProject.csproj */
    char csproj[2048];
    snprintf(csproj, sizeof csproj,
        "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
        "  <PropertyGroup>\n"
        "    <OutputType>Library</OutputType>\n"
        "    <TargetFramework>net9.0</TargetFramework>\n"
        "    <AssemblyName>UserProject</AssemblyName>\n"
        "    <Nullable>enable</Nullable>\n"
        "    <ImplicitUsings>enable</ImplicitUsings>\n"
        "  </PropertyGroup>\n"
        "  <ItemGroup>\n"
        "    <Reference Include=\"Gridworks.Engine\">\n"
        "      <HintPath>%s</HintPath>\n"
        "    </Reference>\n"
        "  </ItemGroup>\n"
        "</Project>\n",
        eng_dll);
    if (!write_text(cfg->user_csproj, csproj)) return false;

    /* starter source file */
    const char *cs =
        "using Gridworks.Engine;\n\n"
        "public class GameLogic\n{\n"
        "    public static void Initialize()\n    {\n    }\n}\n";
    char cs_path[PROJECT_PATH_MAX];
    snprintf(cs_path, sizeof cs_path, "%s\\UserProject\\GameLogic.cs", cfg->root);
    if (!write_text(cs_path, cs)) return false;

    LOG_INFO("[project] created '%s' at %s", name, root_dir);
    return true;
}

/* ---- recent projects -------------------------------------------------- */

void recent_load(char out[][PROJECT_PATH_MAX], int *count) {
    *count = 0;
    char path[MAX_PATH];
    recent_file_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[PROJECT_PATH_MAX];
    while (*count < RECENT_MAX && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0]) strncpy(out[(*count)++], line, PROJECT_PATH_MAX - 1);
    }
    fclose(f);
}

void recent_add(const char *proj_file) {
    char existing[RECENT_MAX][PROJECT_PATH_MAX];
    int  count = 0;
    recent_load(existing, &count);

    char merged[RECENT_MAX][PROJECT_PATH_MAX];
    int  mc = 0;
    strncpy(merged[mc++], proj_file, PROJECT_PATH_MAX - 1);
    for (int i = 0; i < count && mc < RECENT_MAX; i++)
        if (_stricmp(existing[i], proj_file) != 0)
            strncpy(merged[mc++], existing[i], PROJECT_PATH_MAX - 1);

    char path[MAX_PATH];
    recent_file_path(path, sizeof path);

    /* ensure %APPDATA%\Gridworks exists */
    char appdata[MAX_PATH], gw_dir[MAX_PATH];
    GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);
    snprintf(gw_dir, sizeof gw_dir, "%s\\Gridworks", appdata);
    CreateDirectoryA(gw_dir, NULL);

    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < mc; i++) fprintf(f, "%s\n", merged[i]);
    fclose(f);
}