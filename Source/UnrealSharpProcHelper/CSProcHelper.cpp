﻿#include "CSProcHelper.h"
#include "UnrealSharpProcHelper.h"
#include "Interfaces/IPluginManager.h"

FString FCSProcHelper::UserManagedProjectName = FString::Printf(TEXT("Managed%s"), FApp::GetProjectName());
FString FCSProcHelper::PluginDirectory = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir());
FString FCSProcHelper::UnrealSharpDirectory = FPaths::Combine(PluginDirectory, "Managed", "UnrealSharp");
FString FCSProcHelper::ScriptFolderDirectory = FPaths::ProjectDir() / "Script";
FString FCSProcHelper::GeneratedClassesDirectory = FPaths::Combine(UnrealSharpDirectory, "UnrealSharp", "Generated");

bool FCSProcHelper::InvokeCommand(const FString& ProgramPath, const FString& Arguments, int32& OutReturnCode, FString& Output, FString* InWorkingDirectory)
{
	double StartTime = FPlatformTime::Seconds();
	FString ProgramName = FPaths::GetBaseFilename(ProgramPath);
	
	if (!FPaths::FileExists(ProgramPath))
	{
		FText DialogText = FText::FromString(FString::Printf(TEXT("Failed to find %s at %s"), *ProgramPath, *ProgramName));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
		return false;
	}
		
	const bool bLaunchDetached = false;
	const bool bLaunchHidden = true;
	const bool bLaunchReallyHidden = bLaunchHidden;
	
	void* ReadPipe;
	void* WritePipe;
	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	FString WorkingDirectory = InWorkingDirectory ? *InWorkingDirectory : FPaths::GetPath(ProgramPath);
	
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*ProgramPath,
	                                                      *Arguments,
	                                                      bLaunchDetached,
	                                                      bLaunchHidden,
	                                                      bLaunchReallyHidden,
	                                                      NULL, 0, *WorkingDirectory, WritePipe, ReadPipe);

	if (!ProcHandle.IsValid())
	{
		FString DialogText = FString::Printf(TEXT("%s failed to launch!"), *ProgramName);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(DialogText));
		return false;
	}
	
	while (FPlatformProcess::IsProcRunning(ProcHandle))
	{
		Output += FPlatformProcess::ReadPipe(ReadPipe);
	}
	
	FPlatformProcess::GetProcReturnCode(ProcHandle, &OutReturnCode);
	FPlatformProcess::CloseProc(ProcHandle);
	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

	if (OutReturnCode != 0)
	{
		UE_LOG(LogUnrealSharpProcHelper, Warning, TEXT("%s task failed (Args: %s) with return code %d"), *ProgramName, *Arguments, OutReturnCode)
		
		FText DialogText = FText::FromString(FString::Printf(TEXT("%s task failed: \n %s"), *ProgramName, *Output));
		FMessageDialog::Open(EAppMsgType::Ok, DialogText);
		return false;
	}

	double EndTime = FPlatformTime::Seconds();
	double ElapsedTime = EndTime - StartTime;
	UE_LOG(LogUnrealSharpProcHelper, Log, TEXT("%s with args (%s) took %f seconds to execute."), *ProgramName, *Arguments, ElapsedTime);
	
	return true;
}

bool FCSProcHelper::InvokeUnrealSharpBuildTool(EBuildAction BuildAction, const FString* BuildConfiguration)
{
	FName BuildActionCommand = StaticEnum<EBuildAction>()->GetNameByValue(BuildAction);
	FString PluginFolder = FPaths::ConvertRelativePathToFull(IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir());
	
	FString Args = FString::Printf(TEXT("--Action %s"), *BuildActionCommand.ToString());

	Args += FString::Printf(TEXT(" --EngineDirectory \"%s\""), *FPaths::ConvertRelativePathToFull(FPaths::EngineDir()));
	Args += FString::Printf(TEXT(" --ProjectDirectory \"%s\""), *FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
	Args += FString::Printf(TEXT(" --ProjectName %s"), FApp::GetProjectName());
	Args += FString::Printf(TEXT(" --PluginDirectory \"%s\""), *PluginFolder);
	Args += FString::Printf(TEXT(" --OutputPath \"%s\""), *FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), "Binaries", "UnrealSharp")));

	if (BuildConfiguration)
	{
		Args += FString::Printf(TEXT(" --BuildConfig %s"), *(*BuildConfiguration));
	}
	
	int32 ReturnCode = 0;
	FString Output;
	return InvokeCommand(GetUnrealSharpBuildToolPath(), Args, ReturnCode, Output);
}

