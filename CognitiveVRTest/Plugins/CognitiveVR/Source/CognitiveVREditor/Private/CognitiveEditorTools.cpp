
#include "CognitiveEditorTools.h"

#define LOCTEXT_NAMESPACE "BaseToolEditor"

//TSharedRef<FCognitiveEditorTools> ToolsInstance;
FCognitiveEditorTools* FCognitiveEditorTools::CognitiveEditorToolsInstance;
FString FCognitiveEditorTools::Gateway;

FCognitiveEditorTools* FCognitiveEditorTools::GetInstance()
{
	return CognitiveEditorToolsInstance;
}

//GET dynamic object manifest
FString FCognitiveEditorTools::GetDynamicObjectManifest(FString versionid)
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/versions/" + versionid + "/objects";
}

//POST dynamic object manifest
FString FCognitiveEditorTools::PostDynamicObjectManifest(FString sceneid, int32 versionnumber)
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/objects/" + sceneid + "?version=" + FString::FromInt(versionnumber);
}

//POST dynamic object mesh data
FString FCognitiveEditorTools::PostDynamicObjectMeshData(FString sceneid, int32 versionnumber, FString exportdirectory)
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/objects/" + sceneid + "/" + exportdirectory + "?version=" + FString::FromInt(versionnumber);
}

//GET scene settings and read scene version
FString FCognitiveEditorTools::GetSceneVersion(FString sceneid)
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/scenes/" + sceneid;
}

//POST scene screenshot
FString FCognitiveEditorTools::PostScreenshot(FString sceneid, FString versionnumber)
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/scenes/" + sceneid + "/screenshot?version=" + versionnumber;
}

//POST upload decimated scene
FString FCognitiveEditorTools::PostNewScene()
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/scenes";
}

//POST upload and replace existing scene
FString FCognitiveEditorTools::PostUpdateScene(FString sceneid)
{
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	if (Gateway.Len() == 0)
	{
		Gateway = "data.cognitive3d.com";
	}
	return "https://" + Gateway + "/v0/scenes/" + sceneid;
}

//WEB used to open scenes on sceneexplorer or custom session viewer
FString FCognitiveEditorTools::SceneExplorerOpen(FString sceneid)
{
	auto sessionviewer = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SessionViewer", false);
	return "https://" + sessionviewer + sceneid;
}


void FCognitiveEditorTools::Initialize()
{
	CognitiveEditorToolsInstance = new FCognitiveEditorTools;

	//should be able to update gateway while unreal is running, but cache if not in editor since that's nuts
	Gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);

	//CognitiveEditorToolsInstance->BaseExportDirectory = FPaths::GameDir();

	//should update both editor urls and session data urls
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	CognitiveEditorToolsInstance->ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
}

//at any step in the uploading process
bool WizardUploading = false;

bool FCognitiveEditorTools::IsWizardUploading()
{
	return WizardUploading;
}


TArray<TSharedPtr<FDynamicData>> SceneExplorerDynamics;
TArray<TSharedPtr<FString>> SubDirectoryNames;

//deals with all the exporting and non-display stuff in the editor preferences

TArray<TSharedPtr<FDynamicData>> FCognitiveEditorTools::GetSceneDynamics()
{
	return SceneDynamics;
}

bool FCognitiveEditorTools::HasDeveloperKey() const
{
	return FAnalyticsCognitiveVR::Get().DeveloperKey.Len() > 0;
}

bool FCognitiveEditorTools::HasApplicationKey() const
{
	return ApplicationKey.Len() > 0;
}

FProcHandle FCognitiveEditorTools::ExportNewDynamics()
{
	FProcHandle fph;
	UWorld* tempworld = GEditor->GetEditorWorldContext().World();

	if (!tempworld)
	{
		UE_LOG(LogTemp, Warning, TEXT("FCognitiveEditorToolsCustomization::ExportDynamics world is null"));
		return fph;
	}

	if (BaseExportDirectory.Len() == 0)
	{
		GLog->Log("base directory not selected");
		return fph;
	}

	TArray<FString> meshNames;
	TArray<UDynamicObject*> exportObjects;

	//get all dynamic object components in scene. add names/pointers to array
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}

		FString path = GetDynamicsExportDirectory() + "/" + dynamic->MeshName + "/" + dynamic->MeshName;
		FString gltfpath = path + ".gltf";
		if (FPaths::FileExists(*gltfpath))
		{
			//already exported
			continue;
		}
		if (!meshNames.Contains(dynamic->MeshName))
		{
			exportObjects.Add(dynamic);
			meshNames.Add(dynamic->MeshName);
		}
	}

	if (meshNames.Num() == 0)
	{
		return fph;
	}

	return ExportDynamicObjectArray(exportObjects);
}

FReply FCognitiveEditorTools::ExportAllDynamics()
{
	UWorld* tempworld = GEditor->GetEditorWorldContext().World();

	if (!tempworld)
	{
		UE_LOG(LogTemp, Warning, TEXT("FCognitiveEditorToolsCustomization::ExportDynamics world is null"));
		return FReply::Handled();
	}

	TArray<FString> meshNames;
	TArray<UDynamicObject*> exportObjects;

	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		//AStaticMeshActor *Mesh = *ActorItr;

		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		if (!meshNames.Contains(dynamic->MeshName))
		{
			exportObjects.Add(dynamic);
			meshNames.Add(dynamic->MeshName);
		}
	}

	if (meshNames.Num() == 0)
	{
		return FReply::Handled();
	}

	if (BaseExportDirectory.Len() == 0)
	{
		GLog->Log("base directory not selected");
		return FReply::Handled();
	}

	ExportDynamicObjectArray(exportObjects);
	return FReply::Handled();
}

FReply FCognitiveEditorTools::ExportSelectedDynamics()
{
	TArray<FString> meshNames;
	TArray<UDynamicObject*> SelectionSetCache;
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			//SelectionSetCache.Add(Actor);
			UActorComponent* actorComponent = Actor->GetComponentByClass(UDynamicObject::StaticClass());
			if (actorComponent == NULL)
			{
				continue;
			}
			UDynamicObject* dynamicComponent = Cast<UDynamicObject>(actorComponent);
			if (dynamicComponent == NULL)
			{
				continue;
			}
			if (!meshNames.Contains(dynamicComponent->MeshName))
			{
				SelectionSetCache.Add(dynamicComponent);
				meshNames.Add(dynamicComponent->MeshName);
			}
		}
	}

	ExportDynamicObjectArray(SelectionSetCache);

	return FReply::Handled();
}

FProcHandle FCognitiveEditorTools::ExportDynamicData(TArray< TSharedPtr<FDynamicData>> dynamicData)
{
	//find all meshes in scene that are contained in the dynamicData list

	TArray<FString> meshNames;
	TArray<UDynamicObject*> SelectionSetCache;
	for (TActorIterator<AActor> It(GWorld); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			//SelectionSetCache.Add(Actor);
			UActorComponent* actorComponent = Actor->GetComponentByClass(UDynamicObject::StaticClass());
			if (actorComponent == NULL)
			{
				continue;
			}
			UDynamicObject* dynamicComponent = Cast<UDynamicObject>(actorComponent);
			if (dynamicComponent == NULL)
			{
				continue;
			}

			if (meshNames.Contains(dynamicComponent->MeshName))
			{
				//mesh will already be exported
				continue;
			}

			bool exportActor = false;
			for (auto& elem : dynamicData)
			{
				if (elem->MeshName == dynamicComponent->MeshName)
				{
					exportActor = true;
					break;
				}
			}
			if (exportActor)
			{
				SelectionSetCache.Add(dynamicComponent);
				meshNames.Add(dynamicComponent->MeshName);
			}
		}
	}

	return ExportDynamicObjectArray(SelectionSetCache);

	//return FReply::Handled();
}

FProcHandle FCognitiveEditorTools::ExportDynamicObjectArray(TArray<UDynamicObject*> exportObjects)
{
	FProcHandle fph;
	if (!HasFoundBlender())
	{
		UE_LOG(CognitiveVR_Log, Error, TEXT("Could not complete Export - Must have Blender installed to convert images"));
		return fph;
	}

	FVector originalLocation;
	FRotator originalRotation;
	FVector originalScale;
	int32 ActorsExported = 0;

	TArray<FString> DynamicMeshNames;
	TMap<FString, TArray< UStaticMeshComponent*>> BakeExportMaterials;
	TMap<FString, TArray< USkeletalMeshComponent*>> BakeExportSkeletonMaterials;

	for (int32 i = 0; i < exportObjects.Num(); i++)
	{
		GEditor->SelectNone(false, true, false);

		if (exportObjects[i] == NULL)
		{
			continue;
		}
		if (exportObjects[i]->GetOwner() == NULL)
		{
			continue;
		}
		if (exportObjects[i]->MeshName.IsEmpty())
		{
			continue;
		}

		TArray<UActorComponent*> actorComponents;
		exportObjects[i]->GetOwner()->GetComponents(UStaticMeshComponent::StaticClass(), actorComponents);
		TArray< UStaticMeshComponent*> meshes;
		for (int32 j = 0; j < actorComponents.Num(); j++)
		{
			UStaticMeshComponent* mesh = Cast<UStaticMeshComponent>(actorComponents[j]);
			if (mesh == NULL)
			{
				continue;
			}

			ULevel* componentLevel = actorComponents[j]->GetComponentLevel();
			if (componentLevel->bIsVisible == 0)
			{
				continue;
				//not visible! possibly on a disabled sublevel
			}

			meshes.Add(mesh);
		}

		TArray<UActorComponent*> actorSkeletalComponents;
		exportObjects[i]->GetOwner()->GetComponents(USkeletalMeshComponent::StaticClass(), actorSkeletalComponents);
		TArray< USkeletalMeshComponent*> skeletalMeshes;
		for (int32 j = 0; j < actorSkeletalComponents.Num(); j++)
		{
			USkeletalMeshComponent* mesh = Cast<USkeletalMeshComponent>(actorSkeletalComponents[j]);
			if (mesh == NULL)
			{
				continue;
			}

			ULevel* componentLevel = actorSkeletalComponents[j]->GetComponentLevel();
			if (componentLevel->bIsVisible == 0)
			{
				continue;
				//not visible! possibly on a disabled sublevel
			}

			skeletalMeshes.Add(mesh);
		}

		DynamicMeshNames.Add(exportObjects[i]->MeshName);

		originalLocation = exportObjects[i]->GetOwner()->GetActorLocation();
		originalRotation = exportObjects[i]->GetOwner()->GetActorRotation();
		originalScale = exportObjects[i]->GetOwner()->GetActorScale();

		exportObjects[i]->GetOwner()->SetActorLocation(FVector::ZeroVector);
		exportObjects[i]->GetOwner()->SetActorRotation(FQuat::Identity);
		exportObjects[i]->GetOwner()->SetActorScale3D(FVector::OneVector);

		//export skeletal meshes as fbx (missing material pre 4.26) and static meshes as obj

		FString justDirectory = GetDynamicsExportDirectory() + "/" + exportObjects[i]->MeshName;
		FString tempObject;
		FString ExportFilename;
		ExportFilename = exportObjects[i]->MeshName + ".fbx";
		tempObject = GetDynamicsExportDirectory() + "/" + exportObjects[i]->MeshName + "/" + exportObjects[i]->MeshName + ".fbx";

		//create directory before export
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Dynamic Directory Exists?
		if (!PlatformFile.DirectoryExists(*GetDynamicsExportDirectory()))
		{
			PlatformFile.CreateDirectory(*GetDynamicsExportDirectory());
		}

		// Object Directory Exists?
		if (!PlatformFile.DirectoryExists(*justDirectory))
		{
			PlatformFile.CreateDirectory(*justDirectory);
		}

		GEditor->SelectActor(exportObjects[i]->GetOwner(), true, false, true);
		ActorsExported++;

		//export to obj skips skeletal fbx?
		GLog->Log("FCognitiveEditorTools::ExportDynamicObjectArray dynamic output directory " + tempObject);

		GUnrealEd->ExportMap(GWorld, *tempObject, true);
		//FEditorFileUtils::Export(true);

		exportObjects[i]->GetOwner()->SetActorLocation(originalLocation);
		exportObjects[i]->GetOwner()->SetActorRotation(originalRotation);
		exportObjects[i]->GetOwner()->SetActorScale3D(originalScale);

		//bake + export materials
		//if (skeletalMeshes.Num() > meshes.Num())
		if (skeletalMeshes.Num() > 0)
		{
			BakeExportSkeletonMaterials.Add(exportObjects[i]->MeshName, skeletalMeshes);
		}
		if (meshes.Num() > 0)
		{
			BakeExportMaterials.Add(exportObjects[i]->MeshName, meshes);
		}

		//automatic screenshot
		FLevelEditorViewportClient* perspectiveView = NULL;

		for (int32 j = 0; j < GEditor->GetLevelViewportClients().Num(); j++)
		{
			if (GEditor->GetLevelViewportClients()[j]->ViewportType == LVT_Perspective)
			{
				perspectiveView = GEditor->GetLevelViewportClients()[j];
				break;
			}
		}
		if (perspectiveView != NULL)
		{
			FVector startPosition = perspectiveView->GetViewLocation();
			FRotator startRotation = perspectiveView->GetViewRotation();
			FString dir = BaseExportDirectory + "/dynamics/" + exportObjects[i]->MeshName + "/";
			FTimerHandle DelayScreenshotHandle;

			//calc position
			FVector origin;
			FVector extents;
			exportObjects[i]->GetOwner()->GetActorBounds(false, origin, extents);

			float radius = extents.Size();
			FVector calculatedPosition = exportObjects[i]->GetComponentLocation() + (FVector(-1, -1, 1) * radius);
			FVector calcDir = exportObjects[i]->GetComponentLocation() - calculatedPosition;

			FRotator calcRot = FRotator(calcDir.ToOrientationQuat());

			perspectiveView->SetViewLocation(calculatedPosition);
			perspectiveView->SetViewRotation(calcRot);

			perspectiveView->bNeedsRedraw = true;
			perspectiveView->Viewport->Draw(false);
			FCognitiveEditorTools::DelayScreenshot(dir, perspectiveView, startPosition, startRotation);
		}
	}

	GLog->Log("FCognitiveEditorTools::ExportDynamicObjectArray Found " + FString::FromInt(ActorsExported) + " meshes for export");

	if (ActorsExported > 0)
	{
		fph = ConvertDynamicsToGLTF(DynamicMeshNames);
		FindAllSubDirectoryNames();
	}
	return fph;
}


