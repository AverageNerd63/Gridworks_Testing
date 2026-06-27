using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.IO;
using Gridworks.Engine.ECS;

namespace Gridworks.Engine.Host;

public static unsafe class EngineHost
{
    private static WeakReference<AssemblyLoadContext>? _alcRef;

[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
public static void Initialize(void* apiPtr)
{
    try
    {
        NativeEcsApi.Current = *(GwEcsApi*)apiPtr;
        Console.WriteLine("[managed] EngineHost initialized — ECS API bound");
    }
    catch (Exception ex) { Console.WriteLine($"[managed] Initialize error: {ex}"); }
}

[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
public static void LoadUserAssembly(void* pathPtr)
{
    try
    {
        string path = Path.GetFullPath(
            Marshal.PtrToStringUTF8((nint)pathPtr)
            ?? throw new ArgumentNullException(nameof(pathPtr)));

        // Shadow-copy so the original file stays unlocked for dotnet build to overwrite
        string shadowDir = Path.Combine(Path.GetTempPath(), "gw_shadow");
        Directory.CreateDirectory(shadowDir);
        string shadowPath = Path.Combine(shadowDir, Path.GetFileName(path));
        File.Copy(path, shadowPath, overwrite: true);

        // Resolver points to original path so transitive deps resolve correctly
        var alc = new CollectibleGameContext(path);
        _alcRef = new WeakReference<AssemblyLoadContext>(alc);

        Assembly asm = alc.LoadFromAssemblyPath(shadowPath);
        Console.WriteLine($"[managed] loaded {asm.GetName().Name} into collectible ALC");

        Type? gameType = asm.GetType("UserProject.Game");
        MethodInfo? start = gameType?.GetMethod("Start",
            BindingFlags.Public | BindingFlags.Static);

        if (start != null)
            start.Invoke(null, null);
        else
            Console.WriteLine("[managed] warning: UserProject.Game.Start() not found");
    }
    catch (Exception ex) { Console.WriteLine($"[managed] LoadUserAssembly error: {ex}"); }
}

[UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
public static void UnloadUserAssembly(void* unused)
{
    try
    {
        if (_alcRef == null) return;

        if (_alcRef.TryGetTarget(out AssemblyLoadContext? alc))
        {
            alc.Unload();
            Console.WriteLine("[managed] ALC unloading — waiting for GC");
        }

        for (int i = 0; i < 10 && _alcRef.TryGetTarget(out AssemblyLoadContext? __); i++)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        Console.WriteLine(_alcRef.TryGetTarget(out AssemblyLoadContext? ___)
            ? "[managed] warning: ALC not collected after 10 GC cycles"
            : "[managed] ALC collected — ready for reload");

        _alcRef = null;
    }
    catch (Exception ex) { Console.WriteLine($"[managed] UnloadUserAssembly error: {ex}"); }
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
        string? path = _resolver.ResolveAssemblyToPath(name);
        return path != null ? LoadFromAssemblyPath(path) : null;
    }
}