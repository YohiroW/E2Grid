#include "E2GridEdMode.h"

#include "E2GridEdModeSettings.h"
#include "E2GridEdModeToolkit.h"
#include "E2GridManager.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
	Settings = NewObject<UE2GridEdModeSettings>(this);
}

void UE2GridEdMode::Enter()
{
	if (!Settings)
	{
		Settings = NewObject<UE2GridEdModeSettings>(this);
	}
	Settings->ResetToDefaults();
	bSettingsDirty = false;
	ActivePage = EE2GridEdModePage::Grid;

	Super::Enter();

	if (GEngine)
	{
		LevelActorAddedHandle = GEngine->OnLevelActorAdded().AddUObject(this, &UE2GridEdMode::HandleLevelActorAdded);
		LevelActorDeletedHandle = GEngine->OnLevelActorDeleted().AddUObject(this, &UE2GridEdMode::HandleLevelActorDeleted);
	}
	RefreshGridManagers();

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
	if (GEngine)
	{
		GEngine->OnLevelActorAdded().Remove(LevelActorAddedHandle);
		GEngine->OnLevelActorDeleted().Remove(LevelActorDeletedHandle);
	}
	LevelActorAddedHandle.Reset();
	LevelActorDeletedHandle.Reset();
	GridManagers.Reset();
	ActiveGridManager.Reset();
	bSettingsDirty = false;

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

	if (!IsGridPageActive() || !IsCreatingGridManager() || !Settings || !Settings->IsValid())
	{
		return;
	}

	const int32 Width = Settings->GridDimension.X;
	const int32 Height = Settings->GridDimension.Y;
	const double GridSize = static_cast<double>(Settings->GridSize);
	const double HalfWidth = static_cast<double>(Width) * GridSize * 0.5;
	const double HalfHeight = static_cast<double>(Height) * GridSize * 0.5;
	const FTransform PreviewTransform(Settings->Rotation, Settings->Location);
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
	if (IsGridPageActive() && IsCreatingGridManager() && Settings &&
		InViewportClient->GetCurrentWidgetAxis() != EAxisList::None &&
		(!InDrag.IsNearlyZero() || !InRot.IsNearlyZero()))
	{
		Settings->Location += InDrag;
		Settings->Rotation += InRot;
		Settings->Rotation.Normalize();
		NotifySettingsChanged(true);
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
	return IsGridPageActive() && IsCreatingGridManager();
}

bool UE2GridEdMode::UsesTransformWidget() const
{
	return IsGridPageActive() && IsCreatingGridManager();
}

bool UE2GridEdMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	return IsGridPageActive() && IsCreatingGridManager() &&
		(CheckMode == UE::Widget::WM_Translate || CheckMode == UE::Widget::WM_Rotate);
}

bool UE2GridEdMode::UsesPropertyWidgets() const
{
	return false;
}

EAxisList::Type UE2GridEdMode::GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const
{
	if (!IsGridPageActive() || !IsCreatingGridManager())
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
	return Settings ? Settings->Location : FVector::ZeroVector;
}

bool UE2GridEdMode::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	if (!IsGridPageActive() || !IsCreatingGridManager() || !Settings)
	{
		return false;
	}

	InMatrix = FRotationMatrix(Settings->Rotation);
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

	if (bSettingsDirty)
	{
		return;
	}

	for (const TWeakObjectPtr<AE2GridManager>& GridManager : GridManagers)
	{
		if (GridManager.IsValid() && GridManager->IsSelected())
		{
			SetActiveGridManager(GridManager.Get());
			break;
		}
	}
}

void UE2GridEdMode::ElementSelectionChangeNotify()
{
	Super::ElementSelectionChangeNotify();
}

void UE2GridEdMode::ActorPropChangeNotify()
{
	Super::ActorPropChangeNotify();

	if (!bSettingsDirty && ActiveGridManager.IsValid())
	{
		RevertSettings();
	}
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
		if (GEditor)
		{
			GEditor->RedrawAllViewports(false);
		}
	}
}

