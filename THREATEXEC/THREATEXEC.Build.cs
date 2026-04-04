using UnrealBuildTool;
public class THREATEXEC : ModuleRules
{
    public THREATEXEC(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Json", "JsonUtilities", "InputCore", "ProceduralMeshComponent", "RenderCore", "RHI" });
        PrivateDependencyModuleNames.AddRange(new[] { "Engine", "UMG", "Slate", "SlateCore", "ImageWrapper", "RenderCore", "RHI" });
        if (Target.bBuildEditor) { PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd" }); }
    }
}