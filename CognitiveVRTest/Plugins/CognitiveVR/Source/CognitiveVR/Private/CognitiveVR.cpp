// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CognitiveVR.h"
#include "CognitiveVRPrivatePCH.h"
#include "HeadMountedDisplay.h"
#include "AnalyticsSettings.h"
#include "CognitiveVRSettings.h"
#include "PlayerTracker.h"
#include "DynamicObject.h"

using namespace cognitivevrapi;

IMPLEMENT_MODULE(FAnalyticsCognitiveVR, CognitiveVR);

bool bHasSessionStarted = false;

void FAnalyticsCognitiveVR::StartupModule()
{
	CognitiveVRProvider = MakeShareable(new FAnalyticsProviderCognitiveVR());
	GLog->Log("AnalyticsCognitiveVR::StartupModule");
}

void FAnalyticsCognitiveVR::ShutdownModule()
{
	/*if (CognitiveVRProvider.IsValid())
	{
	CognitiveVRProvider->EndSession();
	}*/
}

bool FAnalyticsProviderCognitiveVR::HasStartedSession()
{
	return bHasSessionStarted;
}

TSharedPtr<IAnalyticsProvider> FAnalyticsCognitiveVR::CreateAnalyticsProvider(const FAnalyticsProviderConfigurationDelegate& GetConfigValue) const
{
	return CognitiveVRProvider;
}

TSharedPtr<FAnalyticsProviderCognitiveVR> FAnalyticsCognitiveVR::GetCognitiveVRProvider() const
{
	TSharedPtr<IAnalyticsProvider> Provider = FAnalytics::Get().GetDefaultConfiguredProvider();
	if (Provider.IsValid())
	{
		TSharedPtr<FAnalyticsProviderCognitiveVR> prov = StaticCastSharedPtr<FAnalyticsProviderCognitiveVR>(CognitiveVRProvider);
		return prov;
	}
	cognitivevrapi::CognitiveLog::Warning("FAnalyticsCognitiveVR::GetCognitiveVRProvider could not get provider!");
	return NULL;
}

// Provider


FAnalyticsProviderCognitiveVR::FAnalyticsProviderCognitiveVR() :
	Age(0)
{
	DeviceId = FPlatformMisc::GetUniqueDeviceId();
}

FAnalyticsProviderCognitiveVR::~FAnalyticsProviderCognitiveVR()
{
	UE_LOG(LogTemp, Warning, TEXT("shutdown cognitivevr module"));
}

UWorld* currentWorld;
void FAnalyticsProviderCognitiveVR::SetWorld(UWorld* world)
{
	currentWorld = world;
}
void FAnalyticsProviderCognitiveVR::SendDeviceInfo()
{
	//add a bunch of properties
	if (GEngine->HMDDevice.IsValid())
	{
		auto hmd = GEngine->HMDDevice.Get();

		EHMDDeviceType::Type devicetype = hmd->GetHMDDeviceType();

		if (devicetype == EHMDDeviceType::DT_SteamVR)
		{
			TArray<FAnalyticsEventAttribute> EventAttributes;
			FName deviceName = hmd->GetDeviceName();
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DeviceName"), deviceName.ToString()));
			FAnalyticsCognitiveVR::Get().GetCognitiveVRProvider()->RecordEvent("HMDDevice", EventAttributes);
		}
		else
		{
			hmd->RecordAnalytics();
		}
	}
}

