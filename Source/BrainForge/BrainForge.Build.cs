using UnrealBuildTool;

public class BrainForge : ModuleRules
{
	public BrainForge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"ApplicationCore",
			"RenderCore",
			"RHI",
			"Json",
			"JsonUtilities",
			"ImageWrapper",
			"AudioMixer",
			"Projects"
		});
	}
}
