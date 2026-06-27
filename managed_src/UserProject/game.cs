using System;

namespace UserProject;

public static class Game
{
    public static void Start()
    {
        Console.WriteLine($"[game] Start() — version: {typeof(Game).Assembly.GetName().Version}");
        Console.WriteLine($"[game] Start() at {DateTime.Now:HH:mm:ss}");
        Console.WriteLine("Hello World");
        Console.WriteLine("[game] Save this file to trigger hot reload.");
    }
}