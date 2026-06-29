using System.Runtime.InteropServices;

namespace Gridworks.Engine.ECS;

/* Layout must match GwApi in ecs_export.h exactly. */
[StructLayout(LayoutKind.Sequential)]
public unsafe struct GwApi
{
    /* ECS */
    public delegate* unmanaged[Cdecl]<void*>                           world_create;
    public delegate* unmanaged[Cdecl]<void*, void>                     world_destroy;
    public delegate* unmanaged[Cdecl]<void*, nuint, uint>              register_component;
    public delegate* unmanaged[Cdecl]<void*, uint>                     entity_create;
    public delegate* unmanaged[Cdecl]<void*, uint, void>               entity_destroy;
    public delegate* unmanaged[Cdecl]<void*, uint, byte>               entity_alive;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, void*, void*> component_add;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, void>         component_remove;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, void*>        component_get;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, byte>         component_has;
    public delegate* unmanaged[Cdecl]<void*, uint, uint*, void*>       view;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, uint>         view_entity;
    /* Logging */
    public delegate* unmanaged[Cdecl]<int, byte*, void>                log_user;
}

public static class NativeApi
{
    public static unsafe GwApi Current;
}