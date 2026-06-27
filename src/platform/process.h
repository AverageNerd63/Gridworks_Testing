#pragma once

/* Fire-and-forget: run cmd in a detached background process. Returns immediately. */
void gw_process_spawn(const char *cmd);