void UE2GridEdMode::NotifySettingsChanged(bool bRefreshDetails)
{
	bSettingsDirty = IsCreatingGridManager() || (Settings &&
		!Settings->MatchesGridManager(*ActiveGridManager));

	if (bRefreshDetails && Toolkit.IsValid())
	{
		StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshSettings();
	}

	if (GEditor)
	{
		GEditor->RedrawAllViewports(false);
	}
}

void UE2GridEdMode::RefreshGridManagers()
{
	const bool bKeepCreateDraft = IsCreatingGridManager() && bSettingsDirty;
	AE2GridManager* PreviousGridManager = ActiveGridManager.Get();
	GridManagers.Reset();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AE2GridManager> It(World); It; ++It)
		{
			AE2GridManager* GridManager = *It;
			if (IsValid(GridManager) && !GridManager->IsActorBeingDestroyed())
			{
				GridManagers.Add(GridManager);
			}
		}
	}

	GridManagers.Sort([](
		const TWeakObjectPtr<AE2GridManager>& Left,
		const TWeakObjectPtr<AE2GridManager>& Right)
	{
		const AE2GridManager* LeftManager = Left.Get();
		const AE2GridManager* RightManager = Right.Get();
		return LeftManager && RightManager
			? LeftManager->GetActorLabel() < RightManager->GetActorLabel()
			: LeftManager != nullptr;
	});

	const bool bPreviousManagerStillExists = PreviousGridManager && GridManagers.ContainsByPredicate(
		[PreviousGridManager](const TWeakObjectPtr<AE2GridManager>& GridManager)
		{
			return GridManager.Get() == PreviousGridManager;
		});

	AE2GridManager* NewGridManager = bPreviousManagerStillExists ? PreviousGridManager : nullptr;
	if (!NewGridManager && !bKeepCreateDraft)
	{
		for (const TWeakObjectPtr<AE2GridManager>& GridManager : GridManagers)
		{
			if (GridManager.IsValid() && GridManager->IsSelected())
			{
				NewGridManager = GridManager.Get();
				break;
			}
		}
	}
	if (!NewGridManager && !bKeepCreateDraft && GridManagers.Num() == 1)
	{
		NewGridManager = GridManagers[0].Get();
	}

	const bool bTargetChanged = ActiveGridManager.Get() != NewGridManager;
	ActiveGridManager = NewGridManager;
	if (Settings && (bTargetChanged || !bSettingsDirty))
	{
		if (NewGridManager)
		{
			Settings->LoadFromGridManager(*NewGridManager);
		}
		else
		{
			Settings->ResetToDefaults();
		}
		bSettingsDirty = false;
	}

	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshSettings();
	}
	if (GEditor)
	{
		GEditor->RedrawAllViewports(false);
	}
}

void UE2GridEdMode::SetActiveGridManager(AE2GridManager* InGridManager)
{
	if (InGridManager && InGridManager->GetWorld() != GetWorld())
	{
		return;
	}
	if (ActiveGridManager.Get() == InGridManager)
	{
		return;
	}

	ActiveGridManager = InGridManager;
	bSettingsDirty = false;
	if (Settings)
	{
		if (InGridManager)
		{
			Settings->LoadFromGridManager(*InGridManager);
		}
		else
		{
			Settings->ResetToDefaults();
		}
	}

	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshSettings();
	}
	if (GEditor)
	{
		GEditor->RedrawAllViewports(false);
	}
}

bool UE2GridEdMode::HasPendingSettings() const
{
	return bSettingsDirty && Settings && (IsCreatingGridManager() ||
		!Settings->MatchesGridManager(*ActiveGridManager));
}

bool UE2GridEdMode::CanCommitSettings() const
{
	if (!GEditor || !Settings || !Settings->IsValid())
	{
		return false;
	}

	if (!IsCreatingGridManager())
	{
		return HasPendingSettings();
	}

	const UWorld* World = GetWorld();
	return World && World->GetCurrentLevel();
}

