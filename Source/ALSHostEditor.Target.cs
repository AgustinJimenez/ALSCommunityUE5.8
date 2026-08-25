using UnrealBuildTool;
using System.Collections.Generic;

public class ALSHostEditorTarget : TargetRules
{
	public ALSHostEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ALSHost");
	}
}
