// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CognitiveVR/Public/C3DCommonTypes.h"
#include "CoreMinimal.h"
#include "AnalyticsEventAttribute.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "CognitiveVR/Public/CognitiveVR.h"
#include "CognitiveVR/Public/CognitiveVRBlueprints.h"

#include "HeadMountedDisplay.h"
#include "CognitiveVR/Public/CognitiveVRSettings.h"
#include "CognitiveVR/Private/ExitPoll.h"
#include "CognitiveVR/Private/PlayerTracker.h"
#include "CognitiveVR/Public/DynamicObject.h"
#include "CognitiveVR/Private/FixationRecorder.h"

#include "CognitiveVR/Private/util/util.h"
#include "CognitiveVR/Private/util/cognitive_log.h"
#include "CognitiveVR/Private/network/network.h"
#include "CognitiveVR/Private/api/customeventrecorder.h"
#include "CognitiveVR/Private/api/sensor.h"
#include "CognitiveVR/Private/LocalCache.h"
#include "Engine/Engine.h"
#include "Misc/Base64.h"
#include "Misc/PackageName.h"//to get friendly name of streaming levels




	//included here so the class can be saved as a variable without a circular reference (since these often need to reference the provider)
	//everything here is referenced from headers. why is this being forward declared?
	class Network;
	class CustomEventRecorder;
	//class CognitiveVRResponse;
	class Sensors;
	class ExitPoll;
	class LocalCache;
	//class UDynamicObject;

	class COGNITIVEVR_API FAnalyticsProviderCognitiveVR : public IAnalyticsProvider
	{
		/** Unique Id representing the session the analytics are recording for */
		FString SessionId;
		/** Holds the Age if set */
		/** Holds the build info if set */
		FString BuildInfo;
		FString ParticipantName;
		/** Id representing the user the analytics are recording for */
		FString ParticipantId;
		FString DeviceId;
		double SessionTimestamp = -1;
		FJsonObject NewSessionProperties;
		FJsonObject AllSessionProperties;

		

	private:
		static UWorld* currentWorld;
		
		//reads all scene data from engine ini
		void CacheSceneData();

		static bool bHasSessionStarted;

	public:
		FAnalyticsProviderCognitiveVR();
		virtual ~FAnalyticsProviderCognitiveVR();

		bool StartSession();
		virtual bool StartSession(const TArray<FAnalyticsEventAttribute>& Attributes) override;
		virtual void EndSession() override;
		virtual void FlushEvents() override;
		void FlushAndCacheEvents();

		virtual void SetUserID(const FString& InUserID) override;
		//returns unique participant id
		virtual FString GetUserID() const override;
		//returns participant name
		FString GetUserName() const;

		virtual FString GetSessionID() const override;
		//allows developers to manually set a sesison id instead of generating one
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
		TSharedPtr<ExitPoll> exitpoll;
		TSharedPtr<LocalCache> localCache;

		FString GetDeviceID() const;

		void SetLobbyId(FString lobbyId);
		FString LobbyId;

		double GetSessionTimestamp() const;

		FVector GetPlayerHMDPosition();

		bool HasStartedSession();

		FString ApplicationKey;
		FString AttributionKey;

		FString GetCurrentSceneId();
		FString GetCurrentSceneVersionNumber();
		//if a session name has been explicitly set. otherwise will use participant name when that is set
		bool bHasCustomSessionName;
		void SetSessionName(FString sessionName);
		
		//used to identify when a scene changes and session properties need to be resent
		FString CurrentTrackingSceneId;
		//used to see id the current scene has changed and needs to search for new sceneId
		TSharedPtr<FSceneData> LastSceneData;


		void SetWorld(UWorld* world);
		UWorld* GetWorld();
		//calls player tracker session begin (which sets the world). if not found, will return null
		UWorld* EnsureGetWorld();

		TArray<TSharedPtr<FSceneData>> SceneData;
		TSharedPtr<FSceneData> GetSceneData(FString scenename);
		TSharedPtr<FSceneData> GetCurrentSceneData();
		FJsonObject GetNewSessionProperties();
		FJsonObject GetAllSessionProperties();

		bool ForceWriteSessionMetadata = false;

		void SetParticipantFullName(FString participantName);
		void SetParticipantId(FString participantId);
		void SetParticipantProperty(FString name, int32 value);
		void SetParticipantProperty(FString name, float value);
		void SetParticipantProperty(FString name, FString value);
		void SetSessionTag(FString tag);

		void SetSessionProperty(FString name, int32 value);
		void SetSessionProperty(FString name, float value);
		void SetSessionProperty(FString name, FString value);

		FString GetAttributionParameters();
	};