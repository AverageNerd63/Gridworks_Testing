using System.Text;
using Gridworks.Engine.ECS;

namespace Gridworks.Engine;

public static unsafe class GwLog
{
    private static void Log(int level, string msg)
    {
        var fn = NativeApi.Current.log_user;
        if (fn == null) return;
        byte[] buf = Encoding.UTF8.GetBytes(msg + "\0");
        fixed (byte* p = buf) fn(level, p);
    }

    public static void Info(string msg)  => Log(2, msg);
    public static void Warn(string msg)  => Log(3, msg);
    public static void Error(string msg) => Log(4, msg);
}
