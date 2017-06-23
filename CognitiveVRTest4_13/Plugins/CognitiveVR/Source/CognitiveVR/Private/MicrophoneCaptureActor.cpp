// Fill out your copyright notice in the Description page of Project Settings.

#include "CognitiveVR.h"
#include "CognitiveVRProvider.h"
#include "Private/CognitiveVRPrivatePCH.h"
#include "MicrophoneCaptureActor.h"


// Sets default values
AMicrophoneCaptureActor::AMicrophoneCaptureActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMicrophoneCaptureActor::BeginPlay()
{
	Super::BeginPlay();	
}

//microphone input 'processed' every tick
void AMicrophoneCaptureActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (!VoiceCapture.IsValid())
		return;

	uint32 VoiceCaptureBytesAvailable = 0;
	EVoiceCaptureState::Type CaptureState = VoiceCapture->GetCaptureState(VoiceCaptureBytesAvailable);
	
	//UE_LOG(LogTemp, Warning, TEXT("capture state is %s"), EVoiceCaptureState::ToString(CaptureState));

	VoiceCaptureBuffer.Reset();
	
	//VoiceCaptureBytesAvailable = 16000 / DeltaTime;

	//UE_LOG(LogTemp, Warning, TEXT("record bytes %d"), (int32)VoiceCaptureBytesAvailable);

	//quiet sections sometimes trigger the NoData capture state
	if (CaptureState == EVoiceCaptureState::Ok && VoiceCaptureBytesAvailable > 0)
	{
		//could fake voicecapturereadbytes? set it to 16000 * deltatime and force it to read even if empty. NOPE
		uint32 VoiceCaptureReadBytes;
		//float VoiceCaptureTotalSquared = 0;
	
		VoiceCaptureBuffer.SetNumUninitialized(VoiceCaptureBytesAvailable);
	
		VoiceCapture->GetVoiceData(VoiceCaptureBuffer.GetData(), VoiceCaptureBytesAvailable, VoiceCaptureReadBytes);

		//VoiceCaptureReadBytes = VoiceCaptureBytesAvailable/2; //repeats recorded sections, but includes pauses

		QueueAudio(VoiceCaptureBuffer.GetData(), VoiceCaptureReadBytes);

		TotalRecordedBytes += VoiceCaptureReadBytes;

		//all volume level stuff
		/*
		//TotalRecordedBytes += VoiceCaptureReadBytes;
		for (uint32 i = 0; i < (VoiceCaptureReadBytes / 2); i++)
		{
			VoiceCaptureSample = (VoiceCaptureBuffer[i * 2 + 1] << 8) | VoiceCaptureBuffer[i * 2];
			VoiceCaptureTotalSquared += ((float) VoiceCaptureSample * (float) VoiceCaptureSample);
		}

		//VoiceCaptureReadBytes = VoiceCaptureBytesAvailable;
	
		float VoiceCaptureMeanSquare = (2 * (VoiceCaptureTotalSquared / VoiceCaptureBuffer.Num()));
		float VoiceCaptureRms = FMath::Sqrt(VoiceCaptureMeanSquare);
		float VoiceCaptureFinalVolume = ((VoiceCaptureRms / 32768.0) * 200.f);
	
		VoiceCaptureVolume = VoiceCaptureFinalVolume;*/	

		//DEBUG puts audio
		//VoiceCaptureSoundWaveProcedural->QueueAudio(VoiceCaptureBuffer.GetData(), VoiceCaptureReadBytes);
	}
}

bool AMicrophoneCaptureActor::BeginRecording()
{
	VoiceCapture = FVoiceModule::Get().CreateVoiceCapture();
	if (!VoiceCapture.IsValid())
		return false;
	VoiceCapture->Start();
	return true;
}

//this enqueues 1 tick lengths of audio bytes (16000*deltatime). stitch these together at the end to get all the recorded bytes
void AMicrophoneCaptureActor::QueueAudio(const uint8* AudioData, const int32 BufferSize)
{
	if (BufferSize == 0 || !ensure((BufferSize % sizeof(int16)) == 0))
	{
		//UE_LOG(LogTemp, Warning, TEXT("QUEUE AUDIO BUFFER SIZE IS ZERO"));
		return;
	}

	TArray<uint8> NewAudioBuffer;
	NewAudioBuffer.AddUninitialized(BufferSize);
	FMemory::Memcpy(NewAudioBuffer.GetData(), AudioData, BufferSize);
	//QueuedAudio.Enqueue(NewAudioBuffer);
	QueuedAudio.Append(NewAudioBuffer);

	AvailableByteCount.Add(BufferSize);
}

