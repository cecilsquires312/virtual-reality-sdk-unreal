// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
namespace UnrealBuildTool.Rules
{
	public class CognitiveVR : ModuleRules
	{
		public CognitiveVR(ReadOnlyTargetRules Target): base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			bLegacyPublicIncludePaths = true;
			
            PublicIncludePathModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
					"Analytics"
					// ... add public include paths required here ...
				}
				);
			
			PrivateIncludePaths.AddRange(
				new string[] {
					"CognitiveVR/Private",
                    "CognitiveVR/Public",
					"CognitiveVR/Private/util"
					// ... add other private include paths required here ...
				}
				);

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "HeadMountedDisplay",
					"Slate",
					"SlateCore"
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
					"AnalyticsVisualEditing",
					"Projects",
					"HTTP",
					"Json",
                    "JsonUtilities",
					"UMG",
					"EngineSettings",
					"EyeTracker" //openxr
                }
				);

#if UE_4_26_OR_LATER
	PublicDependencyModuleNames.Add("DeveloperSettings");
#endif

		//uncomment this to enable eye tracking support using IEyeTracker interface (varjo openxr support, etc)
		//PublicDefinitions.Add("OPENXR_EYETRACKING");
		//PublicDefinitions.Add("OPENXR_LOCALSPACE"); //uncomment this if OPENXR implemented in local space instead of worldspace

		//uncomment these lines to enable Vive WaveVR eye tracking support
		//PublicDefinitions.Add("WAVEVR_EYETRACKING");
		//PublicDependencyModuleNames.Add("WaveVR");

		var pluginsDirectory = System.IO.Path.Combine(Target.ProjectFile.Directory.ToString(),"Plugins");
		
		//Varjo (up to and including version 3.0.0)
		if (System.IO.Directory.Exists(System.IO.Path.Combine(pluginsDirectory,"Varjo")))
		{
			System.Console.WriteLine("CognitiveVR.Build.cs found Varjo Plugin folder");
			PublicDependencyModuleNames.Add("VarjoHMD");
			PublicDependencyModuleNames.Add("VarjoEyeTracker");
		}
		
		//TobiiEyeTracking
		if (System.IO.Directory.Exists(System.IO.Path.Combine(pluginsDirectory,"TobiiEyeTracking")))
		{
			System.Console.WriteLine("CognitiveVR.Build.cs found TobiiEyeTracking Plugin folder");
			PrivateIncludePaths.AddRange(
				new string[] {
					"../../TobiiEyeTracking/Source/TobiiCore/Private",
					"../../TobiiEyeTracking/Source/TobiiCore/Public"
				});

			PublicDependencyModuleNames.Add("TobiiCore");
		}
		
		//Vive Pro Eye (SRanipal)
		if (System.IO.Directory.Exists(System.IO.Path.Combine(pluginsDirectory,"SRanipal"))) 
		{
			//ideally read the uplugin file as json and get the VersionName
			//for now, just read the source directory layout
			var sranipalPlugin = System.IO.Path.Combine(pluginsDirectory,"SRanipal");
			var sranipalSource = System.IO.Path.Combine(sranipalPlugin,"Source");
			string[] sourceModules = System.IO.Directory.GetDirectories(sranipalSource);
			if (sourceModules.Length == 2)//1.1.0.1 and 1.2.0.1 only have eye and lip modules
			{
				PublicDefinitions.Add("SRANIPAL_1_2_API");
				System.Console.WriteLine("CognitiveVR.Build.cs found SRanipal Plugin folder");
				PrivateIncludePaths.AddRange(
					new string[] {
						"../../SRanipal/Source/SRanipal/Private",
						"../../SRanipal/Source/SRanipal/Public"
					});

				PublicDependencyModuleNames.Add("SRanipal");

				string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", "..", ".."));
				string SRanipalDir = System.IO.Path.Combine(BaseDirectory,"SRanipal","Binaries",Target.Platform.ToString());
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(SRanipalDir,"SRanipal.lib"));
				PublicDelayLoadDLLs.Add(System.IO.Path.Combine(SRanipalDir,"SRanipal.dll"));	
			}
			else if (sourceModules.Length == 5)
			{
				PublicDefinitions.Add("SRANIPAL_1_3_API");
				System.Console.WriteLine("CognitiveVR.Build.cs found SRanipal Plugin folder 1.3.0.9 or newer");
				PrivateIncludePaths.AddRange(
					new string[] {
						"../../SRanipal/Source/SRanipal/Private",
						"../../SRanipal/Source/SRanipal/Public",
						"../../SRanipal/Source/SRanipalEye/Private",
						"../../SRanipal/Source/SRanipalEye/Public",
						"../../SRanipal/Source/SRanipalEyeTracker/Private",
						"../../SRanipal/Source/SRanipalEyeTracker/Public"
					});

				PublicDependencyModuleNames.Add("SRanipal");
				PublicDependencyModuleNames.Add("SRanipalEye");
				PublicDependencyModuleNames.Add("SRanipalEyeTracker");

				string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", "..", ".."));
				string SRanipalDir = System.IO.Path.Combine(BaseDirectory,"SRanipal","Binaries",Target.Platform.ToString());
				PublicAdditionalLibraries.Add(System.IO.Path.Combine(SRanipalDir,"SRanipal.lib"));
				PublicDelayLoadDLLs.Add(System.IO.Path.Combine(SRanipalDir,"SRanipal.dll"));	
			}
		}

		//Pico Neo 2 Eye
		if (System.IO.Directory.Exists(System.IO.Path.Combine(pluginsDirectory,"PicoMobile")))
		{
			System.Console.WriteLine("CognitiveVR.Build.cs found Pico Plugin folder");
			PublicDependencyModuleNames.Add("PicoMobile");
		}

        //HP Omnicept
		if (System.IO.Directory.Exists(System.IO.Path.Combine(pluginsDirectory, "HPGlia")))
		{
			System.Console.WriteLine("CognitiveVR.Build.cs found HP Glia Omnicept folder");
			PublicDependencyModuleNames.Add("HPGlia");
		}

		//this is all for runtime audio capture support
		if (Target.Platform == UnrealTargetPlatform.Win64
			|| Target.Platform == UnrealTargetPlatform.Win32
            )
        {
            // Add __WINDOWS_WASAPI__ so that RtAudio compiles with WASAPI
            PublicDefinitions.Add("__WINDOWS_DS__");

            // Allow us to use direct sound
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DirectSound");
			
			string DirectXSDKDir = Target.UEThirdPartySourceDirectory + "Windows/DirectX";
			PublicSystemIncludePaths.Add( DirectXSDKDir + "/include");

			PublicAdditionalLibraries.AddRange(
					new string[] {
					"dxguid.lib",
					"dsound.lib"
					}
				);
			}
        }
	}
}
