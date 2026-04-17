// cloudRenderer.cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "cloudRenderer.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FcloudRendererModule"

void FcloudRendererModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    // Log a message to the console to mark module start
    UE_LOG(LogTemp, Warning, TEXT("cloudRenderer StartupModule executed"));

    // Query the Plugin Manager to find the loaded instance of this plugin by its name
    TSharedPtr<IPlugin> Plugin =
        IPluginManager::Get().FindPlugin(TEXT("cloudRenderer"));

    // Check if plugin was found
    if (!Plugin.IsValid())
    {
        // exit early if not found
        UE_LOG(LogTemp, Error, TEXT("cloudRenderer plugin not found"));
        return;
    }

    // Get abs. path to the shaders folder in base directory
    FString ShaderDir = FPaths::ConvertRelativePathToFull(
        Plugin->GetBaseDir() / TEXT("Shaders")
    );

    // Log the resolved shader directory path
    UE_LOG(LogTemp, Warning, TEXT("ShaderDir: %s"), *ShaderDir);

    // Verify shaders directory exists
    if (!FPaths::DirectoryExists(ShaderDir))
    {
        UE_LOG(LogTemp, Error, TEXT("Shader directory missing!"));
        return;
    }

    // Map the physical shader directory to path "/cloudRenderer" in Unreal's shader file system.
    // This allows shaders in this folder to be compiled and included using "#include /cloudRenderer/..."
    AddShaderSourceDirectoryMapping(TEXT("/cloudRenderer"), ShaderDir);
}

void FcloudRendererModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FcloudRendererModule, cloudRenderer)