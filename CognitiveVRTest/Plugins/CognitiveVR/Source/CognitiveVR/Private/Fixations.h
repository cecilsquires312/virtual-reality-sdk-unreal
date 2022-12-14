// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CognitiveVR/Private/EyeCapture.h"
#include "Runtime/Engine/Classes/Components/SceneComponent.h"
#include "Fixations.generated.h"


//used to pass basic eye data into active session view to draw on screen
USTRUCT(BlueprintType)
struct FC3DGazePoint
{
	GENERATED_BODY()

		//public:
		UPROPERTY(BlueprintReadWrite)
		USceneComponent* Parent = NULL;
	UPROPERTY(BlueprintReadWrite)
		bool IsLocal = false;
	UPROPERTY(BlueprintReadWrite)
		FVector WorldPosition = FVector(0, 0, 0);
	UPROPERTY(BlueprintReadWrite)
		FVector LocalPosition = FVector(0, 0, 0);
};

class COGNITIVEVR_API FFixation
{
public:
	FVector WorldPosition;
	FVector LocalPosition;
	//USceneComponent* LocalTransform = NULL;

	//float DebugScale;
	int64 LastUpdated;
	int64 DurationMs;
	int64 StartMs;
	
	int64 LastNonDiscardedTime;
	int64 LastEyesOpen;
	int64 LastInRange;
	int64 LastOnTransform;

	float StartDistance;
	float MaxRadius;
	bool IsLocal;
	FString DynamicObjectId = "";
	
	//set in begin local fixation and updated while fixation is active
	FTransform Transformation;
	
	void AddEyeCapture(FEyeCapture eyeCapture)
	{
		bool flag = true;
		if (eyeCapture.Discard)
		{
			flag = false;
		}
		else
		{
			LastNonDiscardedTime = eyeCapture.Time;
		}
		if (eyeCapture.EyesClosed)
		{
			flag = false;
		}
		else
		{
			LastEyesOpen = eyeCapture.Time;
		}
		if (eyeCapture.OutOfRange)
		{
			flag = false;
		}
		else
		{
			LastInRange = eyeCapture.Time;
		}
		if (eyeCapture.OffTransform)
		{
			flag = false;
		}
		else
		{
			LastOnTransform = eyeCapture.Time;
		}
		if (flag)
		{
			LastUpdated = eyeCapture.Time;
			DurationMs = eyeCapture.Time - StartMs;
		}

		if (IsLocal && eyeCapture.HitDynamicId == DynamicObjectId)
		{
			Transformation = eyeCapture.CaptureMatrix;
		}
	}
};