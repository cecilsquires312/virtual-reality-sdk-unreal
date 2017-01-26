// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CognitiveVR.h"
#include "CognitiveVRPrivatePCH.h"

#include "Private/unreal/override_http_interface.h"
#include "Private/util/util.h"
#include "Private/util/config.h"
#include "Private/util/cognitive_log.h"
#include "Private/network/cognitivevr_response.h"
#include "Private/util/cognitivevr_exception.h"
#include "Private/network/http_interface.h"
#include "Private/unreal/buffer_manager.h"
#include "Private/network/network.h"
#include "Private/api/tuning.h"
#include "Private/api/transaction.h"
#include "Private/api/coreutilities.h"
#include "Engine.h"

extern bool bHasSessionStarted;

	enum Error {
		kErrorSuccess = 0,
		kErrorGeneric = -1,
		kErrorNotInitialized = -2,
		kErrorNotFound = -3,
		kErrorInvalidArgs = -4,
		kErrorMissingId = -5,
		kErrorRequestTimedOut = -6,
		kErrorUnknown = -7
	};

	//included here so the class can be saved as a variable without a circular reference (since these often need to reference the provider)
	class Network;
	class Transaction;
	class Tuning;
	class BufferManager;
	class CoreUtilities;
	class OverrideHttpInterface;
	class CognitiveVRResponse;

	class FAnalyticsProviderCognitiveVR : public IAnalyticsProvider
	{
		/** Path where analytics files are saved out */
		//FString AnalyticsFilePath;
		/** Tracks whether we need to start the session or restart it */
		//bool bHasSessionStarted;
		/** Whether an event was written before or not */
		//bool bHasWrittenFirstEvent;
		/** Id representing the user the analytics are recording for */
		FString UserId;
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
		/** The file archive used to write the data */
		//FArchive* FileArchive;

	public:
		/** True once server has responded. everything is initialized at this point */
		//TODO time out should set bPendingInitRequest to false
		bool bPendingInitRequest=false;

		FAnalyticsProviderCognitiveVR();
		//virtual ~FAnalyticsProviderCognitiveVR();

		virtual bool StartSession(const TArray<FAnalyticsEventAttribute>& Attributes) override;
		virtual void EndSession() override;
		virtual void FlushEvents() override;

		virtual void SetUserID(const FString& InUserID) override;
		virtual FString GetUserID() const override;

		virtual FString GetSessionID() const override;
		virtual bool SetSessionID(const FString& InSessionID) override;

		virtual void RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes) override;

		virtual void RecordItemPurchase(const FString& ItemId, const FString& Currency, int PerItemCost, int ItemQuantity) override;

		virtual void RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const FString& RealCurrencyType, float RealMoneyCost, const FString& PaymentProvider) override;

		virtual void RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount) override;

		virtual void SetBuildInfo(const FString& InBuildInfo) override;
		virtual void SetGender(const FString& InGender) override;
		virtual void SetLocation(const FString& InLocation) override;
		virtual void SetAge(const int32 InAge) override;

		virtual void RecordItemPurchase(const FString& ItemId, int ItemQuantity, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordCurrencyPurchase(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordCurrencyGiven(const FString& GameCurrencyType, int GameCurrencyAmount, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordError(const FString& Error, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;
		virtual void RecordProgress(const FString& ProgressType, const FString& ProgressHierarchy, const TArray<FAnalyticsEventAttribute>& EventAttrs) override;


		//custom cognitive
		FString DeviceId;
		Transaction* transaction;
		Tuning* tuning;
		Network* network;
		BufferManager* thread_manager;
		CoreUtilities* core_utils;
		TSharedPtr<FJsonObject> initProperties; //optional properties sent when initializing. platform, ram, etc
		FString GetDeviceID() const;
		void SetDeviceID(const FString& InDeviceID);

		void AppendUD(TSharedPtr<FJsonValueArray> &json);
		FVector GetPlayerHMDPosition();
		void SendDeviceInfo();

	};

	void ThrowDummyResponseException(std::string s);