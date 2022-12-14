#pragma once

#include "CognitiveVRSettings.h"
#include "IDetailCustomization.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "Json.h"
#include "SCheckBox.h"
#include "STableRow.h"
#include "SFStringListWidget.h"
#include "CognitiveEditorTools.h"
#include "STextComboBox.h"
#include "SDynamicObjectListWidget.h"

//customization ui for cognitive settings/preferences. most of this is interface code that calls FCognitiveEditorTools logic

class UCognitiveVRSettings;

class FCognitiveSettingsCustomization : public IDetailCustomization
{
public:

	static FCognitiveSettingsCustomization* CognitiveSettingsCustomizationInstance;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance();
	UCognitiveVRSettings *Settings;
	IDetailLayoutBuilder *DetailLayoutPtr;

	TArray<TSharedPtr<FDynamicData>> GetSceneDynamics();

	TSharedRef<ITableRow> OnGenerateWorkspaceRow(TSharedPtr<FEditorSceneData> InItem, const TSharedRef<STableViewBase>& OwnerTable);


	void OnChangedExcludeMesh(const FText& InNewValue);

	void OnChangedExportMinimumSize(float InNewValue);
	void OnCommitedExportMinimumSize(float InNewValue, ETextCommit::Type CommitType);

	void OnChangedExportMaximumSize(float InNewValue);
	void OnCommitedExportMaximumSize(float InNewValue, ETextCommit::Type CommitType);

	void OnChangedExportMinPolygon(int32 InNewValue);
	void OnCommitedExportMinPolygon(int32 InNewValue, ETextCommit::Type CommitType);

	void OnChangedExportMaxPolygon(int32 InNewValue);
	void OnCommitedExportMaxPolygon(int32 InNewValue, ETextCommit::Type CommitType);

	void OnChangedExportTextureRefactor(int32 InNewValue);
	void OnCommitedExportTextureRefactor(int32 InNewValue, ETextCommit::Type CommitType);

	void OnCheckStateChangedStaticOnly(const bool& InNewValue);

	void OnBlenderPathChanged(const FText& Text);
	void OnExportPathChanged(const FText& Text);

	TSharedPtr<SWidget> PickerWidget;
	TSharedPtr<SButton> BrowseButton;
};