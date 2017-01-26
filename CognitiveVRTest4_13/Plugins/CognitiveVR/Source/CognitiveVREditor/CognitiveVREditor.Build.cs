using UnrealBuildTool;

public class CognitiveVREditor : ModuleRules
{
	public CognitiveVREditor(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
               "CognitiveVR"
            });

        PrivateDependencyModuleNames.AddRange(
			new string[] {
                "Core",
                "CoreUObject",
                "DesktopPlatform",
                "AssetTools",
                "Engine",
                "Slate",
				"InputCore",
				"SlateCore",
				"PropertyEditor",
				"LevelEditor",
                "MeshUtilities",
                "MainFrame",
                "UnrealEd",
                "PluginBrowser",
                "Projects",
                "CognitiveVR"
            }
		);
	}
}