bool UE2GridEdMode::CommitSettings()
{
	if (!CanCommitSettings())
	{
		return false;
	}

	if (IsCreatingGridManager())
	{
		UWorld* World = GetWorld();
		const FTransform SpawnTransform(Settings->Rotation, Settings->Location);
		const FIntPoint GridDimension = Settings->GridDimension;
		const int32 GridSize = Settings->GridSize;
		const TSubclassOf<UE2GridRuntimeData> GridDataClass = Settings->GridDataClass;
		FScopedTransaction Transaction(LOCTEXT("CreateGridManagerTransaction", "Create E2 Grid Manager"));

		AE2GridManager* GridManager = Cast<AE2GridManager>(GEditor->AddActor(
			World->GetCurrentLevel(),
			AE2GridManager::StaticClass(),
			SpawnTransform,
			false,
			RF_Transactional,
			false));
		if (!GridManager)
		{
			Transaction.Cancel();
			return false;
		}

		GridManager->Modify();
		GridManager->GridDimension = GridDimension;
		GridManager->GridSize = GridSize;
		GridManager->GridDataClass = GridDataClass;
		GridManager->Generate();
		GridManager->MarkPackageDirty();

		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(GridManager, true, true, true);
		bSettingsDirty = false;
		RefreshGridManagers();
		SetActiveGridManager(GridManager);
		GEditor->RedrawAllViewports();
		return true;
	}

	AE2GridManager* GridManager = ActiveGridManager.Get();
	const bool bTransformChanged = !Settings->Location.Equals(GridManager->GetActorLocation()) ||
		!Settings->Rotation.Equals(GridManager->GetActorRotation());
	const bool bGridChanged = Settings->GridDimension != GridManager->GridDimension ||
		Settings->GridSize != GridManager->GridSize ||
		Settings->GridDataClass != GridManager->GridDataClass;

	FScopedTransaction Transaction(LOCTEXT("ApplyGridManagerSettingsTransaction", "Apply E2 Grid Manager Settings"));
	GridManager->Modify();
	if (USceneComponent* RootComponent = GridManager->GetRootComponent())
	{
		RootComponent->Modify();
	}

	if (bTransformChanged)
	{
		const FTransform NewTransform(
			Settings->Rotation,
			Settings->Location,
			GridManager->GetActorScale3D());
		GridManager->SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	GridManager->GridDimension = Settings->GridDimension;
	GridManager->GridSize = Settings->GridSize;
	GridManager->GridDataClass = Settings->GridDataClass;
	if (bGridChanged)
	{
		GridManager->Generate();
	}

	GridManager->MarkPackageDirty();
	Settings->LoadFromGridManager(*GridManager);
	bSettingsDirty = false;

	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshSettings();
	}
	if (GEditor)
	{
		GEditor->RedrawAllViewports();
	}
	return true;
}

void UE2GridEdMode::RevertSettings()
{
	if (Settings)
	{
		if (ActiveGridManager.IsValid())
		{
			Settings->LoadFromGridManager(*ActiveGridManager);
		}
		else
		{
			Settings->ResetToDefaults();
		}
		bSettingsDirty = false;
	}

	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshSettings();
	}
	if (GEditor)
	{
		GEditor->RedrawAllViewports(false);
	}
}

bool UE2GridEdMode::IsGridPageActive() const
{
	return ActivePage == EE2GridEdModePage::Grid;
}

void UE2GridEdMode::HandleLevelActorAdded(AActor* InActor)
{
	if (InActor && InActor->GetWorld() == GetWorld() && InActor->IsA<AE2GridManager>())
	{
		RefreshGridManagers();
	}
}

void UE2GridEdMode::HandleLevelActorDeleted(AActor* InActor)
{
	if (InActor && InActor->GetWorld() == GetWorld() && InActor->IsA<AE2GridManager>())
	{
		RefreshGridManagers();
	}
}

#undef LOCTEXT_NAMESPACE

