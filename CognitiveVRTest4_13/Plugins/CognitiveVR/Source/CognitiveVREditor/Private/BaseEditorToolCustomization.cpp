#include "CognitiveVREditorPrivatePCH.h"
#include "BaseEditorToolCustomization.h"
#include "PropertyEditing.h"
//#include "DetailCustomizationsPrivatePCH.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "BaseToolEditor"

void FBaseEditorToolCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& ForwardRenderingCategory = DetailBuilder.EditCategory(TEXT("Forward Rendering Overrides"));

	TSet<UClass*> Classes;

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	for (auto WeakObject : ObjectsBeingCustomized)
	{
		if (UObject* Instance = WeakObject.Get())
		{
			Classes.Add(Instance->GetClass());
		}
	}

	//should be a struct/class with scene name and scene key


	// Create a commands category
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Commands"));

	//go through all the functions. hand code functions into an order

	TArray<UFunction*> Functions;
	//Functions.Init(UFunction&, 4);
	Functions.SetNum(5, false);

	for (UClass* Class : Classes)
	{
		for (TFieldIterator<UFunction> FuncIt(Class); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			if (Function->HasAnyFunctionFlags(FUNC_Exec) && (Function->NumParms == 0))
			{
				const FString FunctionName = Function->GetName();

				if (FunctionName == "SelectExportMeshes")
				{
					Functions[1] = Function;
				}
				if (FunctionName == "SelectBlender")
				{
					Functions[0] = Function;
				}
				if (FunctionName == "ExportScene")
				{
					Functions[2] = Function;
				}
				if (FunctionName == "RunBlenderCleanup")
				{
					Functions[3] = Function;
				}
				if (FunctionName == "ConvertTextures")
				{
					Functions[4] = Function;
				}

				/*const FText ButtonCaption = FText::FromString(FunctionName);
				const FString FilterString = FunctionName;

				Category.AddCustomRow(FText::FromString(FilterString))
				.ValueContent()
				[
					SNew(SButton)
					.Text(ButtonCaption)
					.OnClicked(FOnClicked::CreateStatic(&FBaseEditorToolCustomization::ExecuteToolCommand, &DetailBuilder, Function))
				];*/
			}
		}
		
		for (auto& Function : Functions)
		{
			const FString FunctionName = Function->GetName();
			const FText ButtonCaption = FText::FromString(FunctionName);
			
			const FString FilterString = FunctionName;

			Category.AddCustomRow(FText::FromString(FilterString))
				.ValueContent()
				[
					SNew(SButton)
					.Text(ButtonCaption)
					.OnClicked(FOnClicked::CreateStatic(&FBaseEditorToolCustomization::ExecuteToolCommand, &DetailBuilder, Function))
				];
		}
	}
}

TSharedRef<IDetailCustomization> FBaseEditorToolCustomization::MakeInstance()
{
	return MakeShareable(new FBaseEditorToolCustomization);
}

FReply FBaseEditorToolCustomization::ExecuteToolCommand(IDetailLayoutBuilder* DetailBuilder, UFunction* MethodToExecute)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder->GetObjectsBeingCustomized(ObjectsBeingCustomized);

	for (auto WeakObject : ObjectsBeingCustomized)
	{
		if (UObject* Instance = WeakObject.Get())
		{
			Instance->CallFunctionByNameWithArguments(*MethodToExecute->GetName(), *GLog, nullptr, true);
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE