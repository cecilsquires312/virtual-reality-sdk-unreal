#pragma once

#include "CognitiveVREditorPrivatePCH.h"
#include "CognitiveEditorTools.h"
#include "CognitiveEditorData.h"
#include "CognitiveVRSettings.h"
#include "IDetailCustomization.h"
#include "PropertyEditing.h"
//#include "DetailCustomizationsPrivatePCH.h"
#include "SDynamicObjectListWidget.h"
#include "PropertyCustomizationHelpers.h"
#include "Json.h"
#include "SCheckBox.h"
#include "STableRow.h"
#include "STextComboBox.h"
#include "SListView.h"
#include "SThrobber.h"
#include "Runtime/SlateCore/Public/Layout/Visibility.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

class FCognitiveTools;

class SSceneSetupWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSceneSetupWidget){}
	SLATE_ARGUMENT(TArray<TSharedPtr<FDynamicData>>, Items)
		SLATE_ARGUMENT(FSlateBrush*,ScreenshotTexture)
	//SLATE_ARGUMENT(FCognitiveEditorTools*, CognitiveEditorTools)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);


	FString DisplayAPIKey;
	FText GetDisplayAPIKey() const;
	void OnAPIKeyChanged(const FText& Text);

	FString DisplayDeveloperKey;
	FText GetDisplayDeveloperKey() const;
	void OnDeveloperKeyChanged(const FText& Text);



	TArray<TSharedPtr<FDynamicData>> GetSceneDynamics();

	/* Adds a new textbox with the string to the list */
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FDynamicData> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/* The list of strings */
	TArray<TSharedPtr<FDynamicData>> Items;
	//FCognitiveTools* CognitiveTools;

	int32 CurrentPage;
	bool SceneWasExported = false;

	
	//FString APIKey;
	//FText GetAPIKey() const;

	//FText GetDeveloperKey() const;

	

	EVisibility IsIntroVisible() const;
	EVisibility IsKeysVisible() const;
	EVisibility IsBlenderVisible() const;
	EVisibility IsExplainDynamicsVisible() const;
	EVisibility IsExplainSceneVisible() const;
	EVisibility IsDynamicsVisible() const;
	EVisibility IsExportVisible() const;
	EVisibility IsUploadVisible() const;
	EVisibility IsCompleteVisible() const;
	EVisibility IsUploadComplete() const;
	FReply NextPage();
	EVisibility NextButtonVisibility() const;
	bool NextButtonEnabled() const;
	FText NextButtonText() const;
	EVisibility BackButtonVisibility() const;
	FReply LastPage();
	EVisibility UploadErrorVisibility() const;
	FText UploadErrorText() const;

	EVisibility IsNewSceneUpload() const;
	EVisibility IsSceneVersionUpload() const;
	EVisibility IsIntroNewVersionVisible() const;

	//FReply Export_Selected();
	//FReply Export_All();

	EVisibility ARButtonVisibility() const;
	FReply ARSkipExport();

	TArray<TSharedPtr<FString>> NEWEXPORTFILES;
	void GetScreenshotBrush();
	FSlateBrush* ScreenshotTexture;
	const FSlateBrush* GetScreenshotBrushTexture() const;

	const FSlateBrush* GetDynamicsGreyTexture() const;
	FSlateBrush* DynamicsGreyTexture;
	const FSlateBrush* GetDynamicsBlueTexture() const;
	FSlateBrush* DynamicsBlueTexture;
	const FSlateBrush* GetSceneGreyTexture() const;
	FSlateBrush* SceneGreyTexture;
	const FSlateBrush* GetSceneBlueTexture() const;
	FSlateBrush* SceneBlueTexture;

	TSharedRef<ITableRow> OnGenerateSceneExportFileRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	EVisibility DisplayWizardThrobber() const;
	

	TSharedPtr<SDynamicObjectListWidget> SceneDynamicObjectList;
	TSharedPtr<SImage> ScreenshotImage;

	FText DisplayDynamicObjectsCountInScene() const;
	FReply RefreshDisplayDynamicObjectsCountInScene();
	EVisibility GetDuplicateDyanmicObjectVisibility() const;

	FReply SetUniqueDynamicIds();
	//bool DuplicateDynamicIdsInScene() const;

	int32 CountDynamicObjectsInScene() const;

	FText DynamicCountInScene;

	/* The actual UI list */
	TSharedPtr< SListView< TSharedPtr<FDynamicData> > > ListViewWidget;

	FReply SelectDynamic(TSharedPtr<FDynamicData> data);

	void RefreshList();

	FReply TakeScreenshot();

	int32 ScreenshotWidth = 256;
	int32 ScreenshotHeight = 256;

	FOptionalSize GetScreenshotWidth() const;
	FOptionalSize GetScreenshotHeight() const;


	FReply EvaluateExport();
	bool NoExportGameplayMeshes;
	ECheckBoxState GetNoExportGameplayMeshCheckbox() const;
	void OnChangeNoExportGameplayMesh(ECheckBoxState newstate)
	{
		if (newstate == ECheckBoxState::Checked)
		{
			NoExportGameplayMeshes = true;
		}
		else
		{
			NoExportGameplayMeshes = false;
		}
	}
	bool OnlyExportSelected;
	ECheckBoxState GetOnlyExportSelectedCheckbox() const;
	void OnChangeOnlyExportSelected(ECheckBoxState newstate)
	{
		if (newstate == ECheckBoxState::Checked)
		{
			OnlyExportSelected = true;
		}
		else
		{
			OnlyExportSelected = false;
		}
	}

};