void FCognitiveEditorTools::DelayScreenshot(FString filePath, FLevelEditorViewportClient* perspectiveView, FVector startPos, FRotator startRot)
{
	UThumbnailManager::CaptureProjectThumbnail(perspectiveView->Viewport, filePath + "cvr_object_thumbnail.png", false);

	perspectiveView->SetViewLocation(startPos);
	perspectiveView->SetViewRotation(startRot);
	perspectiveView->bNeedsRedraw = true;
	perspectiveView->RedrawRequested(perspectiveView->Viewport);
}

FProcHandle FCognitiveEditorTools::ConvertDynamicsToGLTF(TArray<FString> meshnames)
{
	FProcHandle fph;
	//open blender
	//pass in array of meshnames comma separated

	FString pythonscriptpath = IPluginManager::Get().FindPlugin(TEXT("CognitiveVR"))->GetBaseDir() / TEXT("Resources") / TEXT("ConvertDynamicToGLTF.py");
	const TCHAR* charPath = *pythonscriptpath;

	//found something
	UE_LOG(LogTemp, Log, TEXT("FCognitiveEditorTools::ConvertToGLTF Python script path: %s"), charPath);


	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();


	TArray<FAssetData> ScriptList;
	if (!AssetRegistry.GetAssetsByPath(FName(*pythonscriptpath), ScriptList))
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::ConvertToGLTF Could not find python script at path. Canceling"));
		return fph;
	}

	FString stringurl = BlenderPath;

	if (BlenderPath.Len() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::ConvertToGLTF No path set for Blender.exe. Canceling"));
		return fph;
	}

	UWorld* tempworld = GEditor->GetEditorWorldContext().World();
	if (!tempworld)
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::ConvertToGLTF World is null. canceling"));
		return fph;
	}

	FString ObjPath = GetDynamicsExportDirectory();

	if (ObjPath.Len() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::ConvertToGLTF No know export directory. Canceling"));
		return fph;
	}

	FString escapedMeshNameList = "'";
	for (int32 i = 0; i < meshnames.Num(); i++)
	{
		if (i != 0)
			escapedMeshNameList += ",";
		escapedMeshNameList += meshnames[i];
	}
	escapedMeshNameList += "'";


	//FString resizeFactor = FString::FromInt(TextureRefactor);

	FString escapedPythonPath = pythonscriptpath.Replace(TEXT(" "), TEXT("\" \""));
	FString escapedTargetPath = ObjPath.Replace(TEXT(" "), TEXT("\" \""));

	FString stringparams = " -P " + escapedPythonPath + " " + escapedTargetPath + " " + escapedMeshNameList;// +" " + MaxPolyCount + " " + SceneName;

	FString stringParamSlashed = stringparams.Replace(TEXT("\\"), TEXT("/"));

	const TCHAR* params = *stringParamSlashed;
	int32 priorityMod = 0;
	fph = FPlatformProcess::CreateProc(*BlenderPath, params, false, false, false, NULL, priorityMod, 0, nullptr);
	return fph;
}

FReply FCognitiveEditorTools::SetUniqueDynamicIds()
{
	//loop thorugh all dynamics in the scene
	TArray<UDynamicObject*> dynamics;

	//make a list of all the used objectids

	TArray<FDynamicObjectId> usedIds;

	//get all the dynamic objects in the scene
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		//AStaticMeshActor *Mesh = *ActorItr;

		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		dynamics.Add(dynamic);
	}

	//create objectids for each dynamic that's already set
	for (auto& dynamic : dynamics)
	{
		FString finalMeshName = dynamic->MeshName;
		if (!dynamic->UseCustomMeshName)
		{
			if (dynamic->CommonMeshName == ECommonMeshName::ViveController) { finalMeshName = "vivecontroller"; }
			if (dynamic->CommonMeshName == ECommonMeshName::ViveTracker) { finalMeshName = "vivecontroller"; }
			if (dynamic->CommonMeshName == ECommonMeshName::OculusRiftTouchRight) { finalMeshName = "oculusrifttouchright"; }
			if (dynamic->CommonMeshName == ECommonMeshName::OculusRiftTouchLeft) { finalMeshName = "oculusrifttouchleft"; }
			if (dynamic->CommonMeshName == ECommonMeshName::WindowsMixedRealityRight) { finalMeshName = "windows_mixed_reality_controller_right"; }
			if (dynamic->CommonMeshName == ECommonMeshName::WindowsMixedRealityLeft) { finalMeshName = "windows_mixed_reality_controller_left"; }
			if (dynamic->CommonMeshName == ECommonMeshName::PicoNeo2EyeControllerRight) { finalMeshName = "pico_neo_2_eye_controller_right"; }
			if (dynamic->CommonMeshName == ECommonMeshName::PicoNeo2EyeControllerLeft) { finalMeshName = "pico_neo_2_eye_controller_left"; }
		}
	}

	//int32 currentUniqueId = 1;
	int32 changedDynamics = 0;

	//unassigned or invalid numbers
	TArray<UDynamicObject*> UnassignedDynamics;

	//try to put all ids back where they were
	for (auto& dynamic : dynamics)
	{
		//id dynamic custom id is not in usedids - add it

		if (dynamic->MeshName.IsEmpty())
		{
			dynamic->TryGenerateMeshName();
		}
		FString findId = dynamic->CustomId;

		FDynamicObjectId* FoundId = usedIds.FindByPredicate([findId](const FDynamicObjectId& InItem)
		{
			return InItem.Id == findId;
		});

		if (FoundId == NULL && dynamic->CustomId != "")
		{
			usedIds.Add(FDynamicObjectId(dynamic->CustomId, dynamic->MeshName));
		}
		else
		{
			//assign a new and unused id
			UnassignedDynamics.Add(dynamic);
		}
	}

	for (auto& dynamic : UnassignedDynamics)
	{
		dynamic->GenerateCustomId();
		usedIds.Add(FDynamicObjectId(dynamic->CustomId, dynamic->MeshName));
		changedDynamics++;
	}

	GLog->Log("CognitiveVR Tools set " + FString::FromInt(changedDynamics) + " dynamic ids");

	GWorld->MarkPackageDirty();
	//save the scene? mark the scene as changed?

	RefreshDisplayDynamicObjectsCountInScene();

	return FReply::Handled();
}

FReply FCognitiveEditorTools::UploadDynamicsManifest()
{
	TArray<UDynamicObject*> dynamics;

	//get all the dynamic objects in the scene
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		//AStaticMeshActor *Mesh = *ActorItr;

		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		dynamics.Add(dynamic);
	}

	GLog->Log("CognitiveVR Tools uploading manifest for " + FString::FromInt(dynamics.Num()) + " objects");

	bool wroteAnyObjects = false;
	FString objectManifest = "{\"objects\":[";
	//write preset customids to manifest
	for (int32 i = 0; i < dynamics.Num(); i++)
	{
		//if they have a customid -> add them to the objectmanifest string
		if (dynamics[i]->IdSourceType == EIdSourceType::CustomId && dynamics[i]->CustomId != "")
		{
			FVector location = dynamics[i]->GetComponentLocation();
			FQuat rotation = dynamics[i]->GetComponentQuat();
			FVector scale = dynamics[i]->GetComponentScale();

			wroteAnyObjects = true;
			objectManifest += "{";
			objectManifest += "\"id\":\"" + dynamics[i]->CustomId + "\",";
			objectManifest += "\"mesh\":\"" + dynamics[i]->MeshName + "\",";
			objectManifest += "\"name\":\"" + dynamics[i]->GetOwner()->GetName() + "\",";
			objectManifest += "\"scaleCustom\":[" + FString::SanitizeFloat(scale.X) + "," + FString::SanitizeFloat(scale.Z) + "," + FString::SanitizeFloat(scale.Y) + "],";
			objectManifest += "\"initialPosition\":[" + FString::SanitizeFloat(-location.X) + "," + FString::SanitizeFloat(location.Z) + "," + FString::SanitizeFloat(location.Y) + "],";
			objectManifest += "\"initialRotation\":[" + FString::SanitizeFloat(-rotation.X) + "," + FString::SanitizeFloat(rotation.Z) + "," + FString::SanitizeFloat(-rotation.Y) + "," + FString::SanitizeFloat(rotation.W) + "]";
			objectManifest += "},";
		}
	}
	if (!wroteAnyObjects)
	{
		GLog->Log("Couldn't find any dynamic objects to put into the aggregation manifest!");
		FCognitiveEditorTools::OnUploadManifestCompleted(NULL, NULL, true);
		return FReply::Handled();
	}
	//remove last comma
	objectManifest.RemoveFromEnd(",");
	//add ]}
	objectManifest += "]}";


	//get scene id
	TSharedPtr<FEditorSceneData> currentSceneData = GetCurrentSceneData();
	if (!currentSceneData.IsValid())
	{
		GLog->Log("FCognitiveEditorTools::UploadDynamicObjectManifest could not find current scene id");
		return FReply::Handled();
	}

	if (currentSceneData->Id == "")
	{
		GLog->Log("CognitiveToolsCustomization::UploadDynamicsManifest couldn't find sceneid for current scene");
		return FReply::Handled();
	}
	if (currentSceneData->VersionNumber == 0)
	{
		GLog->Log("CognitiveTools::UploadDynamicsManifest current scene does not have valid version number. GetSceneVersions and try again");
		return FReply::Handled();
	}

	FString url = PostDynamicObjectManifest(currentSceneData->Id, currentSceneData->VersionNumber);

	//send manifest to api/objects/sceneid

	GLog->Log("CognitiveTools::UploadDynamicsManifest send dynamic object aggregation manifest");
	FString AuthValue = "APIKEY:DEVELOPER " + FAnalyticsCognitiveVR::Get().DeveloperKey;
	auto HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(url);
	HttpRequest->SetVerb("POST");
	HttpRequest->SetHeader("Content-Type", TEXT("application/json"));
	HttpRequest->SetHeader("Authorization", AuthValue);
	HttpRequest->SetContentAsString(objectManifest);
	GLog->Log(objectManifest);

	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FCognitiveEditorTools::OnUploadManifestCompleted);

	HttpRequest->ProcessRequest();

	return FReply::Handled();
}

FReply FCognitiveEditorTools::UploadDynamicsManifestIds(TArray<FString> ids, FString meshName, FString prefabName)
{
	//this is used by dynamic object id pool. doesn't have initial positions, rotations, scale
	GLog->Log("CognitiveVR Tools uploading manifest for " + FString::FromInt(ids.Num()) + " objects");

	bool wroteAnyObjects = false;
	FString objectManifest = "{\"objects\":[";
	//write preset customids to manifest
	for (int32 i = 0; i < ids.Num(); i++)
	{
		//if they have a customid -> add them to the objectmanifest string
		if (ids[i] != "")
		{
			wroteAnyObjects = true;
			objectManifest += "{";
			objectManifest += "\"id\":\"" + ids[i] + "\",";
			objectManifest += "\"mesh\":\"" + meshName + "\",";
			objectManifest += "\"name\":\"" + prefabName + "\"";

			objectManifest += "\"scaleCustom\":[1,1,1],";
			objectManifest += "\"initialPosition\":[0,0,0],";
			objectManifest += "\"initialRotation\":[0,0,0,1]";
			objectManifest += "},";
		}
	}
	if (!wroteAnyObjects)
	{
		GLog->Log("Couldn't find any dynamic objects to put into the aggregation manifest!");
		return FReply::Handled();
	}
	//remove last comma
	objectManifest.RemoveFromEnd(",");
	//add ]}
	objectManifest += "]}";


	//get scene id
	TSharedPtr<FEditorSceneData> currentSceneData = GetCurrentSceneData();
	if (!currentSceneData.IsValid())
	{
		GLog->Log("FCognitiveEditorTools::UploadDynamicObjectManifest could not find current scene id");
		return FReply::Handled();
	}

	if (currentSceneData->Id == "")
	{
		GLog->Log("CognitiveToolsCustomization::UploadDynamicsManifest couldn't find sceneid for current scene");
		return FReply::Handled();
	}
	if (currentSceneData->VersionNumber == 0)
	{
		GLog->Log("CognitiveTools::UploadDynamicsManifest current scene does not have valid version number. GetSceneVersions and try again");
		return FReply::Handled();
	}

	FString url = PostDynamicObjectManifest(currentSceneData->Id, currentSceneData->VersionNumber);

	//send manifest to api/objects/sceneid

	GLog->Log("CognitiveTools::UploadDynamicsManifest send dynamic object aggregation manifest");
	FString AuthValue = "APIKEY:DEVELOPER " + FAnalyticsCognitiveVR::Get().DeveloperKey;
	auto HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(url);
	HttpRequest->SetVerb("POST");
	HttpRequest->SetHeader("Content-Type", TEXT("application/json"));
	HttpRequest->SetHeader("Authorization", AuthValue);
	HttpRequest->SetContentAsString(objectManifest);

	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FCognitiveEditorTools::OnUploadManifestCompleted);

	HttpRequest->ProcessRequest();

	return FReply::Handled();
}

