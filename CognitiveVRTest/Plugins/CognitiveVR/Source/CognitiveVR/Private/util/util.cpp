/*
** Copyright (c) 2016 CognitiveVR, Inc. All rights reserved.
*/
#include "CognitiveVR.h"
#include "util/util.h"
#include "GenericPlatformDriver.h"

double cognitivevrapi::Util::GetTimestamp()
{
	#pragma warning(push)
	#pragma warning(disable:4244) //Disable warning regarding loss of accuracy, no concern.

	long ts = time(0);
	double miliseconds = FDateTime::UtcNow().GetMillisecond();
	double finalTime = ts + miliseconds*0.001;

	return finalTime;
	//http://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c

	#pragma warning(pop)
}

FString cognitivevrapi::Util::GetDeviceName(FString DeviceName)
{
	if (DeviceName == "OculusRift")
	{
		return "rift";
	}
	if (DeviceName == "OSVR")
	{
		return "rift";
	}
	if (DeviceName == "SimpleHMD")
	{
		return "rift";
	}
	if (DeviceName == "SteamVR")
	{
		return "vive";
	}
	return FString("unknown");
}

void cognitivevrapi::Util::SetHardwareSessionProperties()
{
	auto cog = FAnalyticsCognitiveVR::Get().GetCognitiveVRProvider();

	FString appName;
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectName"), appName, GGameIni);

	cog->SetSessionProperty("c3d.app.name", appName);

	FString appVersion = "1.0";
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), appVersion, GGameIni);
	cog->SetSessionProperty("c3d.app.version", appVersion);

	FString engineVersion = FEngineVersion::Current().ToString().Replace(TEXT("+"), TEXT(" "));;
	cog->SetSessionProperty("c3d.app.engine.version", engineVersion);

	auto platformName = UGameplayStatics::GetPlatformName();
	if (platformName.Compare("Windows", ESearchCase::IgnoreCase) == 0 || platformName.Compare("Mac", ESearchCase::IgnoreCase) == 0 || platformName.Compare("Linux", ESearchCase::IgnoreCase) == 0)
	{
		cog->SetSessionProperty("c3d.device.type", "Desktop");
	}
	else if (platformName.Compare("IOS", ESearchCase::IgnoreCase) == 0 || platformName.Compare("Android", ESearchCase::IgnoreCase) == 0)
	{
		cog->SetSessionProperty("c3d.device.type", "Handheld");
	}
	else if (platformName.Compare("PS4", ESearchCase::IgnoreCase) == 0 || platformName.Contains("xbox", ESearchCase::IgnoreCase) || platformName.Contains("Switch", ESearchCase::IgnoreCase))
	{
		cog->SetSessionProperty("c3d.device.type", "Console");
	}
	else
	{
		cog->SetSessionProperty("c3d.device.type", "Unknown");
	}


	cog->SetSessionProperty("c3d.device.cpu", FWindowsPlatformMisc::GetCPUBrand());

	//TODO device model, especially for phones

	cog->SetSessionProperty("c3d.device.gpu", FWindowsPlatformMisc::GetPrimaryGPUBrand());

	FString osVersionOut;
	FString osSubVersionOut;
	FWindowsPlatformMisc::GetOSVersions(osVersionOut, osSubVersionOut);
	cog->SetSessionProperty("c3d.device.os", osVersionOut + " " + osSubVersionOut);

	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	cog->SetSessionProperty("c3d.device.memory", (int)MemoryConstants.TotalPhysicalGB);
}