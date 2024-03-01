namespace UnrealSharpBuildTool;
public static class DotNetPathFinder
{ 
    public static string? FindDotNetExecutable()
    {
        var pathVariable = Environment.GetEnvironmentVariable("PATH");
        
        if (pathVariable == null)
        {
            return null;
        }
        
        var paths = pathVariable.Split(Path.PathSeparator);
        
        foreach (var path in paths)
        {
            string fullPath;
            
            if (OperatingSystem.IsWindows())
            {
                fullPath = Path.Combine(path, "dotnet.exe");
            }
            else
            {
                fullPath = Path.Combine(path, "dotnet");
            }

            if (File.Exists(fullPath))
            {
                return fullPath;
            }
        }

        throw new Exception("Couldn't find dotnet.exe!");
    }
}