bool FAnalyticsProviderCognitiveVR::StartSession(const TArray<FAnalyticsEventAttribute>& Attributes)
{
	CognitiveLog::Init();

	if (bPendingInitRequest)
	{
		CognitiveLog::Warning("AnalyticsProviderCognitiveVR::StartSession already pending init!");
		return false;
	}
	if (bHasSessionStarted)
	{
		//EndSession();
		//return false;
	}

	GetSessionTimestamp();
	GetSessionID();

	TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject());

	//get attributes
	//userid
	//deviceid
	//initProperties
	if (Age != 0)
	{
		properties->SetNumberField("Age", Age);
	}
	if (Gender.Len() > 0)
	{
		properties->SetStringField("Gender", Gender);
	}
	if (Location.Len() > 0)
	{
		properties->SetStringField("Location", Location);
	}

	if (GetUserID().IsEmpty())
	{
		CognitiveLog::Info("FAnalyticsProviderCognitiveVR::StartSession user id is empty!");
		SetUserID("anonymous_"+DeviceId);
	}

	initProperties = properties;

	CustomerId = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "Analytics", "CognitiveVRApiKey", false);

	//OverrideHttpInterface* httpint = new OverrideHttpInterface();
	network = MakeShareable(new Network(this));
	//network->Init(httpint, &InitCallback);
	bPendingInitRequest = true;

	CognitiveLog::Info("FAnalyticsProviderCognitiveVR::StartSession");
	

	// Also include all the streaming levels in the results
	/*for (int32 LevelIndex = 0; LevelIndex < GWorld->StreamingLevels.Num(); ++LevelIndex)
	{
		ULevelStreaming* StreamingLevel = GWorld->StreamingLevels[LevelIndex];
		if (StreamingLevel != NULL)
		{
			//StreamingLevel->OnLevelLoaded.Add(this,&FAnalyticsProviderCognitiveVR::OnLevelLoaded);
			//TODO clear objectid list when persistent scene unloads
		}
	}*/

	CacheSceneData();

	CognitiveLog::Info("CognitiveVR InitCallback Response");

	//-----------------------'response'

	bHasSessionStarted = true;

	customevent = MakeShareable(new CustomEvent(this));

	if (!network.IsValid())
	{
		CognitiveLog::Warning("CognitiveVRProvider InitCallback network is null");
		return false;
	}

	core_utils = MakeShareable(new CoreUtilities(this));
	sensors = MakeShareable(new Sensors(this));

	TSharedPtr<FJsonObject>deviceProperties = Util::DeviceScraper(initProperties);
	if (deviceProperties.IsValid())
	{
		core_utils->UpdateDeviceState(TCHAR_TO_UTF8(*GetDeviceID()), deviceProperties);
	}
	else
	{
		CognitiveLog::Error("Cognitive InitCallback could not find device properties!");
	}

	//send new user / new device messages if necessary

	/*auto dataObject = resp.GetContent();
	if (dataObject.GetBoolField("usernew"))
	{
		core_utils->NewUser(TCHAR_TO_UTF8(*cog->GetUserID()));
		//new device
	}
	if (dataObject.GetBoolField("devicenew"))
	{
		core_utils->NewDevice(TCHAR_TO_UTF8(*cog->GetDeviceID()));
	//new device
	}*/

	customevent->Send(FString("Session Begin"));
	bPendingInitRequest = false;

	//get all dynamic objects

	//cog->OnInitResponse().Broadcast(resp.IsSuccessful());

	if (currentWorld != NULL)
	{
		for (TActorIterator<AStaticMeshActor> ActorItr(currentWorld); ActorItr; ++ActorItr)
		{
			// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
			AStaticMeshActor *Mesh = *ActorItr;

			UActorComponent* actorComponent = Mesh->GetComponentByClass(UDynamicObject::StaticClass());
			if (actorComponent == NULL)
			{
				continue;
			}
			UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
			if (dynamic == NULL)
			{
				continue;
			}
			if (dynamic->GetObjectId().IsValid())
			{
				continue;
			}
			CognitiveLog::Warning("InitCallback Dynamic Special startup!");
			//TODO network test to actually reach cognitive3d server
			//dynamic->BeginPlayCallback(true); 
		}
	}
	else
	{
		CognitiveLog::Error("InitCallback current world is null - SceneExplorer will not receive data. Have you added a PlayerTracker component to your character?");
	}

	SendDeviceInfo();

	return bHasSessionStarted;
}

void FAnalyticsProviderCognitiveVR::OnLevelLoaded()
{

}

