// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/SceneComponent.h"
//#include "Json.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "SceneView.h"
#include "Engine/TextureRenderTarget2D.h"
//#include "KismetMathLibrary.h"
#include "DynamicObject.generated.h"

UENUM(BlueprintType)
enum class ECommonMeshName : uint8
{
	ViveController,
	OculusTouchLeft,
	OculusTouchRight,
	ViveTracker
};

class FEngagementEvent
{
public:
	bool Active = true;
	FString EngagementType = "";
	FString Parent = "";
	float EngagementTime = 0;
	int32 EngagementNumber = 0;

	FEngagementEvent(FString name, FString parent, int engagementNumber)
	{
		EngagementType = name;
		Parent = parent;
		EngagementNumber = engagementNumber;
	}
};

class FDynamicObjectManifestEntry
{
public:
	FString Id = "";
	FString Name = "";
	FString MeshName = "";
	TMap<FString, FString> StringProperties;

	FDynamicObjectManifestEntry(FString id, FString name, FString mesh)
	{
		Id = id;
		Name = name;
		MeshName = mesh;
	}
	
	FDynamicObjectManifestEntry(){}

	FDynamicObjectManifestEntry* SetProperty(FString key, FString value);
};

class FDynamicObjectId
{
public:
	FString Id = "";
	bool Used = true;
	FString MeshName = "";

	FDynamicObjectId(FString id, FString meshName)
	{
		Id = id;
		MeshName = meshName;
	}

	FDynamicObjectId() {}
};

USTRUCT()
struct FDynamicObjectSnapshot
{
	GENERATED_BODY()

public:
	FVector position = FVector(0, 0, 0);
	FQuat rotation = FQuat(0, 0, 0, 1);
	double time = 0;
	FString id = "";
	TMap<FString, FString> StringProperties;
	TMap<FString, int32> IntegerProperties;
	TMap<FString, float> FloatProperties;
	TMap<FString, bool> BoolProperties;

	TArray<FEngagementEvent> Engagements;

	/*FDynamicObjectSnapshot* SnapshotProperty(FString key, FString value);
	FDynamicObjectSnapshot* SnapshotProperty(FString key, bool value);
	FDynamicObjectSnapshot* SnapshotProperty(FString key, int32 value);
	FDynamicObjectSnapshot* SnapshotProperty(FString key, double value);*/
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COGNITIVEVR_API UDynamicObject : public USceneComponent //UActorComponent
{
	GENERATED_BODY()

private:
	//TArray<TSharedPtr<FJsonObject>> snapshots;
	//extern TArray<TSharedPtr<FJsonObject>> snapshots;
	//extern int32 jsonPart = 0;
	//extern int32 MaxSnapshots = 64;

	float currentTime = 0;
	TSharedPtr<FAnalyticsProviderCognitiveVR> s;
	TSharedPtr<FDynamicObjectId> ObjectID;
	FVector LastPosition;
	FVector LastForward;

public:	
	// Sets default values for this component's properties

	UPROPERTY(EditAnywhere)// , BlueprintReadWrite)
	ECommonMeshName CommonMeshName;

	UPROPERTY(EditAnywhere)
	bool UseCustomMeshName = true;

	UPROPERTY(EditAnywhere)
	FString MeshName;

	UPROPERTY(EditAnywhere)
	bool SnapshotOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool SnapshotOnInterval = true;

	UPROPERTY(EditAnywhere)
	bool ReleaseIdOnDestroy = true;

	//group and id

	UPROPERTY(EditAnywhere)
	bool UseCustomId;

	UPROPERTY(EditAnywhere)
	FString CustomId = "";

	UPROPERTY(EditAnywhere)
	FString GroupName;

	UPROPERTY(EditAnywhere)
	bool TrackGaze = false;

	//snapshots

	UPROPERTY(EditAnywhere)
	float SnapshotInterval = 0.1;

	UPROPERTY(EditAnywhere)
	float PositionThreshold = 2;

	UPROPERTY(EditAnywhere)
	float RotationThreshold = 10;

	UDynamicObject();
	
	//engagements
	TArray<FEngagementEvent> DirtyEngagements;
	TArray<FEngagementEvent> Engagements;
	void BeginEngagementId(FString engagementName, FString parentDynamicObjectId);
	void EndEngagementId(FString engagementName, FString parentDynamicObjectId);


	virtual void OnComponentCreated() override;
	virtual void BeginPlay() override;

	TSharedPtr<FDynamicObjectId> GetUniqueId(FString meshName);
	TSharedPtr<FDynamicObjectId> GetObjectId();

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
	FDynamicObjectSnapshot MakeSnapshot();

	static TSharedPtr<FJsonValueObject> WriteSnapshotToJson(FDynamicObjectSnapshot snapshot);

	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	static void SendData();
	static TArray<TSharedPtr<FJsonValueObject>> DynamicSnapshotsToString();
	static TSharedPtr<FJsonObject> DynamicObjectManifestToString();

	
	//FDynamicObjectSnapshot NewSnapshot();

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		FDynamicObjectSnapshot SnapshotStringProperty(UPARAM(ref) FDynamicObjectSnapshot& target, FString key, FString stringValue);

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		FDynamicObjectSnapshot SnapshotBoolProperty(UPARAM(ref) FDynamicObjectSnapshot& target, FString key, bool boolValue);

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		FDynamicObjectSnapshot SnapshotIntegerProperty(UPARAM(ref) FDynamicObjectSnapshot& target, FString key, int32 intValue);

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		FDynamicObjectSnapshot SnapshotFloatProperty(UPARAM(ref) FDynamicObjectSnapshot& target, FString key, float floatValue);

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		static void BeginEngagement(UDynamicObject* target, FString engagementType);

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		static void EndEngagement(UDynamicObject* target, FString engagementType);

	UFUNCTION(BlueprintCallable, Category = "CognitiveVR Analytics|Dynamic Object")
		///this does not directly send a snapshot - it stores it until Flush is called or the number of stored dynamic snapshots reaches its limit
		void SendDynamicObjectSnapshot(UPARAM(ref) FDynamicObjectSnapshot& target);

	void EndPlay(const EEndPlayReason::Type EndPlayReason);

};