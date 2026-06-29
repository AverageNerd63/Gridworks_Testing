using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Gridworks.Engine.ECS;

public sealed unsafe class World : IDisposable
{
    private void* _ptr;
    private readonly Dictionary<Type, uint> _ids = new();
    private bool _disposed;

    public static World Create()
    {
        var w = new World();
        w._ptr = NativeApi.Current.world_create();
        return w;
    }

    public uint Register<T>() where T : unmanaged
    {
        uint cid = NativeApi.Current.register_component(_ptr, (nuint)sizeof(T));
        _ids[typeof(T)] = cid;
        return cid;
    }

    public uint ComponentId<T>() where T : unmanaged =>
        _ids.TryGetValue(typeof(T), out uint id) ? id : uint.MaxValue;

    public uint   CreateEntity()        => NativeApi.Current.entity_create(_ptr);
    public void   DestroyEntity(uint e) => NativeApi.Current.entity_destroy(_ptr, e);
    public bool   IsEntityAlive(uint e) => NativeApi.Current.entity_alive(_ptr, e) != 0;

    public void Add<T>(uint e, T value) where T : unmanaged
    {
        uint cid = ComponentId<T>();
        NativeApi.Current.component_add(_ptr, e, cid, &value);
    }

    public void   Remove<T>(uint e) where T : unmanaged =>
        NativeApi.Current.component_remove(_ptr, e, ComponentId<T>());

    public ref T  Get<T>(uint e) where T : unmanaged =>
        ref *(T*)NativeApi.Current.component_get(_ptr, e, ComponentId<T>());

    public bool   Has<T>(uint e) where T : unmanaged =>
        NativeApi.Current.component_has(_ptr, e, ComponentId<T>()) != 0;

    public Span<T> View<T>() where T : unmanaged
    {
        uint cid = ComponentId<T>();
        if (cid == uint.MaxValue) return Span<T>.Empty;
        uint count;
        void* data = NativeApi.Current.view(_ptr, cid, &count);
        return data == null ? Span<T>.Empty : new Span<T>(data, (int)count);
    }

    public uint ViewEntity<T>(uint i) where T : unmanaged =>
        NativeApi.Current.view_entity(_ptr, ComponentId<T>(), i);

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        NativeApi.Current.world_destroy(_ptr);
        _ptr = null;
    }
}