void FCognitiveEditorTools::OnUploadManifestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	WizardUploading = false;
	if (Response.Get() == NULL) //likely no aggregation manifest to upload. no request, no response
	{
		GetDynamicsManifest();
		return;
	}

	if (bWasSuccessful && Response->GetResponseCode() < 300) //successfully uploaded
	{
		GetDynamicsManifest();
		WizardUploadError = FString::FromInt(Response->GetResponseCode());
		WizardUploadResponseCode = Response->GetResponseCode();
	}
	else //upload failed
	{
		WizardUploadError = FString("FCognitiveEditorTools::OnUploadManifestCompleted response code ") + FString::FromInt(Response->GetResponseCode());
		WizardUploadResponseCode = Response->GetResponseCode();
		GLog->Log("FCognitiveEditorTools::OnUploadManifestCompleted response code " + FString::FromInt(Response->GetResponseCode()));
	}
}

FReply FCognitiveEditorTools::GetDynamicsManifest()
{
	TSharedPtr<FEditorSceneData> currentSceneData = GetCurrentSceneData();
	if (!currentSceneData.IsValid())
	{
		GLog->Log("CognitiveTools::GetDyanmicManifest could not find current scene data");
		return FReply::Handled();
	}
	if (currentSceneData->VersionId == 0)
	{
		GLog->Log("CognitiveTools::GetDyanmicManifest version id is not set! Makes sure the scene has updated scene version");
		return FReply::Handled();
	}
	if (!HasDeveloperKey())
	{
		GLog->Log("CognitiveTools::GetDyanmicManifest auth token is empty. Must log in!");
		return FReply::Handled();
	}

	auto HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->SetURL(GetDynamicObjectManifest(FString::FromInt(currentSceneData->VersionId)));

	HttpRequest->SetHeader("X-HTTP-Method-Override", TEXT("GET"));
	FString AuthValue = "APIKEY:DEVELOPER " + FAnalyticsCognitiveVR::Get().DeveloperKey;
	HttpRequest->SetHeader("Authorization", AuthValue);

	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FCognitiveEditorTools::OnDynamicManifestResponse);
	HttpRequest->ProcessRequest();
	return FReply::Handled();
}

void FCognitiveEditorTools::OnDynamicManifestResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
		GLog->Log("FCognitiveEditorTools::OnDynamicManifestResponse response code " + FString::FromInt(Response->GetResponseCode()));
	else
	{
		GLog->Log("FCognitiveEditorTools::OnDynamicManifestResponse failed to connect");
		WizardUploading = false;
		WizardUploadError = "FCognitiveEditorTools::OnDynamicManifestResponse failed to connect";
		WizardUploadResponseCode = 0;
		return;
	}

	WizardUploadResponseCode = Response->GetResponseCode();

	if (bWasSuccessful && Response->GetResponseCode() < 300)
	{
		//GLog->Log("CognitiveTools::OnDynamicManifestResponse content: " + Response->GetContentAsString());

		//do json stuff to this

		TSharedPtr<FJsonValue> JsonDynamics;

		TSharedRef<TJsonReader<>>Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonDynamics))
		{
			int32 count = JsonDynamics->AsArray().Num();
			GLog->Log("FCognitiveEditorTools::OnDynamicManifestResponse returned " + FString::FromInt(count) + " objects");
			for (int32 i = 0; i < count; i++)
			{
				TSharedPtr<FJsonObject> jsonobject = JsonDynamics->AsArray()[i]->AsObject();
				FString name = jsonobject->GetStringField("name");
				FString meshname = jsonobject->GetStringField("meshName");
				FString id = jsonobject->GetStringField("sdkId");

				SceneExplorerDynamics.Add(MakeShareable(new FDynamicData(name, meshname, id)));
			}
		}
	}
	else
	{
		WizardUploadError = "FCognitiveEditorTools::OnDynamicManifestResponse response code " + FString::FromInt(Response->GetResponseCode());
	}
	if (WizardUploading)
	{
		WizardUploading = false;
	}
}
int32 OutstandingDynamicUploadRequests = 0;

FReply FCognitiveEditorTools::UploadDynamics()
{
	FString filesStartingWith = TEXT("");
	FString pngextension = TEXT("png");

	// Get all files in directory
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> DirectoriesToSkip;
	TArray<FString> DirectoriesToNotRecurse;

	// use the timestamp grabbing visitor (include directories)
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, true);
	Visitor.Visit(*GetDynamicsExportDirectory(), true);

	TArray<FString> dynamicNames;
	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		if (TimestampIt.Key() == GetDynamicsExportDirectory() || (!TimestampIt.Key().EndsWith(".png")))
		{
			GLog->Log("skip file - not dynamic " + TimestampIt.Key());
		}
		else
		{
			GLog->Log("upload dynamic " + FPaths::GetCleanFilename(TimestampIt.Key()));
			dynamicNames.Add(FPaths::GetCleanFilename(TimestampIt.Key()));
		}
	}

	ReadSceneDataFromFile();

	GLog->Log("FCognitiveEditorTools::UploadDynamics found " + FString::FromInt(dynamicNames.Num()) + " exported dynamic objects");
	TSharedPtr<FEditorSceneData> currentSceneData = GetCurrentSceneData();

	if (!currentSceneData.IsValid())
	{
		GLog->Log("FCognitiveEditorToolsCustomization::UploadDynamics can't find current scene!");
		return FReply::Handled();
	}

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		const FString filePath = TimestampIt.Key();
		const FString fileName = FPaths::GetCleanFilename(filePath);

		if (GetDynamicsExportDirectory() == filePath)
		{
			//GLog->Log("root found " + filePath);
		}
		else if (FPaths::DirectoryExists(filePath))
		{
			//GLog->Log("directory found " + filePath);
			FString url = PostDynamicObjectMeshData(currentSceneData->Id, currentSceneData->VersionNumber, fileName);
			GLog->Log("dynamic upload to url " + url);

			UploadFromDirectory(url, filePath, "object");
			OutstandingDynamicUploadRequests++;
		}
		else
		{
			//GLog->Log("file found " + filePath);
		}
	}

	if (OutstandingDynamicUploadRequests == 0 && WizardUploading)
	{
		GLog->Log("FCognitiveEditorTools::UploadDynamics has no dynamics to upload!");
		WizardUploading = false;
	}

	return FReply::Handled();
}

FReply FCognitiveEditorTools::UploadDynamic(FString directory)
{
	FString filesStartingWith = TEXT("");
	FString pngextension = TEXT("png");

	// Get all files in directory
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> DirectoriesToSkip;
	TArray<FString> DirectoriesToNotRecurse;

	FString singleDir = GetDynamicsExportDirectory() + "/" + directory;

	// use the timestamp grabbing visitor (include directories)
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, true);
	Visitor.Visit(*singleDir, true);

	TArray<FString> dynamicNames;
	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		if (TimestampIt.Key().EndsWith(".png"))
		{
			GLog->Log("upload dynamic " + FPaths::GetCleanFilename(TimestampIt.Key()));
			dynamicNames.Add(FPaths::GetCleanFilename(TimestampIt.Key()));
		}
		else
		{
			GLog->Log("skip file - not dynamic " + TimestampIt.Key());
		}
	}

	ReadSceneDataFromFile();

	GLog->Log("FCognitiveEditorTools::UploadDynamics found " + FString::FromInt(dynamicNames.Num()) + " exported dynamic objects");
	TSharedPtr<FEditorSceneData> currentSceneData = GetCurrentSceneData();

	if (!currentSceneData.IsValid())
	{
		GLog->Log("FCognitiveEditorToolsCustomization::UploadDynamics can't find current scene!");
		return FReply::Handled();
	}

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		const FString filePath = TimestampIt.Key();
		const FString fileName = FPaths::GetCleanFilename(filePath);

		if (GetDynamicsExportDirectory() == filePath)
		{
			GLog->Log("root found " + filePath);
		}
		else if (FPaths::DirectoryExists(filePath))
		{
			//GLog->Log("directory found " + filePath);
			FString url = PostDynamicObjectMeshData(currentSceneData->Id, currentSceneData->VersionNumber, fileName);
			GLog->Log("dynamic upload to url " + url);

			UploadFromDirectory(url, filePath, "object");
			OutstandingDynamicUploadRequests++;
		}
		else
		{
			//GLog->Log("file found " + filePath);
		}
	}

	if (OutstandingDynamicUploadRequests == 0)
	{
		GLog->Log("FCognitiveEditorTools::UploadDynamics has no dynamics to upload!");
	}
	else
	{
		GLog->Log("FCognitiveEditorTools::UploadDynamics uploaded a Mesh");
	}

	return FReply::Handled();
}

TArray<TSharedPtr<FString>> FCognitiveEditorTools::GetSubDirectoryNames()
{
	return SubDirectoryNames;
}

void FCognitiveEditorTools::FindAllSubDirectoryNames()
{
	// Get all files in directory
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> DirectoriesToSkip;
	TArray<FString> DirectoriesToNotRecurse;

	// use the timestamp grabbing visitor (include directories)
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, true);
	Visitor.Visit(*GetDynamicsExportDirectory(), true);

	//no matches anywhere
	SubDirectoryNames.Empty();

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		if (PlatformFile.DirectoryExists(*TimestampIt.Key()) && TimestampIt.Key().Contains("dynamics") && !TimestampIt.Key().EndsWith("dynamics"))
		{
			SubDirectoryNames.Add(MakeShareable(new FString(FPaths::GetCleanFilename(TimestampIt.Key()))));
		}
	}
}

void FCognitiveEditorTools::CreateExportFolderStructure()
{
	VerifyOrCreateDirectory(BaseExportDirectory);
	FString temp = GetDynamicsExportDirectory();
	VerifyOrCreateDirectory(temp);
}

FReply FCognitiveEditorTools::Select_Blender()
{
	FString title = "Select Blender.exe";
	FString fileTypes = ".exe";
	FString lastPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::UNR);
	FString defaultfile = FString();
	FString outFilename = FString();
	if (PickFile(title, fileTypes, lastPath, defaultfile, outFilename))
	{
		BlenderPath = outFilename;
		FString EditorIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEditor.ini"));
		GConfig->SetString(TEXT("Analytics"), TEXT("BlenderPath"), *FCognitiveEditorTools::GetInstance()->BlenderPath, EditorIni);
	}
	return FReply::Handled();
}

FReply FCognitiveEditorTools::SelectBaseExportDirectory()
{
	FString title = "Select Export Directory";
	FString fileTypes = ".exe";
	FString lastPath = FPaths::ProjectDir();
	FString defaultfile = FString();
	FString outFilename = FString();
	if (PickDirectory(title, fileTypes, lastPath, defaultfile, outFilename))
	{
		BaseExportDirectory = outFilename;
		FString EditorIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEditor.ini"));
		GConfig->SetString(TEXT("Analytics"), TEXT("ExportPath"), *FCognitiveEditorTools::GetInstance()->BaseExportDirectory, EditorIni);
	}
	else
	{
		BaseExportDirectory = "";
		FString EditorIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEditor.ini"));
		GConfig->SetString(TEXT("Analytics"), TEXT("ExportPath"), *FCognitiveEditorTools::GetInstance()->BaseExportDirectory, EditorIni);
	}
	return FReply::Handled();
}


//used to select blender
bool FCognitiveEditorTools::PickFile(const FString& Title, const FString& FileTypes, FString& InOutLastPath, const FString& DefaultFile, FString& OutFilename)
{
	OutFilename = FString();

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bFileChosen = false;
	TArray<FString> OutFilenames;
	if (DesktopPlatform)
	{
		void* ParentWindowWindowHandle = ChooseParentWindowHandle();

		bFileChosen = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			Title,
			InOutLastPath,
			DefaultFile,
			FileTypes,
			EFileDialogFlags::None,
			OutFilenames
		);
	}

	bFileChosen = (OutFilenames.Num() > 0);

	if (bFileChosen)
	{
		// User successfully chose a file; remember the path for the next time the dialog opens.
		InOutLastPath = OutFilenames[0];
		OutFilename = OutFilenames[0];
	}

	return bFileChosen;
}

