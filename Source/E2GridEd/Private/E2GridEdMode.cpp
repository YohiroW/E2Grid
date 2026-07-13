#include "E2GridEdMode.h"

#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "E2GridEdMode"

const FEditorModeID UE2GridEdMode::EM_E2GridEdModeId = TEXT("EditMode.E2Grid");

UE2GridEdMode::UE2GridEdMode()
{
	Info = FEditorModeInfo(
		UE2GridEdMode::EM_E2GridEdModeId,
		LOCTEXT("E2GridEdModeName", "Grid"),
		FSlateIcon(),
		true);
}

UE2GridEdMode::~UE2GridEdMode()
{
}

void UE2GridEdMode::Enter()
{
	Super::Enter();

	// GEditor->OnEditorClose().AddUObject(this, &UMeshPaintMode::OnResetViewMode);
	// FCoreUObjectDelegates::OnObjectsReplaced.AddUObject(this, &UMeshPaintMode::OnObjectsReplaced);
	// ModeSettings = Cast<UMeshPaintModeSettings>(SettingsObject);
	//
	// FMeshPaintEditorModeCommands ToolManagerCommands = FMeshPaintEditorModeCommands::Get();
	//
	// UVertexAdapterClickToolBuilder* VertexClickToolBuilder = NewObject<UVertexAdapterClickToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.SelectVertex, VertexSelectToolName, VertexClickToolBuilder);
	//
	// UTextureColorAdapterClickToolBuilder* TextureColorClickToolBuilder = NewObject<UTextureColorAdapterClickToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.SelectTextureColor, TextureColorSelectToolName, TextureColorClickToolBuilder);
	//
	// UTextureAssetAdapterClickToolBuilder* TextureAssetClickToolBuilder = NewObject<UTextureAssetAdapterClickToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.SelectTextureAsset, TextureAssetSelectToolName, TextureAssetClickToolBuilder);
	//
	// UMeshVertexColorPaintingToolBuilder* MeshColorPaintingToolBuilder = NewObject<UMeshVertexColorPaintingToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.PaintVertexColor, VertexColorPaintToolName, MeshColorPaintingToolBuilder);
	//
	// UMeshVertexWeightPaintingToolBuilder* WeightPaintingToolBuilder = NewObject<UMeshVertexWeightPaintingToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.PaintVertexWeight, VertexWeightPaintToolName, WeightPaintingToolBuilder);
	//
	// UMeshTextureColorPaintingToolBuilder* MeshTextureColorPaintingToolBuilder = NewObject<UMeshTextureColorPaintingToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.PaintTextureColor, TextureColorPaintToolName, MeshTextureColorPaintingToolBuilder);
	//
	// UMeshTextureAssetPaintingToolBuilder* TextureAssetPaintingToolBuilder = NewObject<UMeshTextureAssetPaintingToolBuilder>(this);
	// RegisterTool(ToolManagerCommands.PaintTextureAsset, TextureAssetPaintToolName, TextureAssetPaintingToolBuilder);
	//
	// UpdateSelectedMeshes();
	//
	// // Toolkit
	// PaletteChangedHandle = Toolkit->OnPaletteChanged().AddUObject(this, &UMeshPaintMode::UpdateOnPaletteChange);
	//
	// // disable tool change tracking to activate default tool
	// GetToolManager()->ConfigureChangeTrackingMode(EToolChangeTrackingMode::NoChangeTracking);
	// Toolkit->SetCurrentPalette(GetValidPaletteName(ModeSettings->DefaultPalette));
	// // switch back to full undo / redo tracking mode here if that is behavior we want
	// //GetToolManager()->ConfigureChangeTrackingMode(EToolChangeTrackingMode::FullUndoRedo);
	//
	// FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(FName(TEXT("LevelEditor")));
	// LevelEditor.OnRedrawLevelEditingViewports().AddUObject(this, &UMeshPaintMode::UpdateOnMaterialChange);
	//
	// FAssetCompilingManager::Get().OnAssetPostCompileEvent().AddUObject(this, &UMeshPaintMode::UpdateOnPostCompile);
	//
	// // some global cvars can affect whether painting is valid (nanite on/off etc)
	// CVarDelegateHandle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(FConsoleCommandDelegate::CreateLambda([this]{ bRecacheValidForPaint = true; }));
}

void UE2GridEdMode::Exit()
{
	// ModeSettings->DefaultPalette = Toolkit->GetCurrentPalette();
	//
	// Toolkit->OnPaletteChanged().Remove(PaletteChangedHandle);
	// FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	// GEditor->OnEditorClose().RemoveAll(this);
	// OnResetViewMode();
	//
	// const FMeshPaintEditorModeCommands& Commands = FMeshPaintEditorModeCommands::Get();
	// const TSharedRef<FUICommandList>& CommandList = Toolkit->GetToolkitCommands();
	// for (auto It : Commands.Commands)
	// {
	// 	for (const TSharedPtr<const FUICommandInfo> Action : It.Value)
	// 	{
	// 		CommandList->UnmapAction(Action);
	// 	}
	// }

	Super::Exit();

	// GEngine->GetEngineSubsystem<UMeshPaintingSubsystem>()->ResetState();
	//
	// FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(FName(TEXT("LevelEditor")));
	// LevelEditor.OnRedrawLevelEditingViewports().RemoveAll(this);
	//
	// FAssetCompilingManager::Get().OnAssetPostCompileEvent().RemoveAll(this);
	//
	// IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(CVarDelegateHandle);
	// CVarDelegateHandle = {};
}

void UE2GridEdMode::CreateToolkit()
{
	Super::CreateToolkit();
}

void UE2GridEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	ILegacyEdModeViewportInterface::Tick(ViewportClient, DeltaTime);
}

bool UE2GridEdMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	return ILegacyEdModeViewportInterface::HandleClick(InViewportClient, HitProxy, Click);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UE2GridEdMode::GetModeCommands() const
{
	return Super::GetModeCommands();
}

void UE2GridEdMode::BindCommands()
{
	Super::BindCommands();
}

void UE2GridEdMode::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	Super::OnToolStarted(Manager, Tool);
}

void UE2GridEdMode::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	Super::OnToolEnded(Manager, Tool);
}

void UE2GridEdMode::ActorSelectionChangeNotify()
{
	Super::ActorSelectionChangeNotify();
}

void UE2GridEdMode::ElementSelectionChangeNotify()
{
	Super::ElementSelectionChangeNotify();
}

void UE2GridEdMode::ActorPropChangeNotify()
{
	Super::ActorPropChangeNotify();
}

void UE2GridEdMode::ActivateDefaultTool()
{
	Super::ActivateDefaultTool();
}

void UE2GridEdMode::UpdateOnPaletteChange(FName NewPalette)
{
}

#undef LOCTEXT_NAMESPACE