void FAnalyticsProviderCognitiveVR::EndSession()
{
	if (!customevent.IsValid())
	{
		return;
	}

	CognitiveLog::Info("FAnalyticsProviderCognitiveVR::EndSession");

	bPendingInitRequest = false;

	TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
	properties->SetNumberField("sessionlength", Util::GetTimestamp() - GetSessionTimestamp());

	customevent->Send(FString("Session End"), properties);

	FlushEvents();
	CognitiveLog::Info("Freeing CognitiveVR memory.");

	//delete network;
	network = NULL;

	//delete transaction;
	customevent = NULL;

	//delete core_utils;
	core_utils = NULL;

	//delete sensors;
	sensors = NULL;
	CognitiveLog::Info("CognitiveVR memory freed.");

	SessionTimestamp = -1;
	SessionId = "";

	bHasSessionStarted = false;
	LastSesisonTimestamp = Util::GetTimestamp() + 1;
}

void FAnalyticsProviderCognitiveVR::FlushEvents()
{
	FAnalyticsProviderCognitiveVR* cog = FAnalyticsCognitiveVR::Get().GetCognitiveVRProvider().Get();
	if (cog == NULL)
	{
		CognitiveLog::Error("FAnalyticsProviderCognitiveVR::FlushEvents could not GetCognitiveVRProvider!");
		return;
	}

	if (cog->HasStartedSession() == false)
	{
		CognitiveLog::Error("FAnalyticsProviderCognitiveVR::FlushEvents CognitiveVRProvider has not started session!");
		return;
	}

	//send to dashboard
	cog->customevent->SendData();

	//send to scene explorer
	sensors->SendData();
	UDynamicObject::SendData();

	TArray<APlayerController*, FDefaultAllocator> controllers;
	GEngine->GetAllLocalPlayerControllers(controllers);
	if (controllers.Num() == 0)
	{
		CognitiveLog::Error("FAnalyticsProviderCognitiveVR::FlushEvents cannot find controller. Skip upload to scene explorer");
		return;
	}
	if (controllers[0]->GetPawn() == NULL)
	{
		CognitiveLog::Error("FAnalyticsProviderCognitiveVR::FlushEvents controller0 has no pawn. Skip upload to scene explorer");
		return;
	}

	UPlayerTracker* up = controllers[0]->GetPawn()->FindComponentByClass<UPlayerTracker>();
	if (up == NULL)
	{
		CognitiveLog::Error("FAnalyticsProviderCognitiveVR::FlushEvents couldn't find player tracker. Skip upload to scene explorer");
		return;
	}
	else
	{
		up->SendData();
	}
}

void FAnalyticsProviderCognitiveVR::SetUserID(const FString& InUserID)
{
	if (!bHasSessionStarted)
	{
		UserId = InUserID;
		CognitiveLog::Info("FAnalyticsProviderCognitiveVR::SetUserID set user id");
	}
	else
	{
		// Log that we shouldn't switch users during a session
		CognitiveLog::Warning("FAnalyticsProviderCognitiveVR::SetUserID called while session is in progress. Ignoring");
	}
}

FString FAnalyticsProviderCognitiveVR::GetUserID() const
{
	return UserId;
}

FString FAnalyticsProviderCognitiveVR::GetDeviceID() const
{
	return DeviceId;
}

FString FAnalyticsProviderCognitiveVR::GetSessionID() const
{
	return SessionId;
}

double FAnalyticsProviderCognitiveVR::GetSessionTimestamp()
{
	if (SessionTimestamp < 0)
	{
		SessionTimestamp = Util::GetTimestamp();
	}
	return SessionTimestamp;
}

FString FAnalyticsProviderCognitiveVR::GetCognitiveSessionID()
{
	if (SessionId.IsEmpty())
		SessionId = FString::FromInt(GetSessionTimestamp()) + TEXT("_") + DeviceId;
	return SessionId;
}

bool FAnalyticsProviderCognitiveVR::SetSessionID(const FString& InSessionID)
{
	if (!bHasSessionStarted)
	{
		CognitiveLog::Warning("FAnalyticsProviderCognitiveVR::SetSessionID automatically sets session id. Ignoring");

		/*if (SessionTimestamp < 0)
		{
			SessionTimestamp = Util::GetTimestamp();
		}*/
	}
	else
	{
		// Log that we shouldn't switch session ids during a session
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordEvent while a session is in progress. Ignoring");
	}
	return !bHasSessionStarted;
}