FReply FCognitiveEditorTools::TakeScreenshot()
{
	FString dir = BaseExportDirectory + "/" + GetCurrentSceneName() + "/screenshot/";
	if (VerifyOrCreateDirectory(dir))
	{
		FScreenshotRequest::RequestScreenshot(dir + "screenshot", false, false);
	}
	return FReply::Handled();
}

FReply FCognitiveEditorTools::TakeDynamicScreenshot(FString dynamicName)
{
	FString dir = BaseExportDirectory + "/dynamics/" + dynamicName + "/";
	if (VerifyOrCreateDirectory(dir))
	{
		FScreenshotRequest::RequestScreenshot(dir + "cvr_object_thumbnail", false, false);
	}
	return FReply::Handled();
}

void FCognitiveEditorTools::OnUploadScreenshotCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
		GLog->Log("FCognitiveEditorTools::OnUploadScreenshotCompleted response code " + FString::FromInt(Response->GetResponseCode()));
	else
	{
		GLog->Log("FCognitiveEditorTools::OnUploadScreenshotCompleted failed to connect");
		return;
	}
}

bool FCognitiveEditorTools::PickDirectory(const FString& Title, const FString& FileTypes, FString& InOutLastPath, const FString& DefaultFile, FString& OutFilename)
{
	OutFilename = FString();

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool directoryChosen = false;
	TArray<FString> OutFilenames;
	if (DesktopPlatform)
	{
		void* ParentWindowWindowHandle = ChooseParentWindowHandle();

		directoryChosen = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowWindowHandle,
			Title,
			InOutLastPath,
			OutFilename
		);
	}

	return directoryChosen;
}

void* FCognitiveEditorTools::ChooseParentWindowHandle()
{
	void* ParentWindowWindowHandle = NULL;
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	return ParentWindowWindowHandle;
}

FReply FCognitiveEditorTools::UploadScene()
{
	FString url = "";

	//get scene name
	//look if scene name has an entry in the scene datas
	TSharedPtr<FEditorSceneData> sceneData = GetCurrentSceneData();
	if (sceneData.IsValid() && sceneData->Id.Len() > 0)
	{
		//GLog->Log("post update existing scene");
		//existing uploaded scene
		url = PostUpdateScene(sceneData->Id);
	}
	else
	{
		//GLog->Log("post new scene");
		//new scene
		url = PostNewScene();
	}

	GLog->Log("FCognitiveEditorTools::UploadScene upload scene to " + url);
	UploadFromDirectory(url, GetCurrentSceneExportDirectory(), "scene");
	//IMPROVEMENT listen for response. when the response returns, request the scene version with auth token

	return FReply::Handled();
}

void FCognitiveEditorTools::RefreshSceneUploadFiles()
{
	FString filesStartingWith = TEXT("");
	FString pngextension = TEXT("png");
	TArray<FString> filesInDirectory = GetAllFilesInDirectory(GetCurrentSceneExportDirectory(), true, filesStartingWith, filesStartingWith, pngextension, false);

	TArray<FString> imagesInDirectory = GetAllFilesInDirectory(GetCurrentSceneExportDirectory(), true, filesStartingWith, pngextension, filesStartingWith, false);
	imagesInDirectory.Remove(GetCurrentSceneExportDirectory() + "/screenshot/screenshot.png");

	SceneUploadFiles.Empty();
	for (int32 i = 0; i < filesInDirectory.Num(); i++)
	{
		SceneUploadFiles.Add(MakeShareable(new FString(filesInDirectory[i])));
	}
	for (int32 i = 0; i < imagesInDirectory.Num(); i++)
	{
		SceneUploadFiles.Add(MakeShareable(new FString(imagesInDirectory[i])));
	}
}

void FCognitiveEditorTools::RefreshDynamicUploadFiles()
{
	FString filesStartingWith = TEXT("");
	FString pngextension = TEXT("png");
	TArray<FString> filesInDirectory = GetAllFilesInDirectory(BaseExportDirectory + "/dynamics", true, filesStartingWith, filesStartingWith, pngextension, false);

	TArray<FString> imagesInDirectory = GetAllFilesInDirectory(BaseExportDirectory + "/dynamics", true, filesStartingWith, pngextension, filesStartingWith, false);

	DynamicUploadFiles.Empty();
	for (int32 i = 0; i < filesInDirectory.Num(); i++)
	{
		DynamicUploadFiles.Add(MakeShareable(new FString(filesInDirectory[i])));
	}
	for (int32 i = 0; i < imagesInDirectory.Num(); i++)
	{
		DynamicUploadFiles.Add(MakeShareable(new FString(imagesInDirectory[i])));
	}
}

void FCognitiveEditorTools::UploadFromDirectory(FString url, FString directory, FString expectedResponseType)
{
	FString filesStartingWith = TEXT("");
	FString pngextension = TEXT("png");
	TArray<FString> filesInDirectory = GetAllFilesInDirectory(directory, true, filesStartingWith, filesStartingWith, filesStartingWith, true);

	//TArray<FString> imagesInDirectory = GetAllFilesInDirectory(directory, true, filesStartingWith, pngextension, filesStartingWith, true);

	filesInDirectory.Remove(directory + "/screenshot/screenshot.png");
	//imagesInDirectory.Remove(directory + "/screenshot/screenshot.png");
	FString screenshotPath = directory + "/screenshot/screenshot.png";

	TArray<FContentContainer> contentArray;

	//UE_LOG(LogTemp, Log, TEXT("UploadScene image count%d"), imagesInDirectory.Num());
	UE_LOG(LogTemp, Log, TEXT("UploadScene all file count%d"), filesInDirectory.Num());

	for (int32 i = 0; i < filesInDirectory.Num(); i++)
	{
		FString Content;
		TArray<uint8> byteResult;
		if (FFileHelper::LoadFileToArray(byteResult, *filesInDirectory[i]))
		{
			FContentContainer container = FContentContainer();
			UE_LOG(LogTemp, Log, TEXT("Loaded file %s"), *filesInDirectory[i]);
			//loaded the file

			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append("--cJkER9eqUVwt2tgnugnSBFkGLAgt7djINNHkQP0i");
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append("Content-Type: application/octet-stream");
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append("Content-disposition: form-data; name=\"file\"; filename=\"" + FPaths::GetCleanFilename(filesInDirectory[i]) + "\"");
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append(TEXT("\r\n"));

			container.Headers = Content;
			container.BodyBinary = byteResult;

			contentArray.Add(container);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("failed to load %s"), *filesInDirectory[i]);
		}
	}

	//append screenshot
	FString Content;
	TArray<uint8> byteResult;
	if (FPaths::FileExists(*screenshotPath))
	{
		if (FFileHelper::LoadFileToArray(byteResult, *screenshotPath))
		{
			FContentContainer container = FContentContainer();
			//UE_LOG(LogTemp, Log, TEXT("Loaded image %s"), *imagesInDirectory[i]);
			//loaded the file
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append("--cJkER9eqUVwt2tgnugnSBFkGLAgt7djINNHkQP0i");
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append("Content-Type: image/png");
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append("Content-disposition: form-data; name=\"screenshot\"; filename=\"screenshot.png\"");
			Content = Content.Append(TEXT("\r\n"));
			Content = Content.Append(TEXT("\r\n"));

			container.Headers = Content;
			container.BodyBinary = byteResult;

			contentArray.Add(container);
		}
		else
		{
			//GLog->Log("couldn't find screenshot to upload");
		}
	}
	else
	{
		//GLog->Log("screenshot path doesn't exist -------- " + screenshotPath);
	}

	TArray<uint8> AllBytes;
	auto HttpRequest = FHttpModule::Get().CreateRequest();

	for (int32 i = 0; i < contentArray.Num(); i++)
	{
		//reference
		//RequestPayload.SetNumUninitialized(Converter.Length());
		//FMemory::Memcpy(RequestPayload.GetData(), (const uint8*)Converter.Get(), RequestPayload.Num());


		//headers
		FTCHARToUTF8 Converter(*contentArray[i].Headers);
		auto data = (const uint8*)Converter.Get();
		AllBytes.Append(data, Converter.Length());

		//content
		if (contentArray[i].BodyText.Len() > 0)
		{
			FTCHARToUTF8 ConverterBody(*contentArray[i].BodyText);
			auto bodydata = (const uint8*)ConverterBody.Get();
			//TArray<uint8> outbytes;
			//StringToBytes((ANSI_TO_TCHAR(contentArray[i].BodyText*)), outbytes);

			AllBytes.Append(bodydata, ConverterBody.Length());
		}
		else
		{
			AllBytes.Append(contentArray[i].BodyBinary);
		}
	}



	TArray<uint8> EndBytes;
	FString EndString;

	EndString = TEXT("\r\n");
	FTCHARToUTF8 ConverterEnd1(*EndString);
	auto enddata1 = (const uint8*)ConverterEnd1.Get();
	AllBytes.Append(enddata1, ConverterEnd1.Length());

	EndString = TEXT("--cJkER9eqUVwt2tgnugnSBFkGLAgt7djINNHkQP0i--");
	FTCHARToUTF8 ConverterEnd2(*EndString);
	auto enddata2 = (const uint8*)ConverterEnd2.Get();
	AllBytes.Append(enddata2, ConverterEnd2.Length());

	EndString = TEXT("\r\n");
	FTCHARToUTF8 ConverterEnd3(*EndString);
	auto enddata3 = (const uint8*)ConverterEnd3.Get();
	AllBytes.Append(enddata3, ConverterEnd3.Length());

	HttpRequest->SetURL(url);
	HttpRequest->SetHeader("Content-Type", "multipart/form-data; boundary=\"cJkER9eqUVwt2tgnugnSBFkGLAgt7djINNHkQP0i\"");
	HttpRequest->SetHeader("Accept-Encoding", "identity");
	FString AuthValue = "APIKEY:DEVELOPER " + FAnalyticsCognitiveVR::Get().DeveloperKey;
	HttpRequest->SetHeader("Authorization", AuthValue);
	HttpRequest->SetVerb("POST");
	HttpRequest->SetContent(AllBytes);

	FHttpModule::Get().SetHttpTimeout(0);

	if (expectedResponseType == "scene")
	{
		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FCognitiveEditorTools::OnUploadSceneCompleted);
	}
	if (expectedResponseType == "object")
	{
		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FCognitiveEditorTools::OnUploadObjectCompleted);
	}

	HttpRequest->ProcessRequest();
}

void FCognitiveEditorTools::OnUploadSceneCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
		GLog->Log("FCognitiveEditorTools::OnUploadSceneCompleted response code " + FString::FromInt(Response->GetResponseCode()));
	else
	{
		GLog->Log("FCognitiveEditorTools::OnUploadSceneCompleted failed to connect");
		WizardUploadError = "FCognitiveEditorTools::OnUploadSceneCompleted failed to connect";
		WizardUploading = false;
		WizardUploadResponseCode = 0;
		return;
	}
	WizardUploadResponseCode = Response->GetResponseCode();

	if (bWasSuccessful && Response->GetResponseCode() < 300)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Upload Scene Response is %s"), *Response->GetContentAsString());

		UWorld* myworld = GWorld->GetWorld();
		if (myworld == NULL)
		{
			UE_LOG(LogTemp, Error, TEXT("Upload Scene - No world!"));
			WizardUploadError = "FCognitiveEditorTools::OnUploadSceneCompleted no world";
			return;
		}

		FString currentSceneName = myworld->GetMapName();
		currentSceneName.RemoveFromStart(myworld->StreamingLevelsPrefix);

		FString responseNoQuotes = *Response->GetContentAsString().Replace(TEXT("\""), TEXT(""));

		if (responseNoQuotes.Len() > 0)
		{
			SaveSceneData(currentSceneName, responseNoQuotes);
			ReadSceneDataFromFile();
		}
		else
		{
			//successfully uploaded a scene but no response - updated an existing scene version
			ReadSceneDataFromFile();
		}
		ConfigFileHasChanged = true;

		if (WizardUploading)
		{
			CurrentSceneVersionRequest();
			//ReadSceneDataFromFile();

			//UploadDynamics();
		}
	}
	else
	{
		WizardUploading = false;
		WizardUploadError = "FCognitiveEditorTools::OnUploadSceneCompleted response code " + FString::FromInt(Response->GetResponseCode());
	}
}

