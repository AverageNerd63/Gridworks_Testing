#pragma once
#include <stdbool.h>

#define PROJECT_PATH_MAX 512
#define PROJECT_NAME_MAX 128
#define RECENT_MAX       10

typedef struct {
    char root[PROJECT_PATH_MAX];           /* C:\Projects\MyGame                        */
    char name[PROJECT_NAME_MAX];           /* MyGame                                    */
    char proj_file[PROJECT_PATH_MAX];      /* {root}\MyGame.gwproj                      */
    char user_dll[PROJECT_PATH_MAX];       /* {root}\UserProject\bin\Debug\net9.0\*.dll */
    char user_watch_dir[PROJECT_PATH_MAX]; /* {root}\UserProject\bin\Debug\net9.0       */
    char user_src_dir[PROJECT_PATH_MAX];   /* {root}\UserProject                        */
    char user_csproj[PROJECT_PATH_MAX];    /* {root}\UserProject\UserProject.csproj     */
} ProjectConfig;

bool project_open(const char *proj_file, ProjectConfig *cfg);
bool project_new(const char *root_dir, const char *name, ProjectConfig *cfg);

void recent_load(char out[][PROJECT_PATH_MAX], int *count);
void recent_add(const char *proj_file);
void recent_remove(const char *proj_file);

/* blocking initial build — call after project_new or when DLL is missing */
bool project_build(const ProjectConfig *cfg);
void project_set_engine_dll(const char *abs_path);