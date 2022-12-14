// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "CognitiveVR/Public/DynamicObject.h"

class UDynamicObject;

//a collection of these are kept to record current and recent gaze data and the state of the eye

class COGNITIVEVR_API FEyeCapture
{
public:
	FVector WorldPosition;
	FVector LocalPosition;
	
	FTransform CaptureMatrix;
	bool UseCaptureMatrix;
	FString HitDynamicId;

	bool SkipPositionForFixationAverage;
	FVector HMDPosition;
	int64 Time;
	
	bool Discard = true;
	bool EyesClosed;
	bool OutOfRange;
	bool OffTransform;
	FVector2D ScreenPos;
};