void FCognitiveEditorTools::OnUploadObjectCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	OutstandingDynamicUploadRequests--;

	if (bWasSuccessful)
		GLog->Log("FCognitiveEditorTools::OnUploadObjectCompleted response code " + FString::FromInt(Response->GetResponseCode()));
	else
	{
		GLog->Log("FCognitiveEditorTools::OnUploadObjectCompleted failed to connect");
		WizardUploading = false;
		WizardUploadError = "FCognitiveEditorTools::OnUploadObjectCompleted failed to connect";
		WizardUploadResponseCode = 0;
		return;
	}

	WizardUploadResponseCode = Response->GetResponseCode();

	if (bWasSuccessful && Response->GetResponseCode() < 300)
	{
		FString responseNoQuotes = *Response->GetContentAsString().Replace(TEXT("\""), TEXT(""));
		//GLog->Log("Upload Dynamic Complete " + Request->GetURL());
	}
	else
	{
		WizardUploading = false;
		WizardUploadError = "FCognitiveEditorTools::OnUploadObjectCompleted response code " + FString::FromInt(Response->GetResponseCode());
	}

	if (WizardUploading && OutstandingDynamicUploadRequests <= 0)
	{
		//upload manifest
		UploadDynamicsManifest();
	}
}

//https://answers.unrealengine.com/questions/212791/how-to-get-file-list-in-a-directory.html
/**
Gets all the files in a given directory.
@param directory The full path of the directory we want to iterate over.
@param fullpath Whether the returned list should be the full file paths or just the filenames.
@param onlyFilesStartingWith Will only return filenames starting with this string. Also applies onlyFilesEndingWith if specified.
@param onlyFilesEndingWith Will only return filenames ending with this string (it looks at the extension as well!). Also applies onlyFilesStartingWith if specified.
@return A list of files (including the extension).
*/
TArray<FString> FCognitiveEditorTools::GetAllFilesInDirectory(const FString directory, const bool fullPath, const FString onlyFilesStartingWith, const FString onlyFilesWithExtension, const FString ignoreExtension, bool skipsubdirectory) const
{
	// Get all files in directory
	TArray<FString> directoriesToSkip;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, directoriesToSkip, directoriesToSkip, false);
	//PlatformFile.IterateDirectoryStat(*directory, Visitor);
	Visitor.Visit(*directory, true);
	TArray<FString> files;

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		if (skipsubdirectory)
		{
			//check if subdirectory
			FString basicPath = TimestampIt.Key();
			basicPath.RemoveFromStart(directory + "/");
			if (basicPath.Contains("\"") || basicPath.Contains("/"))
			{
				continue;
			}
		}

		const FString filePath = TimestampIt.Key();
		const FString fileName = FPaths::GetCleanFilename(filePath);
		bool shouldAddFile = true;

		// Check if filename starts with required characters
		if (!onlyFilesStartingWith.IsEmpty())
		{
			const FString left = fileName.Left(onlyFilesStartingWith.Len());

			if (!(fileName.Left(onlyFilesStartingWith.Len()).Equals(onlyFilesStartingWith)))
				shouldAddFile = false;
		}

		// Check if file extension is required characters
		if (!onlyFilesWithExtension.IsEmpty())
		{
			if (!(FPaths::GetExtension(fileName, false).Equals(onlyFilesWithExtension, ESearchCase::IgnoreCase)))
				shouldAddFile = false;
		}

		if (!ignoreExtension.IsEmpty())
		{
			if ((FPaths::GetExtension(fileName, false).Equals(ignoreExtension, ESearchCase::IgnoreCase)))
				shouldAddFile = false;
		}

		// Add full path to results
		if (shouldAddFile)
		{
			files.Add(fullPath ? filePath : fileName);
		}
	}

	return files;
}

bool FCognitiveEditorTools::HasFoundBlender() const
{
	if (!HasDeveloperKey()) { return false; }
	return FCognitiveEditorTools::GetBlenderPath().ToString().Contains("blender.exe");
}

bool FCognitiveEditorTools::HasFoundBlenderAndHasSelection() const
{
	if (!HasDeveloperKey()) { return false; }
	return FCognitiveEditorTools::GetBlenderPath().ToString().Contains("blender.exe") && GEditor->GetSelectedActorCount() > 0;
}

//checks for json and no bmps files in export directory
bool FCognitiveEditorTools::HasConvertedFilesInDirectory() const
{
	if (!HasSetExportDirectory()) { return false; }

	FString filesStartingWith = TEXT("");
	FString bmpextension = TEXT("bmp");
	FString jsonextension = TEXT("json");

	TArray<FString> imagesInDirectory = GetAllFilesInDirectory(BaseExportDirectory, true, filesStartingWith, bmpextension, filesStartingWith, false);
	TArray<FString> jsonInDirectory = GetAllFilesInDirectory(BaseExportDirectory, true, filesStartingWith, jsonextension, filesStartingWith, false);
	if (imagesInDirectory.Num() > 0) { return false; }
	if (jsonInDirectory.Num() == 0) { return false; }

	return true;
}

bool FCognitiveEditorTools::CanUploadSceneFiles() const
{
	return HasConvertedFilesInDirectory() && HasDeveloperKey();
}

bool FCognitiveEditorTools::LoginAndCustonerIdAndBlenderExportDir() const
{
	return HasDeveloperKey() && HasFoundBlenderAndExportDir();
}

bool FCognitiveEditorTools::HasFoundBlenderAndExportDir() const
{
	if (GetBaseExportDirectory().Len() == 0) { return false; }
	return FCognitiveEditorTools::GetBlenderPath().ToString().Contains("blender.exe");
}

bool FCognitiveEditorTools::HasFoundBlenderAndDynamicExportDir() const
{
	if (GetBaseExportDirectory().Len() == 0) { return false; }
	return FCognitiveEditorTools::GetBlenderPath().ToString().Contains("blender.exe");
}

bool FCognitiveEditorTools::HasFoundBlenderDynamicExportDirSelection() const
{
	if (GetBaseExportDirectory().Len() == 0) { return false; }
	if (GEditor->GetSelectedActorCount() == 0) { return false; }
	return FCognitiveEditorTools::GetBlenderPath().ToString().Contains("blender.exe");
}

bool FCognitiveEditorTools::CurrentSceneHasSceneId() const
{
	if (!HasDeveloperKey()) { return false; }
	TSharedPtr<FEditorSceneData> currentscene = GetCurrentSceneData();
	if (!currentscene.IsValid())
	{
		return false;
	}
	if (currentscene->Id.Len() > 0)
	{
		return true;
	}
	return false;
}

bool FCognitiveEditorTools::HasSetExportDirectory() const
{
	if (!HasDeveloperKey()) { return false; }
	return FCognitiveEditorTools::GetBaseExportDirectory().Len() != 0;
}


FText FCognitiveEditorTools::GetDynamicsOnSceneExplorerTooltip() const
{
	if (!HasDeveloperKey())
	{
		return FText::FromString("Must log in to get Dynamic Objects List from SceneExplorer");
	}
	auto scene = GetCurrentSceneData();
	if (!scene.IsValid())
	{
		return FText::FromString("Scene does not have valid data. Must export your scene before uploading dynamics!");
	}
	return FText::FromString("Something went wrong!");
}

EVisibility FCognitiveEditorTools::ConfigFileChangedVisibility() const
{
	if (ConfigFileHasChanged)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

FText FCognitiveEditorTools::SendDynamicsToSceneExplorerTooltip() const
{
	if (HasDeveloperKey())
	{
		return FText::FromString("");
	}
	return FText::FromString("Must log in to send Dynamic Objects List to SceneExplorer");
}

bool FCognitiveEditorTools::HasSetDynamicExportDirectory() const
{
	if (!HasDeveloperKey()) { return false; }
	if (GetBaseExportDirectory().Len() == 0) { return false; }
	return true;
}

bool FCognitiveEditorTools::HasExportedAnyDynamicMeshes() const
{
	if (GetBaseExportDirectory().Len() == 0) { return false; }

	FString filesStartingWith = TEXT("");
	FString pngextension = TEXT("png");
	TArray<FString> filesInDirectory = GetAllFilesInDirectory(BaseExportDirectory + "/dynamics", true, filesStartingWith, filesStartingWith, pngextension, false);

	TArray<FString> imagesInDirectory = GetAllFilesInDirectory(BaseExportDirectory + "/dynamics", true, filesStartingWith, pngextension, filesStartingWith, false);

	//DynamicUploadFiles.Empty();
	for (int32 i = 0; i < filesInDirectory.Num(); i++)
	{
		return true;
		//DynamicUploadFiles.Add(MakeShareable(new FString(filesInDirectory[i])));
	}
	for (int32 i = 0; i < imagesInDirectory.Num(); i++)
	{
		//DynamicUploadFiles.Add(MakeShareable(new FString(imagesInDirectory[i])));
	}

	return false;
}

bool FCognitiveEditorTools::HasSetDynamicExportDirectoryHasSceneId() const
{
	if (!HasDeveloperKey()) { return false; }
	auto scenedata = GetCurrentSceneData();
	if (!scenedata.IsValid()) { return false; }
	if (GetBaseExportDirectory().Len() == 0) { return false; }
	return true;
}

bool FCognitiveEditorTools::HasFoundBlenderHasSelection() const
{
	if (GEditor->GetSelectedActorCount() == 0) { return false; }
	return HasFoundBlender();
}

FText FCognitiveEditorTools::GetBlenderPath() const
{
	return FText::FromString(BlenderPath);
}

int32 FCognitiveEditorTools::CountDynamicObjectsInScene() const
{
	//loop thorugh all dynamics in the scene
	TArray<UDynamicObject*> dynamics;

	//get all the dynamic objects in the scene
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		//AStaticMeshActor *Mesh = *ActorItr;

		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		dynamics.Add(dynamic);
	}

	return dynamics.Num();
}

FText FCognitiveEditorTools::DisplayDynamicObjectsCountInScene() const
{
	return DynamicCountInScene;
}

FText FCognitiveEditorTools::DisplayDynamicObjectsCountOnWeb() const
{
	FString outstring = "Found " + FString::FromInt(SceneExplorerDynamics.Num()) + " Dynamic Objects on SceneExplorer";
	return FText::FromString(outstring);
}

FReply FCognitiveEditorTools::RefreshDisplayDynamicObjectsCountInScene()
{
	DynamicCountInScene = FText::FromString("Found " + FString::FromInt(CountDynamicObjectsInScene()) + " Dynamic Objects in scene");
	DuplicateDyanmicObjectVisibility = EVisibility::Hidden;
	//SceneDynamicObjectList->RefreshList();

	GLog->Log("FCognitiveEditorTools::RefreshDisplayDynamicObjectsCountInScene");

	SceneDynamics.Empty();
	//get all the dynamic objects in the scene
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		//AStaticMeshActor *Mesh = *ActorItr;

		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		SceneDynamics.Add(MakeShareable(new FDynamicData(dynamic->GetOwner()->GetName(), dynamic->MeshName, dynamic->CustomId)));
		//dynamics.Add(dynamic);
	}

	if (DuplicateDynamicIdsInScene())
	{
		DuplicateDyanmicObjectVisibility = EVisibility::Visible;
	}
	else
	{
		DuplicateDyanmicObjectVisibility = EVisibility::Collapsed;
	}

	return FReply::Handled();
}

EVisibility FCognitiveEditorTools::GetDuplicateDyanmicObjectVisibility() const
{
	return DuplicateDyanmicObjectVisibility;
}

FText FCognitiveEditorTools::GetUploadDynamicsToSceneText() const
{
	return UploadDynamicsToSceneText;
}

bool FCognitiveEditorTools::DuplicateDynamicIdsInScene() const
{
	//loop thorugh all dynamics in the scene
	TArray<UDynamicObject*> dynamics;

	//make a list of all the used objectids
	TArray<FDynamicObjectId> usedIds;

	//get all the dynamic objects in the scene
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		//AStaticMeshActor *Mesh = *ActorItr;

		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		dynamics.Add(dynamic);
	}

	int32 currentUniqueId = 1;
	int32 changedDynamics = 0;

	//unassigned or invalid numbers
	TArray<UDynamicObject*> UnassignedDynamics;

	//try to put all ids back where they were
	for (auto& dynamic : dynamics)
	{
		//id dynamic custom id is not in usedids - add it

		FString findId = dynamic->CustomId;

		FDynamicObjectId* FoundId = usedIds.FindByPredicate([findId](const FDynamicObjectId& InItem)
		{
			return InItem.Id == findId;
		});

		if (FoundId == NULL && dynamic->CustomId != "")
		{
			usedIds.Add(FDynamicObjectId(dynamic->CustomId, dynamic->MeshName));
		}
		else
		{
			//assign a new and unused id
			UnassignedDynamics.Add(dynamic);
			break;
		}
	}

	if (UnassignedDynamics.Num() > 0)
	{
		return true;
	}
	return false;
}


void FCognitiveEditorTools::OnApplicationKeyChanged(const FText& Text)
{
	ApplicationKey = Text.ToString();
	//FAnalyticsCognitiveVR::GetCognitiveVRProvider()->APIKey = APIKey;
	//FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "Analytics", "ApiKey", false);
}

FText FCognitiveEditorTools::GetApplicationKey() const
{
	return FText::FromString(ApplicationKey);
}

FText FCognitiveEditorTools::GetDeveloperKey() const
{
	return FText::FromString(FAnalyticsCognitiveVR::Get().DeveloperKey);
}

FText FCognitiveEditorTools::GetAttributionKey() const
{
	return FText::FromString(AttributionKey);
}

