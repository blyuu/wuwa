// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WutheringWaves : ModuleRules
{
	public WutheringWaves(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "MotionWarping", "GameplayAbilities", "GameplayTags", "GameplayTasks", "AIModule", "NavigationSystem", "UMG"
		});

		// Slate/SlateCore: needed to touch FSlateBrush / FProgressBarStyle directly (per-character bar fill image)
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
