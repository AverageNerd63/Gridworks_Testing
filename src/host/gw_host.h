#pragma once
#include "../core/types.h"
#include <stdbool.h>

bool gw_host_init(const char *runtime_config_path);
void gw_host_shutdown(void);

/* type_name must be "Full.Type.Name, AssemblyName".
   Method must have [UnmanagedCallersOnly(CallConvs={Cdecl})]. */
bool gw_host_get_fn(const char *assembly_path,
                    const char *type_name,
                    const char *method_name,
                    void      **out_fn);