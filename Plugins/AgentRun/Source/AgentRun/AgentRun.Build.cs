using UnrealBuildTool;

public class AgentRun : ModuleRules
{
	public AgentRun(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore"
		});

		// ApplicationCore: the default input device. RenderCore and RHI: frame times for benches.
		PrivateDependencyModuleNames.AddRange(new string[] {
			"ApplicationCore",
			"RenderCore",
			"RHI"
		});
	}
}
