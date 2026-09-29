using UnrealBuildTool;

public class GraphicsOptimizerEditor : ModuleRules
{
    public GraphicsOptimizerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "GraphicsOptimizerRuntime",
            "UnrealEd", "Slate", "SlateCore", "ToolMenus", "RHI", "InputCore", "ApplicationCore", "TraceServices"
        });
    }
}
