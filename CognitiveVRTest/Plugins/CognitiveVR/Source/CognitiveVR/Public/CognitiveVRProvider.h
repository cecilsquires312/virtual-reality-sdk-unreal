// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnalyticsEventAttribute.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "Public/CognitiveVR.h"
#include "Public/CognitiveVRBlueprints.h"

#include "HeadMountedDisplay.h"
#include "Public/CognitiveVRSettings.h"
#include "Private/ExitPoll.h"
#include "Private/PlayerTracker.h"
#include "Public/DynamicObject.h"
#include "Private/FixationRecorder.h"

#include "Private/util/util.h"
#include "Private/util/cognitive_log.h"
#include "Private/network/network.h"
#include "Private/api/customeventrecorder.h"
#include "Private/api/sensor.h"
#include "Engine/Engine.h"

//namespace cognitivevrapi
//{
	class FSceneData
	{
	public:
		FString Name = "";
		FString Id = "";
		int32 VersionNumber = 1;
		int32 VersionId = 0;

		FSceneData(FString name, FString id, int32 versionnumber, int32 versionid)
		{
			Name = name;
			Id = id;
			VersionNumber = versionnumber;
			VersionId = versionid;
		}
	};

	enum CognitiveVRError {
		kErrorSuccess = 0,
		kErrorGeneric = -1,
		kErrorNotInitialized = -2,
		kErrorNotFound = -3,
		kErrorInvalidArgs = -4,
		kErrorMissingId = -5,
		kErrorRequestTimedOut = -6,
		kErrorUnknown = -7
	};
//}

	//included here so the class can be saved as a variable without a circular reference (since these often need to reference the provider)
	//everything here is referenced from headers. why is this being forward declared?
	class Network;
	class CustomEventRecorder;
	//class CognitiveVRResponse;
	class Sensors;
	//class ExitPoll;
	//class UDynamicObject;

	class COGNITIVEVR_API FAnalyticsProviderCognitiveVR : public IAnalyticsProvider
	{
		/** Unique Id representing the session the analytics are recording for */
		FString SessionId;
		/** Holds the Age if set */
		int32 Age;
		/** Holds the Location of the user if set */
		FString Location;
		/** Holds the Gender of the user if set */
		FString Gender;
		/** Holds the build info if set */
		FString BuildInfo;
		/** Id representing the user the analytics are recording for */
		FString UserId;
		FString DeviceId;
		double SessionTimestamp = -1;
		FJsonObject SessionProperties;

		

	private:
		static UWorld* currentWorld;
		
		//reads all scene data from engine ini
		void CacheSceneData();

		static bool bHasSessionStarted;

	public:
		FAnalyticsProviderCognitiveVR();
		virtual ~FAnalyticsProviderCognitiveVR();

		virtual bool StartSession(const TArray<FAnalyticsEventAttribute>& Attributes) override;
		virtual void EndSession() override;
		virtual void FlushEvents() override;

		virtual void SetUserID(const FString& InUserID) override;
		virtual FString GetUserID() const override;

		virtual FString GetSessionID() const override;
		virtual bool SetSessionID(const FString& InSessionID) override;

		virtual void RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes) override;

		virtual void RecordItemPurchase(const FString& ItemId, const FString& Currency, int32 PerItemCost, int32 ItemQuantity) override;

		virtual void RecordCurrencyPurchase(const FString& GameCurrencyType, int32 GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider) override;

		virtual void RecordCurrencyGiven(const FString& GameCurrencyType, int32 GameCurrencyAmount) override;

		virtual void SetBuildInfo(const FString& InBuildInfo) override;
		virtual void SetGender(const FString& InGender) override;
		virtual void SetLocation(const FString& InLocation) override;
		virtual void SetAge(const int32 InAge) override;

		virtual void RecordItemPurchase(const FString& ItemId, int32 ItemQuantity, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordCurrencyPurchase(const FString& GameCurrencyType, int32 GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordCurrencyGiven(const FString& GameCurrencyType, int32 GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordError(const FString& Error, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordProgress(const FString& ProgressType, const FString& ProgressHierarchy, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		
		TSharedPtr<CustomEventRecorder> customEventRecorder;
		TSharedPtr<Network> network;
		TSharedPtr<Sensors> sensors;
		
		FString GetDeviceID() const;

		void SetLobbyId(FString lobbyId);
		FString LobbyId;

		double GetSessionTimestamp() const;

		FVector GetPlayerHMDPosition();

		bool HasStartedSession();

		FString APIKey;

		FString GetCurrentSceneId();
		FString GetCurrentSceneVersionNumber();
		void SetSessionName(FString sessionName);

		void SetWorld(UWorld* world);
		UWorld* GetWorld();
		//calls player tracker session begin (which sets the world). if not found, will return null
		UWorld* EnsureGetWorld();

		TArray<TSharedPtr<FSceneData>> SceneData;
		TSharedPtr<FSceneData> GetSceneData(FString scenename);
		TSharedPtr<FSceneData> GetCurrentSceneData();
		FJsonObject GetSessionProperties();

		void SetSessionProperty(FString name, int32 value);
		void SetSessionProperty(FString name, float value);
		void SetSessionProperty(FString name, FString value);
	};