#pragma once
#include "../project/project.h"
#include <stdbool.h>

void project_ui_open(void);
bool project_ui_draw(ProjectConfig *cfg); /* true when project selected */