void FCognitiveEditorTools::OnDeveloperKeyChanged(const FText& Text)
{
	FAnalyticsCognitiveVR::Get().DeveloperKey = Text.ToString();
}

void FCognitiveEditorTools::OnAttributionKeyChanged(const FText& Text)
{
	AttributionKey = Text.ToString();
}

void FCognitiveEditorTools::OnBlenderPathChanged(const FText& Text)
{
	BlenderPath = Text.ToString();
}
void FCognitiveEditorTools::OnExportPathChanged(const FText& Text)
{

	BaseExportDirectory = Text.ToString();
}

FText FCognitiveEditorTools::UploadSceneNameFiles() const
{
	auto currentscenedata = GetCurrentSceneData();
	if (!currentscenedata.IsValid())
	{
		UWorld* myworld = GWorld->GetWorld();

		FString currentSceneName = myworld->GetMapName();
		currentSceneName.RemoveFromStart(myworld->StreamingLevelsPrefix);

		return FText::FromString("Upload Files for " + currentSceneName);
	}
	FString outstring = "Upload Files for " + currentscenedata->Name;

	return FText::FromString(outstring);
}

FText FCognitiveEditorTools::OpenSceneNameInBrowser() const
{
	auto currentscenedata = GetCurrentSceneData();
	if (!currentscenedata.IsValid())
	{
		UWorld* myworld = GWorld->GetWorld();

		FString currentSceneName = myworld->GetMapName();
		currentSceneName.RemoveFromStart(myworld->StreamingLevelsPrefix);

		return FText::FromString("Open" + currentSceneName + " in Browser...");
	}
	FString outstring = "Open " + currentscenedata->Name + " in Browser...";

	return FText::FromString(outstring);
}

FText FCognitiveEditorTools::GetDynamicObjectUploadText() const
{
	auto data = GetCurrentSceneData();
	if (!data.IsValid())
	{
		return FText::FromString("No Scene Data found - Have you used Cognitive Scene Setup to export this scene?");
	}

	//get selected dynamic data
	//for each unique mesh name

	return FText::FromString("Upload " + FString::FromInt(SubDirectoryNames.Num()) + " Dynamic Object Meshes to " + data->Name + " Version " + FString::FromInt(data->VersionNumber));
}

FText FCognitiveEditorTools::GetDynamicsFromManifest() const
{
	return FText::FromString("DYNAMICS");
}

FReply FCognitiveEditorTools::RefreshDynamicSubDirectory()
{
	FindAllSubDirectoryNames();
	//SubDirectoryListWidget->RefreshList();
	return FReply::Handled();
}

FReply FCognitiveEditorTools::ButtonCurrentSceneVersionRequest()
{
	CurrentSceneVersionRequest();

	return FReply::Handled();
}

void FCognitiveEditorTools::CurrentSceneVersionRequest()
{
	TSharedPtr<FEditorSceneData> scenedata = GetCurrentSceneData();

	if (scenedata.IsValid())
	{
		SceneVersionRequest(*scenedata);
	}
}

FReply FCognitiveEditorTools::OpenURL(FString url)
{
	FPlatformProcess::LaunchURL(*url, nullptr, nullptr);
	return FReply::Handled();
}

FReply FCognitiveEditorTools::OpenSceneInBrowser(FString sceneid)
{
	FString url = SceneExplorerOpen(sceneid);

	FPlatformProcess::LaunchURL(*url, nullptr, nullptr);

	return FReply::Handled();
}

FReply FCognitiveEditorTools::OpenCurrentSceneInBrowser()
{
	TSharedPtr<FEditorSceneData> scenedata = GetCurrentSceneData();

	if (!scenedata.IsValid())
	{
		return FReply::Handled();
	}

	FString url = SceneExplorerOpen(scenedata->Id);

	return FReply::Handled();
}

void FCognitiveEditorTools::ReadSceneDataFromFile()
{
	SceneData.Empty();

	TArray<FString>scenstrings;
	FString TestSyncFile = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEngine.ini"));
	GConfig->GetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), scenstrings, TestSyncFile);

	for (int32 i = 0; i < scenstrings.Num(); i++)
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
			GLog->Log("FCognitiveTools::RefreshSceneData failed to parse " + scenstrings[i]);
			continue;
		}

		SceneData.Add(MakeShareable(new FEditorSceneData(Array[0], Array[1], FCString::Atoi(*Array[2]), FCString::Atoi(*Array[3]))));
	}

	GLog->Log("FCognitiveTools::RefreshSceneData found this many scenes: " + FString::FromInt(SceneData.Num()));
	//ConfigFileHasChanged = true;
}

void FCognitiveEditorTools::SceneVersionRequest(FEditorSceneData data)
{
	if (!HasDeveloperKey())
	{
		GLog->Log("FCognitiveTools::SceneVersionRequest no developer key set!");
		//auto httprequest = RequestAuthTokenCallback();

		return;
	}

	auto HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->SetURL(GetSceneVersion(data.Id));

	HttpRequest->SetHeader("X-HTTP-Method-Override", TEXT("GET"));
	//HttpRequest->SetHeader("Authorization", TEXT("Data " + FAnalyticsCognitiveVR::Get().EditorAuthToken));
	FString AuthValue = "APIKEY:DEVELOPER " + FAnalyticsCognitiveVR::Get().DeveloperKey;
	HttpRequest->SetHeader("Authorization", AuthValue);
	HttpRequest->SetHeader("Content-Type", "application/json");

	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FCognitiveEditorTools::SceneVersionResponse);
	HttpRequest->ProcessRequest();
}

void FCognitiveEditorTools::SceneVersionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
		GLog->Log("FCognitiveEditorTools::SceneVersionResponse response code " + FString::FromInt(Response->GetResponseCode()));
	else
	{
		GLog->Log("FCognitiveEditorTools::SceneVersionResponse failed to connect");
		WizardUploading = false;
		WizardUploadError = "FCognitiveEditorTools::SceneVersionResponse failed to connect";
		WizardUploadResponseCode = 0;
		return;
	}

	WizardUploadResponseCode = Response->GetResponseCode();

	if (WizardUploadResponseCode >= 500)
	{
		//internal server error
		GLog->Log("FCognitiveTools::SceneVersionResponse 500-ish internal server error");
		WizardUploadError = "FCognitiveEditorTools::SceneVersionResponse response code " + FString::FromInt(Response->GetResponseCode());
		return;
	}
	if (WizardUploadResponseCode >= 400)
	{
		WizardUploading = false;
		if (WizardUploadResponseCode == 401)
		{
			//not authorized or scene id does not exist
			GLog->Log("FCognitiveTools::SceneVersionResponse not authorized or scene doesn't exist!");
			WizardUploadError = "FCognitiveEditorTools::SceneVersionResponse response code " + FString::FromInt(Response->GetResponseCode()) + "\nThe Developer Key: " + FAnalyticsCognitiveVR::Get().DeveloperKey + " does not have access to the scene";
			return;
		}
		else
		{
			//maybe no scene?
			GLog->Log("FCognitiveTools::SceneVersionResponse some error. Maybe no scene?");
			WizardUploadError = "FCognitiveEditorTools::SceneVersionResponse response code " + FString::FromInt(Response->GetResponseCode());
			return;
		}
	}
	WizardUploadError = "";

	//parse response content to json

	TSharedPtr<FJsonObject> JsonSceneSettings;

	TSharedRef<TJsonReader<>>Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (FJsonSerializer::Deserialize(Reader, JsonSceneSettings))
	{
		//get the latest version of the scene
		int32 versionNumber = 0;
		int32 versionId = 0;
		TArray<TSharedPtr<FJsonValue>> versions = JsonSceneSettings->GetArrayField("versions");
		for (int32 i = 0; i < versions.Num(); i++) {

			int32 tempversion = versions[i]->AsObject()->GetNumberField("versionnumber");
			if (tempversion > versionNumber)
			{
				versionNumber = tempversion;
				versionId = versions[i]->AsObject()->GetNumberField("id");
			}
		}
		if (versionNumber + versionId == 0)
		{
			GLog->Log("FCognitiveTools::SceneVersionResponse couldn't find a latest version in SceneVersion data");
			WizardUploadError = "FCognitiveTools::SceneVersionResponse couldn't find a latest version in SceneVersion data";
			return;
		}

		//check that there is scene data in ini
		TSharedPtr<FEditorSceneData> currentSceneData = GetCurrentSceneData();
		if (!currentSceneData.IsValid())
		{
			GLog->Log("FCognitiveTools::SceneVersionResponse can't find current scene data in ini files");
			WizardUploadError = "FCognitiveTools::SceneVersionResponse can't find current scene data in ini files";
			return;
		}

		//get array of scene data
		TArray<FString> iniscenedata;

		FString TestSyncFile = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEngine.ini"));
		GConfig->GetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), iniscenedata, TestSyncFile);

		//GLog->Log("found this many scene datas in ini " + FString::FromInt(iniscenedata.Num()));
		//GLog->Log("looking for scene " + currentSceneData->Name);

		int32 lastVersionNumber = 0;

		//update current scene
		for (int32 i = 0; i < iniscenedata.Num(); i++)
		{
			//GLog->Log("looking at data " + iniscenedata[i]);

			TArray<FString> entryarray;
			iniscenedata[i].ParseIntoArray(entryarray, TEXT(","), true);

			if (entryarray[0] == currentSceneData->Name && versionNumber > lastVersionNumber)
			{
				lastVersionNumber = versionNumber;
				iniscenedata[i] = entryarray[0] + "," + entryarray[1] + "," + FString::FromInt(versionNumber) + "," + FString::FromInt(versionId);
				//GLog->Log("FCognitiveToolsCustomization::SceneVersionResponse overwriting scene data to append versionnumber and versionid");
				//GLog->Log(iniscenedata[i]);
			}
			else
			{
				//GLog->Log("found scene " + entryarray[0]);
			}
		}

		GLog->Log("FCognitiveTools::SceneVersionResponse successful. Write scene data to config file");

		//set array to config
		GConfig->RemoveKey(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), TestSyncFile);
		GConfig->SetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), iniscenedata, TestSyncFile);

		//FConfigCacheIni::LoadGlobalIniFile(GEngineIni, TEXT("Engine"));

		//GConfig->RemoveKey(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), GEngineIni);
		//GConfig->SetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), iniscenedata, GEngineIni);


		GConfig->Flush(false, TestSyncFile);
		//GConfig->LoadFile(TestSyncFile);
		ConfigFileHasChanged = true;
		ReadSceneDataFromFile();

		if (WizardUploading)
		{
			UploadDynamics();
		}
	}
	else
	{
		GLog->Log("FCognitiveToolsCustomization::SceneVersionResponse failed to parse json response");
		if (WizardUploading)
		{
			WizardUploadError = "FCognitiveToolsCustomization::SceneVersionResponse failed to parse json response";
			WizardUploadResponseCode = 0;
			WizardUploading = false;
		}
	}
}

TArray<TSharedPtr<FEditorSceneData>> FCognitiveEditorTools::GetSceneData() const
{
	return SceneData;
}

