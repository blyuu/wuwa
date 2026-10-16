using UnrealBuildTool;

public class GraphicsOptimizerRuntime : ModuleRules
{
    public GraphicsOptimizerRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI", "InputCore", "Json", "Projects" });

    }
}