bool FCSProcHelper::Build(const FString& BuildConfiguration)
{
	return InvokeUnrealSharpBuildTool(EBuildAction::Build, &BuildConfiguration);
}

bool FCSProcHelper::Rebuild(const FString& BuildConfiguration)
{
	return InvokeUnrealSharpBuildTool(EBuildAction::Rebuild, &BuildConfiguration);
}

bool FCSProcHelper::Clean()
{
	return InvokeUnrealSharpBuildTool(EBuildAction::Clean);
}
 
bool FCSProcHelper::GenerateProject()
{
	return InvokeUnrealSharpBuildTool(EBuildAction::GenerateProject);
}

FString FCSProcHelper::GetRuntimeHostPath()
{
	FString DotNetPath = GetDotNetDirectory();

#if PLATFORM_WINDOWS
	FString RuntimeHostPath = FPaths::Combine(DotNetPath, "host/fxr", HOSTFXR_VERSION, HOSTFXR_WINDOWS);
#elif PLATFORM_MAC
	FString RuntimeHostPath = FPaths::Combine(DotNetPath, "host/fxr", HOSTFXR_VERSION, HOSTFXR_MAC);
#endif
	
	return RuntimeHostPath;
}

FString FCSProcHelper::GetAssembliesPath()
{
	return FPaths::Combine(PluginDirectory, "Binaries", "DotNet", DOTNET_VERSION);
}

FString FCSProcHelper::GetUnrealSharpLibraryPath()
{
	return GetAssembliesPath() / "UnrealSharp.Plugins.dll";
}

FString FCSProcHelper::GetRuntimeConfigPath()
{
	return GetAssembliesPath() / "UnrealSharp.runtimeconfig.json";
}

FString FCSProcHelper::GetUserAssemblyDirectory()
{
	return FPaths::Combine(FPaths::ProjectDir(), "Binaries", "UnrealSharp");
}

FString FCSProcHelper::GetUserAssemblyPath()
{
	return FPaths::Combine(GetUserAssemblyDirectory(), UserManagedProjectName + ".dll");
}

FString FCSProcHelper::GetManagedSourcePath()
{
	return FPaths::Combine(PluginDirectory, "Managed");
}

FString FCSProcHelper::GetUnrealSharpBuildToolPath()
{
#if PLATFORM_WINDOWS
	return FPaths::ConvertRelativePathToFull(GetAssembliesPath() / "UnrealSharpBuildTool.exe");
#else
	return FPaths::ConvertRelativePathToFull(GetAssembliesPath() / "UnrealSharpBuildTool");
#endif
}

FString FCSProcHelper::GetDotNetDirectory()
{
	const FString PathVariable = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
		
	TArray<FString> Paths;
	PathVariable.ParseIntoArray(Paths, FPlatformMisc::GetPathVarDelimiter());

	for (FString& Path : Paths)
	{
		if (!Path.Contains(TEXT("dotnet")))
		{
			continue;
		}
		
		if (!FPaths::DirectoryExists(Path))
		{
			UE_LOG(LogUnrealSharpProcHelper, Warning, TEXT("Found path to DotNet, but the directory doesn't exist: %s"), *Path);
			break;
		}
			
		return Path;
	}
    
	return "";
}

FString FCSProcHelper::GetDotNetExecutablePath()
{
#if PLATFORM_WINDOWS
	return GetDotNetDirectory() + "dotnet.exe";
#else
	return GetDotNetDirectory() + "/dotnet";
#endif
}

bool FCSProcHelper::BuildBindings(const FString& BuildConfiguration)
{
	int32 ReturnCode = 0;
	FString Output;

	FString Arguments = FString::Printf(TEXT("build -c %s"), *BuildConfiguration);
	return InvokeCommand(GetDotNetExecutablePath(), Arguments, ReturnCode, Output, &UnrealSharpDirectory);
}

bool FCSProcHelper::BuildPrograms()
{
	FString DotNetPath = GetDotNetExecutablePath();
	FString UnrealSharpProgramsPath = FPaths::Combine(PluginDirectory, "Managed", "UnrealSharpPrograms");

	int32 ReturnCode = 0;
	FString Output;
	return InvokeCommand(DotNetPath, "build -c Release", ReturnCode, Output, &UnrealSharpProgramsPath);
}
