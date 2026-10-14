// Copyright (c) 2026 GregOrigin. MIT License: see LICENSE in the plugin root.

using UnrealBuildTool;

public class Foothold : ModuleRules
{
	public Foothold(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Mover", "GameplayTags", "EnhancedInput" });
		PrivateDependencyModuleNames.AddRange(new string[] { "NetCore", "InputCore" });
	}
}