void FAnalyticsProviderCognitiveVR::RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);

		for (auto Attr : Attributes)
		{
			properties->SetStringField(Attr.AttrName, Attr.AttrValue);
		}

		customevent->Send(EventName, properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordEvent called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordItemPurchase(const FString& ItemId, const FString& Currency, int32 PerItemCost, int32 ItemQuantity)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("itemId", ItemId);
		properties->SetStringField("currency", Currency);
		properties->SetNumberField("PerItemCost", PerItemCost);
		properties->SetNumberField("ItemQuantity", ItemQuantity);

		customevent->Send("cvr.recorditempurchase", properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordItemPurchase called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordCurrencyPurchase(const FString& GameCurrencyType, int32 GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("GameCurrencyType", GameCurrencyType);
		properties->SetNumberField("GameCurrencyAmount", GameCurrencyAmount);
		properties->SetStringField("RealCurrencyType", RealCurrencyType);
		properties->SetNumberField("RealMoneyCost", RealMoneyCost);
		properties->SetStringField("PaymentProvider", PaymentProvider);

		customevent->Send("cvr.recordcurrencypurchase", properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordCurrencyPurchase called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordCurrencyGiven(const FString& GameCurrencyType, int32 GameCurrencyAmount)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("GameCurrencyType", GameCurrencyType);
		properties->SetNumberField("GameCurrencyAmount", GameCurrencyAmount);

		customevent->Send(FString("cvr.recordcurrencygiven"), properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordCurrencyGiven called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::SetAge(const int32 InAge)
{
	Age = InAge;
}

void FAnalyticsProviderCognitiveVR::SetLocation(const FString& InLocation)
{
	Location = InLocation;
}

void FAnalyticsProviderCognitiveVR::SetGender(const FString& InGender)
{
	Gender = InGender;
}

void FAnalyticsProviderCognitiveVR::SetBuildInfo(const FString& InBuildInfo)
{
	BuildInfo = InBuildInfo;
}

void FAnalyticsProviderCognitiveVR::RecordError(const FString& Error, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("Error", Error);

		for (auto Attr : Attributes)
		{
			properties->SetStringField(Attr.AttrName, Attr.AttrValue);
		}

		customevent->Send(FString("cvr.recorderror"), properties);

		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordError");
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordError called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordProgress(const FString& ProgressType, const FString& ProgressName, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("ProgressType", ProgressType);
		properties->SetStringField("ProgressName", ProgressName);

		for (auto Attr : Attributes)
		{
			properties->SetStringField(Attr.AttrName, Attr.AttrValue);
		}

		customevent->Send(FString("cvr.recordprogress"), properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordError called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordItemPurchase(const FString& ItemId, int32 ItemQuantity, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("ItemId", ItemId);
		properties->SetNumberField("ItemQuantity", ItemQuantity);

		for (auto Attr : Attributes)
		{
			properties->SetStringField(Attr.AttrName, Attr.AttrValue);
		}

		customevent->Send(FString("cvr.recorditempurchase"), properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordItemPurchase called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordCurrencyPurchase(const FString& GameCurrencyType, int32 GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("GameCurrencyType", GameCurrencyType);
		properties->SetNumberField("GameCurrencyAmount", GameCurrencyAmount);

		for (auto Attr : Attributes)
		{
			properties->SetStringField(Attr.AttrName, Attr.AttrValue);
		}

		customevent->Send(FString("RecordCurrencyPurchase"), properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordCurrencyPurchase called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::RecordCurrencyGiven(const FString& GameCurrencyType, int32 GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	if (bHasSessionStarted)
	{
		TSharedPtr<FJsonObject> properties = MakeShareable(new FJsonObject);
		properties->SetStringField("GameCurrencyType", GameCurrencyType);
		properties->SetNumberField("GameCurrencyAmount", GameCurrencyAmount);

		for (auto Attr : Attributes)
		{
			properties->SetStringField(Attr.AttrName, Attr.AttrValue);
		}

		customevent->Send(FString("cvr.recordcurrencygiven"), properties);
	}
	else
	{
		CognitiveLog::Warning("FAnalyticsProvideCognitiveVR::RecordCurrencyGiven called before StartSession. Ignoring");
	}
}

void FAnalyticsProviderCognitiveVR::AppendUD(TSharedPtr<FJsonValueArray>& jsonArray)
{
	if (UserId.IsEmpty())
	{
		FString empty = FString("");
		//Util::AppendToJsonArray(jsonArray, empty);
	}
	else
	{
		//Util::AppendToJsonArray(jsonArray, UserId);
	}

	if (DeviceId.IsEmpty())
	{
		FString empty = FString("");
		//Util::AppendToJsonArray(jsonArray, empty);
	}
	else
	{
		//Util::AppendToJsonArray(jsonArray, DeviceId);
	}
}

void FAnalyticsProviderCognitiveVR::SetDeviceID(const FString& InDeviceID)
{
	if (!bHasSessionStarted)
	{
		//DeviceId = InDeviceID;
		//CognitiveLog::Info("FAnalyticsProviderCognitiveVR::SetDeviceID set device id");
	}
	else
	{
		// Log that we shouldn't switch users during a session
		CognitiveLog::Warning("FAnalyticsProviderCognitiveVR::SetDeviceID called while session is in progress. Ignoring");
	}
}

TSharedPtr<FSceneData> FAnalyticsProviderCognitiveVR::GetCurrentSceneData()
{
	UWorld* myworld = currentWorld;
	//UWorld* myworld = AActor::GetWorld();
	if (myworld == NULL)
	{
		CognitiveLog::Warning("FAnalyticsProviderCognitiveVR::SendJson no world - use GWorld->GetWorld");
		currentWorld = GWorld->GetWorld();
		myworld = currentWorld;
	}

	FString currentSceneName = myworld->GetMapName();
	currentSceneName.RemoveFromStart(myworld->StreamingLevelsPrefix);
	return GetSceneData(currentSceneName);
}

TSharedPtr<FSceneData> FAnalyticsProviderCognitiveVR::GetSceneData(FString scenename)
{
	for (int i = 0; i < SceneData.Num(); i++)
	{
		if (!SceneData[i].IsValid()) { continue; }
		if (SceneData[i]->Name == scenename)
		{
			return SceneData[i];
		}
	}
	GLog->Log("FAnalyticsProviderCognitiveVR::GetSceneData couldn't find SceneData for scene " + scenename);
	return NULL;
}

FString FAnalyticsProviderCognitiveVR::GetCurrentSceneId()
{
	auto currentData = GetCurrentSceneData();
	if (!currentData.IsValid()) { return ""; }

	return currentData->Id;
}

FString FAnalyticsProviderCognitiveVR::GetCurrentSceneVersionNumber()
{
	auto currentData = GetCurrentSceneData();
	if (!currentData.IsValid()) { return ""; }

	return FString::FromInt(currentData->VersionNumber);
}

void FAnalyticsProviderCognitiveVR::CacheSceneData()
{
	TArray<FString>scenstrings;
	FString TestSyncFile = FPaths::Combine(*(FPaths::GameDir()), TEXT("Config/DefaultEngine.ini"));
	GConfig->GetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), scenstrings, TestSyncFile);

	for (int i = 0; i < scenstrings.Num(); i++)
	{
		TArray<FString> Array;
		scenstrings[i].ParseIntoArray(Array, TEXT(","), true);

		if (Array.Num() == 2) //scenename,sceneid
		{
			//old scene data. append versionnumber and versionid
			Array.Add("1");
			Array.Add("0");
		}

		if (Array.Num() != 4)
		{
			GLog->Log("failed to parse " + scenstrings[i]);
			continue;
		}

		FSceneData* tempscene = new FSceneData(Array[0], Array[1], FCString::Atoi(*Array[2]), FCString::Atoi(*Array[3]));
		SceneData.Add(MakeShareable(tempscene));
	}
}

FVector FAnalyticsProviderCognitiveVR::GetPlayerHMDPosition()
{
	//TODO cache this and check for null. playercontrollers DO NOT persist across level changes

	TArray<APlayerController*, FDefaultAllocator> controllers;
	GEngine->GetAllLocalPlayerControllers(controllers);
	if (controllers.Num() == 0)
	{
		CognitiveLog::Warning("FAnalyticsProviderCognitiveVR::GetPlayerHMDPosition no controllers skip");
		return FVector();
	}

	return controllers[0]->PlayerCameraManager->GetCameraLocation();
}