ECheckBoxState FCognitiveEditorTools::HasFoundBlenderCheckbox() const
{
	return (HasFoundBlender())
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

FReply FCognitiveEditorTools::SaveAPIDeveloperKeysToFile()
{
	FString EngineIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEngine.ini"));
	FString EditorIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEditor.ini"));
	//GLog->Log("FCognitiveTools::SaveAPIDeveloperKeysToFile save: " + CustomerId);

	//GConfig->SetString(TEXT("Analytics"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
	GConfig->SetString(TEXT("Analytics"), TEXT("ApiKey"), *ApplicationKey, EngineIni);
	GConfig->SetString(TEXT("Analytics"), TEXT("AttributionKey"), *AttributionKey, EngineIni);

	GConfig->SetString(TEXT("Analytics"), TEXT("DeveloperKey"), *FAnalyticsCognitiveVR::Get().DeveloperKey, EditorIni);

	GConfig->Flush(false, GEngineIni);

	ConfigFileHasChanged = true;

	return FReply::Handled();
}

void FCognitiveEditorTools::SaveApplicationKeyToFile(FString key)
{
	FString EngineIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEngine.ini"));
	GConfig->SetString(TEXT("Analytics"), TEXT("ApiKey"), *key, EngineIni);
	GConfig->Flush(false, GEngineIni);
	ConfigFileHasChanged = true;

	ApplicationKey = key;
}

void FCognitiveEditorTools::SaveDeveloperKeyToFile(FString key)
{
	//FString EngineIni = FPaths::Combine(*(FPaths::GameDir()), TEXT("Config/DefaultEngine.ini"));
	FString EditorIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEditor.ini"));
	//GConfig->SetString(TEXT("Analytics"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
	GConfig->SetString(TEXT("Analytics"), TEXT("DeveloperKey"), *key, EditorIni);
	GConfig->Flush(false, GEngineIni);
	ConfigFileHasChanged = true;

	FAnalyticsCognitiveVR::Get().DeveloperKey = key;
}

TSharedPtr<FEditorSceneData> FCognitiveEditorTools::GetCurrentSceneData() const
{
	UWorld* myworld = GWorld->GetWorld();

	FString currentSceneName = myworld->GetMapName();
	currentSceneName.RemoveFromStart(myworld->StreamingLevelsPrefix);
	return GetSceneData(currentSceneName);
}

FString lastSceneName;
TSharedPtr<FEditorSceneData> FCognitiveEditorTools::GetSceneData(FString scenename) const
{
	for (int32 i = 0; i < SceneData.Num(); i++)
	{
		if (!SceneData[i].IsValid()) { continue; }
		if (SceneData[i]->Name == scenename)
		{
			return SceneData[i];
		}
	}
	if (lastSceneName != scenename)
	{
		GLog->Log("FCognitiveToolsCustomization::GetSceneData couldn't find SceneData for scene " + scenename);
		lastSceneName = scenename;
	}
	return NULL;
}

void FCognitiveEditorTools::SaveSceneData(FString sceneName, FString sceneKey)
{
	FString keyValue = sceneName + "," + sceneKey;

	TArray<FString> scenePairs = TArray<FString>();

	FString TestSyncFile = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEngine.ini"));
	GConfig->GetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), scenePairs, TestSyncFile);

	bool didSetKey = false;
	for (int32 i = 0; i < scenePairs.Num(); i++)
	{
		FString name;
		FString key;
		scenePairs[i].Split(TEXT(","), &name, &key);
		if (*name == sceneName)
		{
			scenePairs[i] = keyValue;
			didSetKey = true;
			GLog->Log("FCognitiveToolsCustomization::SaveSceneData - found and replace key for scene " + name + " new value " + keyValue);
			break;
		}
	}
	if (!didSetKey)
	{
		scenePairs.Add(keyValue);
		GLog->Log("FCognitiveToolsCustomization::SaveSceneData - added new scene value and key for " + sceneName);
	}

	//remove scene names that don't have keys!
	for (int32 i = scenePairs.Num() - 1; i >= 0; i--)
	{
		FString name;
		FString key;
		if (!scenePairs[i].Split(TEXT(","), &name, &key))
		{
			scenePairs.RemoveAt(i);
		}
	}

	GConfig->SetArray(TEXT("/Script/CognitiveVR.CognitiveVRSceneSettings"), TEXT("SceneData"), scenePairs, TestSyncFile);

	GConfig->Flush(false, GEngineIni);
}

void FCognitiveEditorTools::WizardExportStaticMaterials(FString directory, TArray<UStaticMeshComponent*> meshes, FString mtlFileName)
{
	TArray<FString> ExportedMaterialNames;

	float work = 0;
	FScopedSlowTask* SlowTaskPtr = NULL;
	TArray<UMaterialInterface*> allMaterials;

	if (mtlFileName == FCognitiveEditorTools::GetInstance()->GetCurrentSceneName())
	{
		SlowTaskPtr = new FScopedSlowTask(meshes.Num(), FText::FromString("Baking Scene Materials"));
		SlowTaskPtr->MakeDialog(false);
	}

	for (int32 i = 0; i < meshes.Num(); i++)
	{
		if (SlowTaskPtr != NULL)
		{
			//work += 1;
			SlowTaskPtr->EnterProgressFrame(1);
		}
		if (meshes[i] == NULL) { continue; }
		UStaticMeshComponent* TempObject = meshes[i];

		//TArray<UMaterialInterface*> mats = TempObject->GetMaterials();
		allMaterials.Append(TempObject->GetMaterials());
	}

	TArray<FString> materialContents = WizardExportMaterials(directory, ExportedMaterialNames, allMaterials);

	if (SlowTaskPtr != NULL)
	{
		delete SlowTaskPtr;
	}
}

