

#include "CognitiveVREditorModule.h"

//sets up customization for settings
//adds scene setup window
//creates editortools

#define LOCTEXT_NAMESPACE "DemoTools"

class FCognitiveVREditorModule : public IModuleInterface, IHasMenuExtensibility, IHasToolBarExtensibility
{
public:

	//	FName SequenceRecorderTabName = FName("SequenceRecorder");
		// IMoudleInterface interface
		//virtual void StartupModule() override;
		//virtual void ShutdownModule() override;
		// End of IModuleInterface interface

		//static void OnToolWindowClosed(const TSharedRef<SWindow>& Window, UBaseEditorTool* Instance);

		//void AddMenuEntry(FMenuBuilder& MenuBuilder);
		//void DisplayPopup();
		//void SpawnSequenceRecorderTab(const FSpawnTabArgs& SpawnTabArgs);

		/*static void HandleTestCommandExcute();

		static bool HandleTestCommandCanExcute();*/

	//TSharedPtr<FUICommandList> CommandList;

	//FCognitiveTools* CognitiveEditorTools;

	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	
	//TODO CONSIDER do we need this for anything?
	bool Tick(float deltatime)
	{
		return true;
	}
	
	public:

	//~ IHasMenuExtensibility interface

	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override
	{
		return MenuExtensibilityManager;
	}

public:

	//~ IHasToolBarExtensibility interface

	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override
	{
		return ToolBarExtensibilityManager;
	}