void AMicrophoneCaptureActor::EndRecording()
{
	VoiceCapture->Stop();

	TArray<uint8> wavData = EncodeToWav(QueuedAudio);

	wav64string = FBase64::Encode(wavData);

	//DEBUG save wav to disk
	if (false)
	{
		FString SaveDirectory = FString("C:/Users/calder/Desktop");
		FString FileName = FString("recordedAudio.wav");

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (PlatformFile.CreateDirectoryTree(*SaveDirectory))
		{
			// Get absolute file path
			FString AbsoluteFilePath = SaveDirectory + "/" + FileName;

			//FFileHelper::SaveStringToFile(Content, *AbsoluteFilePath);
			FFileHelper::SaveArrayToFile(wavData, *AbsoluteFilePath);
		}
		UE_LOG(LogTemp, Warning, TEXT("AMicrophoneCaptureActor::EndRecording saved wav file to desktop"));
	}

	//DEBUG playback
	if (false)
	{
		VoiceCaptureAudioComponent->SetSound(VoiceCaptureSoundWaveProcedural);
		VoiceCaptureAudioComponent->Play();
	}
}

TArray<uint8> AMicrophoneCaptureActor::EncodeToWav(TArray<uint8> rawData)
{
	TArray<uint8> finaldata;

	uint8 bytes[4];
	uint8 twobytes[2];
	unsigned long n = 0;

	///RIFF

	//riff (big)
	FString riff = "RIFF";
	FTCHARToUTF8 Converter1(*riff);
	auto riffdata = (const uint8*)Converter1.Get();
	finaldata.Append(riffdata, 4);

	//chunk size (little)
	n = rawData.Num() - (int32)8;
	bytes[3] = (n >> 24) & 0xFF;
	bytes[2] = (n >> 16) & 0xFF;
	bytes[1] = (n >> 8) & 0xFF;
	bytes[0] = n & 0xFF;
	finaldata.Append(bytes, 4);

	//format (big)
	FString wave = "WAVE";
	FTCHARToUTF8 Converter2(*wave);
	auto wavedata = (const uint8*)Converter2.Get();
	finaldata.Append(wavedata, 4);

	///fmt sub-chunk

	//subchunk1 id (big)
	FString fmt = "fmt ";
	FTCHARToUTF8 Converter3(*fmt);
	auto fmtdata = (const uint8*)Converter3.Get();
	finaldata.Append(fmtdata, 4);

	//subchunk 1 size (little)
	n = 16;
	bytes[3] = (n >> 24) & 0xFF;
	bytes[2] = (n >> 16) & 0xFF;
	bytes[1] = (n >> 8) & 0xFF;
	bytes[0] = n & 0xFF;
	finaldata.Append(bytes, 4);

	//audio format (little)
	n = 1;
	twobytes[1] = (n >> 8) & 0xFF;
	twobytes[0] = n & 0xFF;
	finaldata.Append(twobytes, 2);

	//num channels (little)
	n = 1;
	twobytes[1] = (n >> 8) & 0xFF;
	twobytes[0] = n & 0xFF;
	finaldata.Append(twobytes, 2);

	//sample rate (little)
	n = 16000;
	bytes[3] = (n >> 24) & 0xFF;
	bytes[2] = (n >> 16) & 0xFF;
	bytes[1] = (n >> 8) & 0xFF;
	bytes[0] = n & 0xFF;
	finaldata.Append(bytes, 4);

	//byte rate (little)
	n = 32000;
	bytes[3] = (n >> 24) & 0xFF;
	bytes[2] = (n >> 16) & 0xFF;
	bytes[1] = (n >> 8) & 0xFF;
	bytes[0] = n & 0xFF;
	finaldata.Append(bytes, 4);

	//block align (little)
	n = 2;
	twobytes[1] = (n >> 8) & 0xFF;
	twobytes[0] = n & 0xFF;
	finaldata.Append(twobytes, 2);

	//bits per sample (little)
	n = 16;
	twobytes[1] = (n >> 8) & 0xFF;
	twobytes[0] = n & 0xFF;
	finaldata.Append(twobytes, 2);

	/// data subchunk
	
	//subchunk2 id (big)
	FString data = "data";
	FTCHARToUTF8 Converter4(*data);
	auto datadata = (const uint8*)Converter4.Get();
	finaldata.Append(datadata, 4);

	//subchunk2 size (little)
	n = rawData.Num() * 2 * 2;
	bytes[3] = (n >> 24) & 0xFF;
	bytes[2] = (n >> 16) & 0xFF;
	bytes[1] = (n >> 8) & 0xFF;
	bytes[0] = n & 0xFF;
	finaldata.Append(bytes, 4);

	//data (little)
	for (int32 i = 0; i < rawData.Num(); i++)
	{
		finaldata.Add(rawData[i]);
	}

	return finaldata;
}

FString AMicrophoneCaptureActor::GetMicrophoneRecording()
{
	//UE_LOG(LogTemp, Log, TEXT("wav file contents %s"), *wav64string);
	return wav64string;
}