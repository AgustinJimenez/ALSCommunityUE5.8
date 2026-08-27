using UnrealBuildTool;

public class ALSHost : ModuleRules
{
	public ALSHost(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"EnhancedInput",  // For UInputAction/UInputMappingContext, weapon fire binding
			"ALSV4_CPP",      // For AALSCharacter, ALS enums (gait/rotation mode)
			"UMG",            // For the reload-offset debug tuning widget
			"ApplicationCore",// For FPlatformApplicationMisc::ClipboardCopy (debug tuning "copy values" button)
			"Slate",          // For FReply (native mouse handling in UALSOverlayStateOptionWidget)
			"SlateCore",
			"AIModule",       // For AAIController/enemy chase-attack logic (AALSEnemyAIController)
			"GameplayTasks"   // AIModule's AAIController depends on this
		});

		// CQTest: lets automated gameplay tests run as engine code compiled
		// into this module (Automation window / headless -ExecCmds), which is
		// the only way to reach the PIE world's own GWorld from a test - see
		// docs/testing.md. Test files themselves are guarded by
		// #if WITH_AUTOMATION_TESTS, so these deps are inert in Shipping.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CQTest",
			"CQTestEnhancedInput"
		});
	}
}
