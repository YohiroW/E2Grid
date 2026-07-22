#include "E2GridEdMode.h"

#include "E2GridCreateSettings.h"
#include "E2GridEdModeToolkit.h"
#include "E2GridManager.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Math/RotationMatrix.h"
#include "PrimitiveDrawInterface.h"
#include "ScopedTransaction.h"
#include "SceneManagement.h"
#include "Textures/SlateIcon.h"
#include "UnrealWidget.h"

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

void UE2GridEdMode::Initialize()
{
	Super::Initialize();
	CreateSettings = NewObject<UE2GridCreateSettings>(this);
}

void UE2GridEdMode::Enter()
{
	if (!CreateSettings)
	{
		CreateSettings = NewObject<UE2GridCreateSettings>(this);
	}
	CreateSettings->ResetToDefaults();
	ActivePage = EE2GridEdModePage::Create;

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
	Toolkit = MakeShareable(new FE2GridEdModeToolkit);
}

void UE2GridEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	Super::Tick(ViewportClient, DeltaTime);
}

void UE2GridEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	Super::Render(View, Viewport, PDI);

	if (!IsCreatePageActive() || !CreateSettings || CreateSettings->GridDimension.X <= 0 ||
		CreateSettings->GridDimension.Y <= 0 || CreateSettings->GridSize <= 0)
	{
		return;
	}

	const int32 Width = CreateSettings->GridDimension.X;
	const int32 Height = CreateSettings->GridDimension.Y;
	const double GridSize = static_cast<double>(CreateSettings->GridSize);
	const double HalfWidth = static_cast<double>(Width) * GridSize * 0.5;
	const double HalfHeight = static_cast<double>(Height) * GridSize * 0.5;
	const FTransform PreviewTransform(CreateSettings->Rotation, CreateSettings->Location);
	const FLinearColor InnerLineColor(0.05f, 0.65f, 0.10f);
	const FLinearColor BorderLineColor(0.10f, 1.00f, 0.20f);

	for (int32 X = 0; X <= Width; ++X)
	{
		const double LocalX = -HalfWidth + static_cast<double>(X) * GridSize;
		const FVector Start = PreviewTransform.TransformPosition(FVector(LocalX, -HalfHeight, 0.0));
		const FVector End = PreviewTransform.TransformPosition(FVector(LocalX, HalfHeight, 0.0));
		const bool bIsBorder = X == 0 || X == Width;
		PDI->DrawLine(Start, End, bIsBorder ? BorderLineColor : InnerLineColor, SDPG_Foreground, bIsBorder ? 2.0f : 0.75f);
	}

	for (int32 Y = 0; Y <= Height; ++Y)
	{
		const double LocalY = -HalfHeight + static_cast<double>(Y) * GridSize;
		const FVector Start = PreviewTransform.TransformPosition(FVector(-HalfWidth, LocalY, 0.0));
		const FVector End = PreviewTransform.TransformPosition(FVector(HalfWidth, LocalY, 0.0));
		const bool bIsBorder = Y == 0 || Y == Height;
		PDI->DrawLine(Start, End, bIsBorder ? BorderLineColor : InnerLineColor, SDPG_Foreground, bIsBorder ? 2.0f : 0.75f);
	}
}

bool UE2GridEdMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	if (IsCreatePageActive() && CreateSettings && InViewportClient->GetCurrentWidgetAxis() != EAxisList::None &&
		(!InDrag.IsNearlyZero() || !InRot.IsNearlyZero()))
	{
		CreateSettings->Location += InDrag;
		CreateSettings->Rotation += InRot;
		CreateSettings->Rotation.Normalize();
		NotifyCreateSettingsChanged(true);
		return true;
	}

	return Super::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

bool UE2GridEdMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	return Super::HandleClick(InViewportClient, HitProxy, Click);
}

bool UE2GridEdMode::ShouldDrawWidget() const
{
	return IsCreatePageActive();
}

bool UE2GridEdMode::UsesTransformWidget() const
{
	return IsCreatePageActive();
}

bool UE2GridEdMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	return IsCreatePageActive() && (CheckMode == UE::Widget::WM_Translate || CheckMode == UE::Widget::WM_Rotate);
}

bool UE2GridEdMode::UsesPropertyWidgets() const
{
	return false;
}

EAxisList::Type UE2GridEdMode::GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const
{
	if (!IsCreatePageActive())
	{
		return EAxisList::None;
	}

	switch (InWidgetMode)
	{
	case UE::Widget::WM_Translate:
		return EAxisList::XYZ;
	case UE::Widget::WM_Rotate:
		return EAxisList::Z;
	default:
		return EAxisList::None;
	}
}

FVector UE2GridEdMode::GetWidgetLocation() const
{
	return CreateSettings ? CreateSettings->Location : FVector::ZeroVector;
}

bool UE2GridEdMode::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (!IsCreatePageActive() || !CreateSettings)
	{
		return false;
	}

	InMatrix = FRotationMatrix(CreateSettings->Rotation);
	return true;
}

bool UE2GridEdMode::GetCustomInputCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	return GetCustomDrawingCoordinateSystem(InMatrix, InData);
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

void UE2GridEdMode::SetActivePage(EE2GridEdModePage InPage)
{
	if (ActivePage != InPage)
	{
		ActivePage = InPage;
		NotifyCreateSettingsChanged(false);
	}
}

void UE2GridEdMode::NotifyCreateSettingsChanged(bool bRefreshDetails)
{
	if (bRefreshDetails && Toolkit.IsValid())
	{
		StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshCreateSettings();
	}

	if (GEditor)
	{
		GEditor->RedrawAllViewports(false);
	}
}

bool UE2GridEdMode::CanCreateGridManager() const
{
	const UWorld* World = GetWorld();
	return GEditor && CreateSettings && World && World->GetCurrentLevel() &&
		CreateSettings->GridDimension.X > 0 && CreateSettings->GridDimension.Y > 0 && CreateSettings->GridSize > 0;
}

void UE2GridEdMode::CreateGridManager()
{
	if (!CanCreateGridManager())
	{
		return;
	}

	UWorld* World = GetWorld();
	ULevel* CurrentLevel = World->GetCurrentLevel();
	const FTransform SpawnTransform(CreateSettings->Rotation, CreateSettings->Location);
	FScopedTransaction Transaction(LOCTEXT("CreateGridManagerTransaction", "Create E2 Grid Manager"));

	AE2GridManager* GridManager = Cast<AE2GridManager>(GEditor->AddActor(
		CurrentLevel,
		AE2GridManager::StaticClass(),
		SpawnTransform,
		false,
		RF_Transactional,
		false));
	if (!GridManager)
	{
		Transaction.Cancel();
		return;
	}

	GridManager->Modify();
	GridManager->GridDimension = CreateSettings->GridDimension;
	GridManager->GridSize = CreateSettings->GridSize;
	GridManager->GridDataClass = CreateSettings->GridDataClass;
	GridManager->Generate();
	GridManager->MarkPackageDirty();

	GEditor->SelectNone(false, true, false);
	GEditor->SelectActor(GridManager, true, true, true);
	GEditor->RedrawAllViewports();
}

bool UE2GridEdMode::IsCreatePageActive() const
{
	return ActivePage == EE2GridEdModePage::Create;
}

#undef LOCTEXT_NAMESPACE

