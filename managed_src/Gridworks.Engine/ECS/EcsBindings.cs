using System.Runtime.InteropServices;

namespace Gridworks.Engine.ECS;

/* Layout must match GwEcsApi in ecs_export.h exactly (12 × 8-byte fn pointers). */
[StructLayout(LayoutKind.Sequential)]
public unsafe struct GwEcsApi
{
    public delegate* unmanaged[Cdecl]<void*>                           world_create;
    public delegate* unmanaged[Cdecl]<void*, void>                     world_destroy;
    public delegate* unmanaged[Cdecl]<void*, nuint, uint>              register_component;
    public delegate* unmanaged[Cdecl]<void*, uint>                     entity_create;
    public delegate* unmanaged[Cdecl]<void*, uint, void>               entity_destroy;
    public delegate* unmanaged[Cdecl]<void*, uint, byte>               entity_alive;    // C bool → byte
    public delegate* unmanaged[Cdecl]<void*, uint, uint, void*, void*> component_add;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, void>         component_remove;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, void*>        component_get;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, byte>         component_has;   // C bool → byte
    public delegate* unmanaged[Cdecl]<void*, uint, uint*, void*>       view;
    public delegate* unmanaged[Cdecl]<void*, uint, uint, uint>         view_entity;
}

/* Set once by EngineHost.Initialize before any World is created. */
public static class NativeEcsApi
{
    public static unsafe GwEcsApi Current;
}