TArray<FString> FCognitiveEditorTools::WizardExportMaterials(FString directory, TArray<FString> ExportedMaterialNames, TArray<UMaterialInterface*> mats)
{
	IMaterialBakingModule& MaterialBakingModule = FModuleManager::Get().LoadModuleChecked<IMaterialBakingModule>("MaterialBaking");
	FIntPoint resolution = FIntPoint(256, 256);
	TArray<FString> MaterialLine;

	//build a custom mtl file that includes transparency maps
	TArray<FString> CustomMTLFile;

	for (int32 j = 0; j < mats.Num(); j++)
	{
		FString line;

		if (mats[j] == NULL) { continue; }
		if (ExportedMaterialNames.Contains(mats[j]->GetName())) { GLog->Log("skip exporting duplicate material " + mats[j]->GetName()); continue; }

		//try to get the pathname of the material
		//possible that the duplicate name is caused by path/materialtexturename?
		//GLog->Log("export material path " + mats[j]->GetPathName());
		//     /Game/Geometry/Meshes/CubeMaterial.CubeMaterial

		TArray<FMeshData*> MeshSettingPtrs;
		TArray<FMaterialData*> MaterialSettingPtrs;

		ExportedMaterialNames.Add(mats[j]->GetName());

		FMaterialData MaterialSettings;
		MaterialSettings.Material = mats[j];
		MaterialSettings.PropertySizes.Add(EMaterialProperty::MP_BaseColor, resolution);
		MaterialSettings.PropertySizes.Add(EMaterialProperty::MP_Normal, resolution);
		MaterialSettings.PropertySizes.Add(EMaterialProperty::MP_Specular, resolution);

		CustomMTLFile.Add("newmtl " + mats[j]->GetPathName().Replace(TEXT("."), TEXT("_")));

		//add some extra maps if necessary
		if (mats[j]->GetBlendMode() == EBlendMode::BLEND_Opaque)
		{
			//base colour already included above
			line.Append("OPAQUE|");
		}
		else if (mats[j]->GetBlendMode() == EBlendMode::BLEND_Masked)
		{
			MaterialSettings.PropertySizes.Add(EMaterialProperty::MP_OpacityMask, resolution);
			line.Append("MASK|");
		}
		else if (mats[j]->GetBlendMode() == EBlendMode::BLEND_Translucent || mats[j]->GetBlendMode() == EBlendMode::BLEND_Additive)
		{
			MaterialSettings.PropertySizes.Add(EMaterialProperty::MP_Opacity, resolution);
			line.Append("TRANSLUCENT|");
		}
		else
		{
			line.Append("OTHER|");
		}

		MaterialSettingPtrs.Add(&MaterialSettings);

		line.Append(mats[j]->GetName() + "|");

		FMeshData meshdata;
		meshdata.TextureCoordinateBox = FBox2D(FVector2D(0.0f, 0.0f), FVector2D(1.0f, 1.0f));
		meshdata.TextureCoordinateIndex = 0;
		MeshSettingPtrs.Add(&meshdata);

		TArray<FBakeOutput> BakeOutputs;
		MaterialBakingModule.BakeMaterials(MaterialSettingPtrs, MeshSettingPtrs, BakeOutputs);

		for (FBakeOutput& output : BakeOutputs)
		{
			if (output.PropertyData.Contains(EMaterialProperty::MP_BaseColor))
			{
				//diffuse
				FString BMPFilename = directory;
				BMPFilename.RemoveFromEnd("/");
				BMPFilename += mats[j]->GetPathName();
				BMPFilename = BMPFilename.Replace(TEXT("."), TEXT("_"));
				BMPFilename += TEXT("_D.bmp");

				FFileHelper::CreateBitmap(*BMPFilename, resolution.X, resolution.Y, output.PropertyData[EMaterialProperty::MP_BaseColor].GetData());
				line.Append(BMPFilename + "|");
				CustomMTLFile.Add("\tmap_Kd " + mats[j]->GetName().Replace(TEXT("."), TEXT("_"))+"_"+ mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_D.bmp");
			}

			if (output.PropertyData.Contains(EMaterialProperty::MP_Specular))
			{
				//specular
				FString BMPFilename = directory;
				BMPFilename.RemoveFromEnd("/");
				BMPFilename += mats[j]->GetPathName();
				BMPFilename = BMPFilename.Replace(TEXT("."), TEXT("_"));
				BMPFilename += TEXT("_S.bmp");

				FFileHelper::CreateBitmap(*BMPFilename, resolution.X, resolution.Y, output.PropertyData[EMaterialProperty::MP_Specular].GetData());
				line.Append(BMPFilename + "|");
				CustomMTLFile.Add("\tmap_Ks " + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_" + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_S.bmp");
			}

			if (output.PropertyData.Contains(EMaterialProperty::MP_Normal))
			{
				//normal
				FString BMPFilename = directory;
				BMPFilename.RemoveFromEnd("/");
				BMPFilename += mats[j]->GetPathName();
				BMPFilename = BMPFilename.Replace(TEXT("."), TEXT("_"));
				BMPFilename += TEXT("_N.bmp");

				FFileHelper::CreateBitmap(*BMPFilename, resolution.X, resolution.Y, output.PropertyData[EMaterialProperty::MP_Normal].GetData());
				line.Append(BMPFilename + "|");
				CustomMTLFile.Add("\tbump " + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_" + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_N.bmp");
			}
			if (output.PropertyData.Contains(EMaterialProperty::MP_OpacityMask))
			{
				//mask
				FString BMPFilename = directory;
				BMPFilename.RemoveFromEnd("/");
				BMPFilename += mats[j]->GetPathName();
				BMPFilename = BMPFilename.Replace(TEXT("."), TEXT("_"));
				BMPFilename += TEXT("_OM.bmp");
				FFileHelper::CreateBitmap(*BMPFilename, resolution.X, resolution.Y, output.PropertyData[EMaterialProperty::MP_OpacityMask].GetData());
				line.Append(BMPFilename);
				CustomMTLFile.Add("\tmap_d " + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_" + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_OM.bmp");
			}
			if (output.PropertyData.Contains(EMaterialProperty::MP_Opacity))
			{
				//opacity
				FString BMPFilename = directory;
				BMPFilename.RemoveFromEnd("/");
				BMPFilename += mats[j]->GetPathName();
				BMPFilename = BMPFilename.Replace(TEXT("."), TEXT("_"));
				BMPFilename += TEXT("_O.bmp");
				FFileHelper::CreateBitmap(*BMPFilename, resolution.X, resolution.Y, output.PropertyData[EMaterialProperty::MP_Opacity].GetData());
				line.Append(BMPFilename);
				CustomMTLFile.Add("\tmap_d " + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_" + mats[j]->GetName().Replace(TEXT("."), TEXT("_")) + "_O.bmp");
			}
			line = line.ReplaceCharWithEscapedChar();
			MaterialLine.Add(line);
		}
		CustomMTLFile.Add("");
	}
	FString mtlPath = directory + GetCurrentSceneName()+".mtl";
	FFileHelper::SaveStringArrayToFile(CustomMTLFile, *mtlPath);
	return MaterialLine;
}

void FCognitiveEditorTools::ExportScene(TArray<AActor*> actorsToExport)
{
	UWorld* tempworld = GEditor->GetEditorWorldContext().World();

	if (!tempworld)
	{
		UE_LOG(LogTemp, Warning, TEXT("FCognitiveEditorTools::ExportScene world is null"));
		return;
	}

	GEditor->SelectNone(false, true, false);

	for (int32 i = 0; i < actorsToExport.Num(); i++)
	{
		GEditor->SelectActor((actorsToExport[i]), true, false, true);
	}

	//create directory at scene name path
	FString sceneDirectory = FCognitiveEditorTools::GetInstance()->GetCurrentSceneExportDirectory() + "/";
	FCognitiveEditorTools::VerifyOrCreateDirectory(sceneDirectory);

	//export scene
	FString ExportedSceneFile2 = FCognitiveEditorTools::GetInstance()->GetCurrentSceneExportDirectory() + "/" + FCognitiveEditorTools::GetInstance()->GetCurrentSceneName() + ".obj";
	GEditor->ExportMap(tempworld, *ExportedSceneFile2, true);

	//export materials
	TArray< UStaticMeshComponent*> sceneMeshes;

	for (auto& elem : actorsToExport)
	{
		auto TempObject = elem->GetComponentByClass(UStaticMeshComponent::StaticClass());
		if (TempObject == NULL) { continue; }
		auto staticTempObject = (UStaticMeshComponent*)TempObject;

		if (staticTempObject->GetOwner() == NULL) { continue; }

		UActorComponent* dynamic = staticTempObject->GetOwner()->GetComponentByClass(UDynamicObject::StaticClass());
		if (dynamic != NULL) { continue; }

		sceneMeshes.Add(staticTempObject);
	}

	//build materiallist.txt and export textures for transparent materials
	WizardExportStaticMaterials(sceneDirectory, sceneMeshes, FCognitiveEditorTools::GetInstance()->GetCurrentSceneName());

	//Convert scene to GLTF. run python script
	FProcHandle fph = ConvertSceneToGLTF();

	if (fph.IsValid())
	{
		FPlatformProcess::WaitForProc(fph);

		TakeScreenshot();
		//write debug.log including unreal data, scene contents, folder contents
		//should happen after blender finishes/next button is pressed

		FString ObjPath = FPaths::Combine(BaseExportDirectory, GetCurrentSceneName());
		FString escapedOutPath = ObjPath.Replace(TEXT(" "), TEXT("\" \""));
		FString fullPath = escapedOutPath + "/debug.log";
		FString fileContents = BuildDebugFileContents();
		bool success = FFileHelper::SaveStringToFile(fileContents, *fullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);


		FString settingsContents = "{\"scale\":100,\"sdkVersion\":\""+ FString(COGNITIVEVR_SDK_VERSION)+"\",\"sceneName\":\""+ GetCurrentSceneName()+"\"}";
		FString settingsFullPath = escapedOutPath + "/settings.json";
		bool success2 = FFileHelper::SaveStringToFile(settingsContents, *settingsFullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

FProcHandle FCognitiveEditorTools::ConvertSceneToGLTF()
{
	FProcHandle BlenderReduceAllWizardProc;

	FString pythonscriptpath = IPluginManager::Get().FindPlugin(TEXT("CognitiveVR"))->GetBaseDir() / TEXT("Resources") / TEXT("ConvertSceneToGLTF.py");
	const TCHAR* charPath = *pythonscriptpath;
	FString fullScriptPath = FPaths::ConvertRelativePathToFull(pythonscriptpath);
	GLog->Log("full " + fullScriptPath);


	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> ScriptList;
	if (!AssetRegistry.GetAssetsByPath(FName(*pythonscriptpath), ScriptList))
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::Reduce_Meshes_And_Textures - Could not find script at path. Canceling"));
		return BlenderReduceAllWizardProc;
	}

	FString stringurl = BlenderPath;

	if (BlenderPath.Len() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::Reduce_Meshes_And_Textures - No path set for Blender.exe. Canceling"));
		return BlenderReduceAllWizardProc;
	}

	UWorld* tempworld = GEditor->GetEditorWorldContext().World();
	if (!tempworld)
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::Reduce_Meshes_And_Textures - World is null. canceling"));
		return BlenderReduceAllWizardProc;
	}

	FString SceneName = tempworld->GetMapName();
	FString ObjPath = FPaths::Combine(BaseExportDirectory, GetCurrentSceneName());

	if (ObjPath.Len() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FCognitiveEditorTools::Reduce_Meshes_And_Textures No know export directory. Canceling"));
		return BlenderReduceAllWizardProc;
	}

	FString escapedPythonPath = fullScriptPath.Replace(TEXT(" "), TEXT("\" \""));
	FString escapedOutPath = ObjPath.Replace(TEXT(" "), TEXT("\" \""));

	//export path
	//scene/dynamic type
	//targetNames comma separated

	FString stringparams = " -P " + escapedPythonPath + " " + escapedOutPath + " scene " + SceneName;

	FString stringParamSlashed = stringparams.Replace(TEXT("\\"), TEXT("/"));

	UE_LOG(LogTemp, Log, TEXT("FCognitiveEditorTools::ConvertSceneToGLTF Params: %s"), *stringParamSlashed);

	const TCHAR* params = *stringParamSlashed;
	int32 priorityMod = 0;
	BlenderReduceAllWizardProc = FPlatformProcess::CreateProc(*BlenderPath, params, false, false, false, NULL, priorityMod, 0, nullptr);

	return BlenderReduceAllWizardProc;
}

void FCognitiveEditorTools::WizardUpload()
{
	WizardUploading = true;
	WizardUploadError = "";
	WizardUploadResponseCode = 0;
	UploadScene();
}


FText FCognitiveEditorTools::GetBaseExportDirectoryDisplay() const
{
	return FText::FromString(BaseExportDirectory);
}

//c:/users/me/desktop/export/scenename/
FText FCognitiveEditorTools::GetSceneExportDirectoryDisplay(FString scenename) const
{
	return FText::FromString(FPaths::Combine(BaseExportDirectory, scenename));
}

//c:/users/me/desktop/export/scenename/
FText FCognitiveEditorTools::GetCurrentSceneExportDirectoryDisplay() const
{
	return FText::FromString(FPaths::Combine(BaseExportDirectory, FCognitiveEditorTools::GetInstance()->GetCurrentSceneName()));
}

//c:/users/me/desktop/export/dynamics/
FText FCognitiveEditorTools::GetDynamicsExportDirectoryDisplay() const
{
	return FText::FromString(FPaths::Combine(BaseExportDirectory, "dynamics"));
}

EVisibility FCognitiveEditorTools::BlenderValidVisibility() const
{
	if (HasFoundBlender())
		return EVisibility::Visible;
	return EVisibility::Collapsed;
}

EVisibility FCognitiveEditorTools::BlenderInvalidVisibility() const
{
	if (HasFoundBlender())
		return EVisibility::Collapsed;
	return EVisibility::Visible;
}

FString FCognitiveEditorTools::BuildDebugFileContents() const
{
	FString outputString;

	outputString += "*****************************\n";
	outputString += "***********SYSTEM************\n";
	outputString += "*****************************\n";

	//unreal version
	outputString += FString("Unreal Engine Version: ") + FEngineVersion::Current().ToString();
	outputString += "\n";

	//os name
	outputString += FString("Platform Name: ") + FPlatformProperties::IniPlatformName();
	outputString += "\n";

	//date + time
	outputString += FString("System Time: ") + FDateTime::Now().ToString();
	outputString += "\n";

	outputString += "*****************************\n";
	outputString += "***********PROJECT***********\n";
	outputString += "*****************************\n";

	//sdk version
	outputString += FString("Cognitive SDK version: ") + FString(COGNITIVEVR_SDK_VERSION);
	outputString += "\n";

	//plugin folder contents
	FString pluginDir = FPaths::ProjectDir() + "/Plugins/";
	//TArray<FString> DirectoriesToSkip;
	//TArray<FString> DirectoriesToNotRecurse;
	//IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	//FLocalTimestampDirectoryVisitor Visitor(PlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, true);
	//IFileManager::Get().IterateDirectory(*pluginDir, Visitor);
	//Visitor.Visit(*pluginDir, true);
	//for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)

	if (IFileManager::Get().DirectoryExists(*(pluginDir + "Varjo")))
	{
		outputString += FString("Plugins: Varjo");
		outputString += "\n";
	}
	if (IFileManager::Get().DirectoryExists(*(pluginDir + "TobiiEyeTracking")))
	{
		outputString += FString("Plugins: TobiiEyeTracking");
		outputString += "\n";
	}
	if (IFileManager::Get().DirectoryExists(*(pluginDir + "SRanipal")))
	{
		outputString += FString("Plugins: SRanipal");
		outputString += "\n";
	}
	if (IFileManager::Get().DirectoryExists(*(pluginDir + "PicoMobile")))
	{
		outputString += FString("Plugins: PicoMobile");
		outputString += "\n";
	}

	//project directory
	outputString += FString("Project Name: ") + FPaths::ProjectDir();
	outputString += "\n";

	//gateway
	FString gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
	outputString += "Gateway: " + gateway;
	outputString += "\n";

	//api key ****
	FString tempApiKey = FCognitiveEditorTools::GetInstance()->GetApplicationKey().ToString();
	outputString += FString("API Key: ****") + tempApiKey.RightChop(tempApiKey.Len() - 4);
	outputString += "\n";

	//dev key ****
	FString tempDevKey = FAnalyticsCognitiveVR::Get().DeveloperKey;
	outputString += FString("DEV Key: ****") + tempDevKey.RightChop(tempDevKey.Len() - 4);
	outputString += "\n";

	//config settings (batch sizes, etc)
	FString ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "CustomEventBatchSize", false);
	outputString += "Event Snapshot Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "CustomEventExtremeLimit", false);
	outputString += "Event Extreme Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "CustomEventMinTimer", false);
	outputString += "Event Minimum Send Timer: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "CustomEventAutoTimer", false);
	outputString += "Event Auto Send Timer: " + ValueReceived;
	outputString += "\n";

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorBatchSize", false);
	outputString += "Sensor Snapshot Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorExtremeLimit", false);
	outputString += "Sensor Extreme Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorMinTimer", false);
	outputString += "Sensor Minimum Send Timer: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorAutoTimer", false);
	outputString += "Sensor Auto Send Timer: " + ValueReceived;
	outputString += "\n";

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "DynamicBatchSize", false);
	outputString += "Dynamic Snapshot Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "DynamicExtremeLimit", false);
	outputString += "Dynamic Extreme Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "DynamicMinTimer", false);
	outputString += "Dynamic Minimum Send Timer: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "DynamicAutoTimer", false);
	outputString += "Dynamic Auto Send Timer: " + ValueReceived;
	outputString += "\n";

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "GazeBatchSize", false);
	outputString += "Gaze Snapshot Batch Size: " + ValueReceived;
	outputString += "\n";

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationBatchSize", false);
	outputString += "Fixation Snapshot Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationExtremeLimit", false);
	outputString += "Fixation Extreme Batch Size: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationMinTimer", false);
	outputString += "Fixation Minimum Send Timer: " + ValueReceived;
	outputString += "\n";
	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationAutoTimer", false);
	outputString += "Fixation Auto Send Timer: " + ValueReceived;
	outputString += "\n";

	outputString += "Scene Settings:";
	outputString += "\n";

	TSharedPtr<FEditorSceneData> currentLevelSceneData;
	//scene settings (name, id, version)
	for (auto& elem : FCognitiveEditorTools::GetInstance()->SceneData)
	{
		outputString += FString("    Scene Name: ") + elem->Name;
		outputString += "\n";
		outputString += FString("        Scene Id: ") + elem->Id;
		outputString += "\n";
		outputString += FString("        Version Number: ") + FString::FromInt(elem->VersionNumber);
		outputString += "\n";
		outputString += FString("        Version Id: ") + FString::FromInt(elem->VersionId);
		outputString += "\n";

		if (elem->Name == UGameplayStatics::GetCurrentLevelName(GWorld))
		{
			currentLevelSceneData = elem;
		}
	}

	outputString += "*****************************\n";
	outputString += "********CURRENT SCENE********\n";
	outputString += "*****************************\n";

	//current scene info
	if (currentLevelSceneData.IsValid())
	{
		outputString += FString("Scene Name: ") + currentLevelSceneData->Name;
		outputString += "\n";
		outputString += FString("Scene Id: ") + currentLevelSceneData->Id;
		outputString += "\n";
		outputString += FString("Version Number: ") + FString::FromInt(currentLevelSceneData->VersionNumber);
		outputString += "\n";
		outputString += FString("Version Id: ") + FString::FromInt(currentLevelSceneData->VersionId);
		outputString += "\n";
	}

	outputString += "*****************************\n";
	outputString += "****CURRENT SCENE OBJECTS****\n";
	outputString += "*****************************\n";

	//find dynamics in scene
	TArray<UDynamicObject*> foundDynamics;
	TArray<AActor*> foundActors;
	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		foundDynamics.Add(dynamic);
		foundActors.Add(*ActorItr);
	}

	//dynamics in scene count
	outputString += FString("Dynamic Object Count: ") + FString::FromInt(foundDynamics.Num());
	outputString += "\n";

	for (int32 i = 0; i < foundDynamics.Num(); i++)
	{
		outputString += FString("    ") + foundActors[i]->GetFName().ToString();
		outputString += "\n";
		outputString += FString("        Mesh Name: ") + foundDynamics[i]->MeshName;
		outputString += "\n";
		if (foundDynamics[i]->IdSourceType == EIdSourceType::CustomId)
		{
			outputString += FString("        Id: ") + foundDynamics[i]->CustomId;
			outputString += "\n";
		}
	}

	outputString += "*****************************\n";
	outputString += "********EXPORT FOLDER********\n";
	outputString += "*****************************\n";

	//tree list folders in export directory + contents
	FString dir = FCognitiveEditorTools::GetInstance()->GetBaseExportDirectory();

	FCognitiveEditorTools::GetInstance()->AppendDirectoryContents(dir, 0, outputString);

	return outputString;
}

void FCognitiveEditorTools::AppendDirectoryContents(FString FullPath, int32 depth, FString& outputString)
{
	FString searchPath = FullPath + "/*";

	//add all files to output
	TArray<FString> files;
	IFileManager::Get().FindFiles(files, *searchPath, true, false);

	for (auto& elem : files)
	{
		for (int32 i = 0; i < depth; i++)
			outputString += "    ";

		//file size
		int32 bytecount = IFileManager::Get().FileSize(*(FullPath + "/" + elem));
		float mbcount = bytecount * 0.000001;
		TArray<FString> parseArray;
		FString sizeString = FString::SanitizeFloat(mbcount, 2);
		sizeString.ParseIntoArray(parseArray, TEXT("."));
		FString finalString = parseArray[0] + "." + parseArray[1].Left(2) + "mb";

		outputString += elem + " (" + finalString + ")";
		outputString += "\n";
	}

	//call this again for each directory
	TArray<FString> directories;
	IFileManager::Get().FindFiles(directories, *searchPath, false, true);

	for (auto& elem : directories)
	{
		for (int32 i = 0; i < depth; i++)
			outputString += "    ";

		//directory size returns -1?
		//FFileStatData dirStats = IFileManager::Get().GetStatData(*(FullPath + "/" + elem));

		outputString += "/" + elem;
		outputString += "\n";
		AppendDirectoryContents(FullPath + "/" + elem, depth + 1, outputString);
	}
}

#undef LOCTEXT_NAMESPACE
