// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CognitiveVRAnalytics : ModuleRules
{
	public CognitiveVRAnalytics(TargetInfo Target)
	{
        PCHUsage = PCHUsageMode.NoSharedPCHs;

        PublicIncludePathModuleNames.AddRange(
			new string[] {
				"Analytics"
				// ... add public include paths required here ...
			}
			);
		PublicIncludePaths.AddRange(
			new string[] {
				//"CognitiveVRAnalytics/Public",
				// ... add other private include paths required here ...
			}
			);
		
		PrivateIncludePaths.AddRange(
			new string[] {
                					"Runtime/CognitiveVRAnalytics/Private",
					"Engine",
                    "HTTP",
                "Private",
                "Private/unreal",
                "CognitiveVRAnalytics/Private",
                "CognitiveVRAnalytics/Private/api",
                "CognitiveVRAnalytics/Private/network",
                "CognitiveVRAnalytics/Private/unreal",
                "CognitiveVRAnalytics/Private/util",
				// ... add other private include paths required here ...
			}
			);


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "AnalyticsBlueprintLibrary",
                    "Analytics",
                    "HTTP",
					"Json",
					"JsonUtilities"
            }
            );


        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "HTTP",                    
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
