// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BulletHeavenPOC : ModuleRules
{
	public BulletHeavenPOC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"BulletHeavenPOC",
			"BulletHeavenPOC/Variant_Strategy",
			"BulletHeavenPOC/Variant_Strategy/UI",
			"BulletHeavenPOC/Variant_TwinStick",
			"BulletHeavenPOC/Variant_TwinStick/AI",
			"BulletHeavenPOC/Variant_TwinStick/Gameplay",
			"BulletHeavenPOC/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