	virtual void StartupModule() override
	{
		
#if WITH_EDITOR
		// Create the Extender that will add content to the menu
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		FString EngineIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEngine.ini"));
		FString EditorIni = FPaths::Combine(*(FPaths::ProjectDir()), TEXT("Config/DefaultEditor.ini"));

		FString tempGateway;
		GConfig->GetString(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("Gateway"), tempGateway, EngineIni);

		if (tempGateway.IsEmpty())
		{
			GLog->Log("CognitiveVRModule::StartupModule write defaults to ini");
			FString defaultgateway = "data.cognitive3d.com";
			FString defaultsessionviewer = "viewer.cognitive3d.com/scene/";
			FString defaultdashboard = "app.cognitive3d.com";
			FString trueString = "True";
			GConfig->SetString(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("Gateway"), *defaultgateway, EngineIni);
			GConfig->SetString(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("SessionViewer"), *defaultsessionviewer, EngineIni);
			GConfig->SetString(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("Dashboard"), *defaultdashboard, EngineIni);

			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("GazeBatchSize"), 64, EngineIni);

			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("CustomEventBatchSize"), 64, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("CustomEventExtremeLimit"), 128, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("CustomEventMinTimer"), 2, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("CustomEventAutoTimer"), 30, EngineIni);

			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("DynamicDataLimit"), 64, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("DynamicExtremeLimit"), 128, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("DynamicMinTimer"), 2, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("DynamicAutoTimer"), 30, EngineIni);

			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("SensorDataLimit"), 64, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("SensorExtremeLimit"), 128, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("SensorMinTimer"), 2, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("SensorAutoTimer"), 30, EngineIni);

			GConfig->SetString(TEXT("Analytics"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			GConfig->SetString(TEXT("AnalyticsDebug"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			GConfig->SetString(TEXT("AnalyticsTest"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			GConfig->SetString(TEXT("AnalyticsDevelopment"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			
			GConfig->SetString(TEXT("Analytics"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			GConfig->SetString(TEXT("AnalyticsDebug"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			GConfig->SetString(TEXT("AnalyticsTest"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);
			GConfig->SetString(TEXT("AnalyticsDevelopment"), TEXT("ProviderModuleName"), TEXT("CognitiveVR"), EngineIni);

			GConfig->SetString(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("EnableLocalCache"), *trueString, EngineIni);
			GConfig->SetInt(TEXT("/Script/CognitiveVR.CognitiveVRSettings"), TEXT("LocalCacheSize"), 100, EngineIni);
		}
		
		FCognitiveEditorTools::Initialize();
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TSharedPtr< FDynamicIdPoolAssetActions> action = MakeShared<FDynamicIdPoolAssetActions>();
		AssetTools.RegisterAssetTypeActions(action.ToSharedRef());
		GConfig->GetString(TEXT("Analytics"), TEXT("ApiKey"), FCognitiveEditorTools::GetInstance()->ApplicationKey, EngineIni);
		GConfig->GetString(TEXT("Analytics"), TEXT("AttributionKey"), FCognitiveEditorTools::GetInstance()->AttributionKey, EngineIni);
		GConfig->GetString(TEXT("Analytics"), TEXT("DeveloperKey"), FAnalyticsCognitiveVR::Get().DeveloperKey, EditorIni);
		GConfig->GetString(TEXT("Analytics"), TEXT("BlenderPath"), FCognitiveEditorTools::GetInstance()->BlenderPath, EditorIni);
		GConfig->GetString(TEXT("Analytics"), TEXT("ExportPath"), FCognitiveEditorTools::GetInstance()->BaseExportDirectory, EditorIni);
		//ConfigFileHasChanged = true;

		//TickDelegate = FTickerDelegate::CreateRaw(this, &FCognitiveVREditorModule::Tick);
		//TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);

		/*
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension(
			"EditMain",
			EExtensionHook::After,
			NULL,
			FMenuExtensionDelegate::CreateRaw(this, &FCognitiveVREditorModule::AddMenuEntry)
		);

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);*/


		// register 'keep simulation changes' recorder
		//FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		//LevelEditorModule.OnCaptureSingleFrameAnimSequence().BindStatic(&FCognitiveVREditorModule::HandleCaptureSingleFrameAnimSequence);

		// register standalone UI
		LevelEditorTabManagerChangedHandle = LevelEditorModule.OnTabManagerChanged().AddLambda([]()
		{
			//TSharedPtr<FSlateStyleSet> StyleSet = MakeShareable(new FSlateStyleSet("CognitiveEditor"));
			//StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));
			
			//FString iconpath = IPluginManager::Get().FindPlugin(TEXT("CognitiveVR"))->GetBaseDir() / TEXT("Resources") / TEXT("CognitiveSceneWizardTabIcon.png");
			//FName BrushName = FName(*iconpath);

			//const TCHAR* charPath = *iconpath;
			//StyleSet->Set(FName(*iconpath),new FSlateImageBrush(iconpath,FVector2D(128,128),FSlateColor()));

			//FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get()); //need to make a new class for this style? silently crashes unreal if this is called here

			//FSlateIcon& iconRef = FSlateIcon(StyleSet, "CognitiveSceneWizardTabIcon");

			FLevelEditorModule& LocalLevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			LocalLevelEditorModule.GetLevelEditorTabManager()->RegisterTabSpawner(FName("CognitiveSceneSetup"), FOnSpawnTab::CreateStatic(&FCognitiveVREditorModule::SpawnCognitiveSceneSetupTab))
				.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
				.SetDisplayName(LOCTEXT("SceneSetupTabTitle", "Cognitive Scene Setup"))
				.SetTooltipText(LOCTEXT("SceneSetupTooltipText", "Open the Cognitive Scene Setup Wizard"));

			LocalLevelEditorModule.GetLevelEditorTabManager()->RegisterTabSpawner(FName("CognitiveDynamicObjectManager"), FOnSpawnTab::CreateStatic(&FCognitiveVREditorModule::SpawnCognitiveDynamicTab))
				.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
				.SetDisplayName(LOCTEXT("DynamicObjectManagerTabTitle", "Cognitive Dynamic Object Manager"))
				.SetTooltipText(LOCTEXT("DynamicObjectManagerTooltipText", "Open the Cognitive Dynamic Object Manager"));
				//.SetIcon(FSlateIcon(StyleSet.Get()->GetStyleSetName(), "CognitiveSceneWizardTabIcon"));
		});

		// Register the details customizations
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

		PropertyModule.RegisterCustomClassLayout(TEXT("CognitiveVRSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FCognitiveSettingsCustomization::MakeInstance));
		//PropertyModule.RegisterCustomClassLayout(TEXT("BaseEditorTool"), FOnGetDetailCustomizationInstance::CreateStatic(&FSetupCustomization::MakeInstance));
		PropertyModule.RegisterCustomClassLayout(TEXT("DynamicObject"), FOnGetDetailCustomizationInstance::CreateStatic(&UDynamicObjectComponentDetails::MakeInstance));
		PropertyModule.RegisterCustomClassLayout(TEXT("DynamicIdPoolAsset"), FOnGetDetailCustomizationInstance::CreateStatic(&UDynamicIdPoolAssetDetails::MakeInstance));
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR

		//FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

		if (GEditor)
		{
			//FDemoCommands::Unregister();
			//FDemoStyle::Shutdown();

			if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
			{
				FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				//LevelEditorModule.OnCaptureSingleFrameAnimSequence().Unbind();
				LevelEditorModule.OnTabManagerChanged().Remove(LevelEditorTabManagerChangedHandle);
			}
		}
#endif
	}
	
	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}

	void OnToolWindowClosed(const TSharedRef<SWindow>& Window, UBaseEditorTool* Instance)
	{
		Instance->RemoveFromRoot();
	}

	static TSharedRef<SDockTab> SpawnCognitiveDynamicTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		const TSharedRef<SDockTab> MajorTab =
			SNew(SDockTab)
			//.Icon(FEditorStyle::Get().GetBrush("SequenceRecorder.TabIcon"))
			.TabRole(ETabRole::MajorTab);

		MajorTab->SetContent(SNew(SDynamicObjectManagerWidget));

		return MajorTab;
	}

	static TSharedRef<SDockTab> SpawnCognitiveSceneSetupTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		const TSharedRef<SDockTab> MajorTab =
			SNew(SDockTab)
			//.Icon(FEditorStyle::Get().GetBrush("SequenceRecorder.TabIcon"))
			.TabRole(ETabRole::MajorTab);

		MajorTab->SetContent(SNew(SSceneSetupWidget));

		return MajorTab;
	}

	FDelegateHandle LevelEditorTabManagerChangedHandle;
	
	protected:

	/** Unregisters asset tool actions. */
	void UnregisterAssetTools()
	{
		FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

		if (AssetToolsModule != nullptr)
		{
			IAssetTools& AssetTools = AssetToolsModule->Get();

			for (auto Action : RegisteredAssetTypeActions)
			{
				AssetTools.UnregisterAssetTypeActions(Action);
			}
		}
	}

protected:

	/** Registers main menu and tool bar menu extensions. */
	void RegisterMenuExtensions()
	{
		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);
	}

	/** Unregisters main menu and tool bar menu extensions. */
	void UnregisterMenuExtensions()
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();
	}

private:

	/** Holds the menu extensibility manager. */
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** Holds the plug-ins style set. */
	//TSharedPtr<ISlateStyle> Style;

	/** Holds the tool bar extensibility manager. */
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;
};
IMPLEMENT_MODULE(FCognitiveVREditorModule, CognitiveVREditor);

#undef LOCTEXT_NAMESPACE
