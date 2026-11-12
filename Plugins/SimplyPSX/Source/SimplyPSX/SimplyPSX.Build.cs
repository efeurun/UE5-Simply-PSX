// UE5 Simply-PSX

using UnrealBuildTool;

public class SimplyPSX : ModuleRules
{
    public SimplyPSX(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}
