using System;
using System.Collections.Generic;
using System.Reflection;

namespace Gridworks.Engine.Host;

internal sealed class BehaviourRunner
{
    private GwBehaviour[] _behaviours = [];
    public int Count => _behaviours.Length;
    private float _fixedAccum;
    private const float FixedStep = 0.02f;

    public void Start(Assembly asm)
    {
        var list = new List<GwBehaviour>();
        foreach (Type t in asm.GetTypes())
        {
            if (t.IsAbstract || !t.IsSubclassOf(typeof(GwBehaviour))) continue;
            try { list.Add((GwBehaviour)Activator.CreateInstance(t)!); }
            catch (Exception ex) { Console.Error.WriteLine($"[runner] create {t.Name}: {ex.Message}"); }
        }
        _behaviours = list.ToArray();

        foreach (var b in _behaviours) SafeCall(b, static x => x.Awake(), "Awake");
        foreach (var b in _behaviours) if (b.Enabled) SafeCall(b, static x => x.Start(), "Start");
    }

    public void Update(float dt)
    {
        _fixedAccum += dt;
        while (_fixedAccum >= FixedStep)
        {
            foreach (var b in _behaviours)
                if (b.Enabled) SafeCall(b, static x => x.FixedUpdate(), "FixedUpdate");
            _fixedAccum -= FixedStep;
        }
        foreach (var b in _behaviours) if (b.Enabled) SafeCall(b, x => x.Update(dt), "Update");
        foreach (var b in _behaviours) if (b.Enabled) SafeCall(b, x => x.LateUpdate(dt), "LateUpdate");
    }

    public void Stop()
    {
        foreach (var b in _behaviours) SafeCall(b, static x => x.OnDestroy(), "OnDestroy");
        _behaviours = [];
        _fixedAccum = 0f;
    }

    private static void SafeCall(GwBehaviour b, Action<GwBehaviour> fn, string name)
    {
        try { fn(b); }
        catch (Exception ex) { Console.Error.WriteLine($"[runner] {b.GetType().Name}.{name}: {ex.Message}"); }
    }
}
