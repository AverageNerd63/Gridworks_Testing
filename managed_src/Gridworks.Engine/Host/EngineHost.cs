using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.IO;
using Gridworks.Engine.ECS;

namespace Gridworks.Engine.Host;

internal static class EngineState
{
    internal static readonly BehaviourRunner Runner = new BehaviourRunner();
    internal static WeakReference<AssemblyLoadContext>? AlcRef;
    internal static Assembly? UserAssembly;
}

public static unsafe class EngineHost
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void Initialize(void* apiPtr)
    {
        try { NativeApi.Current = *(GwApi*)apiPtr; }
        catch (Exception ex) { Console.Error.WriteLine($"[managed] Initialize error: {ex}"); }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void LoadUserAssembly(void* pathPtr)
    {
        try
        {
            string path = Path.GetFullPath(
                Marshal.PtrToStringUTF8((nint)pathPtr)
                ?? throw new ArgumentNullException(nameof(pathPtr)));

            string shadowDir  = Path.Combine(Path.GetTempPath(), "gw_shadow");
            Directory.CreateDirectory(shadowDir);
            string baseName   = Path.GetFileNameWithoutExtension(path);
            string shadowPath = Path.Combine(shadowDir,
                $"{baseName}_{Environment.TickCount64:X}.dll");
            File.Copy(path, shadowPath, overwrite: false);

            var alc = new CollectibleGameContext(path);
            EngineState.AlcRef       = new WeakReference<AssemblyLoadContext>(alc);
            EngineState.UserAssembly = alc.LoadFromAssemblyPath(shadowPath);
        }
        catch (Exception ex) { Console.Error.WriteLine($"[managed] LoadUserAssembly error: {ex}"); }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void StartGame(void* _)
    {
        try
        {
            if (EngineState.UserAssembly != null)
                EngineState.Runner.Start(EngineState.UserAssembly);
        }
        catch (Exception ex) { Console.Error.WriteLine($"[managed] StartGame error: {ex}"); }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void UpdateGame(float dt)
    {
        try { EngineState.Runner.Update(dt); }
        catch (Exception ex) { Console.Error.WriteLine($"[managed] UpdateGame error: {ex}"); }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void StopGame(void* _)
    {
        try { EngineState.Runner.Stop(); }
        catch (Exception ex) { Console.Error.WriteLine($"[managed] StopGame error: {ex}"); }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void UnloadUserAssembly(void* unused)
    {
        try
        {
            EngineState.Runner.Stop();
            EngineState.UserAssembly = null;

            if (EngineState.AlcRef?.TryGetTarget(out AssemblyLoadContext? alc) == true)
                alc.Unload();

            EngineState.AlcRef = null;
        }
        catch (Exception ex) { Console.Error.WriteLine($"[managed] UnloadUserAssembly error: {ex}"); }
    }
}

file sealed class CollectibleGameContext : AssemblyLoadContext
{
    private readonly AssemblyDependencyResolver _resolver;

    public CollectibleGameContext(string assemblyPath)
        : base(name: "UserGame", isCollectible: true)
    {
        _resolver = new AssemblyDependencyResolver(assemblyPath);
    }

    protected override Assembly? Load(AssemblyName name)
    {
        if (name.Name == "Gridworks.Engine")
            return typeof(EngineHost).Assembly;
        string? path = _resolver.ResolveAssemblyToPath(name);
        return path != null ? LoadFromAssemblyPath(path) : null;
    }
}
