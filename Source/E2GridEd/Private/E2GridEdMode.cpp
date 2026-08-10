#include "E2GridEdMode.h"

#include "E2GridEdModeGridSettings.h"
#include "E2GridEdModeSettings.h"
#include "E2GridEdModeToolkit.h"
#include "E2GridManager.h"
#include "E2GridSettings.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Math/RotationMatrix.h"
#include "Misc/MessageDialog.h"
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
	GridSettings = NewObject<UE2GridEdModeGridSettings>(this);
}

void UE2GridEdMode::Enter()
{
	if (!Settings)
	{
		Settings = NewObject<UE2GridEdModeSettings>(this);
	}
	if (!GridSettings)
	{
		GridSettings = NewObject<UE2GridEdModeGridSettings>(this);
	}
	Settings->ResetToDefaults();
	ClearSelectedGrid();
	bSettingsDirty = false;
	ActivePage = EE2GridEdModePage::Grid;
	ActiveTool = EE2GridEdModeTool::New;

	Super::Enter();

	if (GEngine)
	{
		LevelActorAddedHandle = GEngine->OnLevelActorAdded().AddUObject(this, &UE2GridEdMode::HandleLevelActorAdded);
		LevelActorDeletedHandle = GEngine->OnLevelActorDeleted().AddUObject(this, &UE2GridEdMode::HandleLevelActorDeleted);
	}
	RefreshGridManagers();
	if (CanActivateTool(EE2GridEdModeTool::Edit))
	{
		SetActiveTool(EE2GridEdModeTool::Edit);
	}

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
	HoveredGridCoord.Reset();
	ClearSelectedGrid();
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

	if (!CanShowPreview())
	{
		return;
	}

	const int32 Width = Settings->GridDimension.X;
	const int32 Height = Settings->GridDimension.Y;
	const double GridSize = static_cast<double>(Settings->GridSize);
	const double HalfWidth = static_cast<double>(Width) * GridSize * 0.5;
	const double HalfHeight = static_cast<double>(Height) * GridSize * 0.5;
	const UE2GridSettings* RuntimeSettings = GetDefault<UE2GridSettings>();
	const FLinearColor PreviewColor = RuntimeSettings->PreviewColor;
	constexpr float PreviewThickness = 2.5f;
	constexpr float PreviewHeight = 0.5f;
	const FTransform PreviewTransform = GetPreviewTransform();
	const double CornerLength = FMath::Min(
		GridSize * 0.33,
		FMath::Min(HalfWidth, HalfHeight));

	const auto DrawLocalLine = [PDI, &PreviewTransform](
		const FVector& LocalStart,
		const FVector& LocalEnd,
		const FLinearColor& Color,
		float Thickness,
		ESceneDepthPriorityGroup DepthPriority)
	{
		PDI->DrawLine(
			PreviewTransform.TransformPosition(LocalStart),
			PreviewTransform.TransformPosition(LocalEnd),
			Color,
			DepthPriority,
			Thickness);
	};

	for (int32 X = 1; X < Width; ++X)
	{
		const double LocalX = -HalfWidth + static_cast<double>(X) * GridSize;
		DrawLocalLine(
			FVector(LocalX, -HalfHeight, PreviewHeight),
			FVector(LocalX, HalfHeight, PreviewHeight),
			PreviewColor,
			PreviewThickness,
			SDPG_World);
	}

	for (int32 Y = 1; Y < Height; ++Y)
	{
		const double LocalY = -HalfHeight + static_cast<double>(Y) * GridSize;
		DrawLocalLine(
			FVector(-HalfWidth, LocalY, PreviewHeight),
			FVector(HalfWidth, LocalY, PreviewHeight),
			PreviewColor,
			PreviewThickness,
			SDPG_World);
	}

	const auto DrawBorder = [&DrawLocalLine, CornerLength, &PreviewColor, PreviewThickness, PreviewHeight](
		const FVector& LocalStart,
		const FVector& LocalEnd)
	{
		const FVector Direction = (LocalEnd - LocalStart).GetSafeNormal();
		const double BorderLength = FVector::Distance(LocalStart, LocalEnd);
		const FVector CornerOffset = Direction * FMath::Min(CornerLength, BorderLength * 0.5);
		DrawLocalLine(LocalStart, LocalStart + CornerOffset, PreviewColor, PreviewThickness, SDPG_Foreground);
		DrawLocalLine(LocalStart + CornerOffset, LocalEnd - CornerOffset, PreviewColor, PreviewThickness, SDPG_Foreground);
		DrawLocalLine(LocalEnd - CornerOffset, LocalEnd, PreviewColor, PreviewThickness, SDPG_Foreground);
	};

	DrawBorder(FVector(-HalfWidth, -HalfHeight, PreviewHeight), FVector(HalfWidth, -HalfHeight, PreviewHeight));
	DrawBorder(FVector(HalfWidth, -HalfHeight, PreviewHeight), FVector(HalfWidth, HalfHeight, PreviewHeight));
	DrawBorder(FVector(HalfWidth, HalfHeight, PreviewHeight), FVector(-HalfWidth, HalfHeight, PreviewHeight));
	DrawBorder(FVector(-HalfWidth, HalfHeight, PreviewHeight), FVector(-HalfWidth, -HalfHeight, PreviewHeight));

	const auto DrawGridCellOutline = [
		&DrawLocalLine,
		Width,
		Height,
		GridSize,
		HalfWidth,
		HalfHeight
		]
	(
		const FIntPoint& Coord,
		const FLinearColor& Color,
		float Thickness,
		double LocalZ)
	{
		if (Coord.X < 0 || Coord.X >= Width || Coord.Y < 0 || Coord.Y >= Height)
		{
			return;
		}

		const double MinX = -HalfWidth + static_cast<double>(Coord.X) * GridSize;
		const double MinY = -HalfHeight + static_cast<double>(Coord.Y) * GridSize;
		const double MaxX = MinX + GridSize;
		const double MaxY = MinY + GridSize;
		DrawLocalLine(FVector(MinX, MinY, LocalZ), FVector(MaxX, MinY, LocalZ), Color, Thickness, SDPG_Foreground);
		DrawLocalLine(FVector(MaxX, MinY, LocalZ), FVector(MaxX, MaxY, LocalZ), Color, Thickness, SDPG_Foreground);
		DrawLocalLine(FVector(MaxX, MaxY, LocalZ), FVector(MinX, MaxY, LocalZ), Color, Thickness, SDPG_Foreground);
		DrawLocalLine(FVector(MinX, MaxY, LocalZ), FVector(MinX, MinY, LocalZ), Color, Thickness, SDPG_Foreground);
	};

	if (SelectedGridCoord.IsSet())
	{
		DrawGridCellOutline(
			SelectedGridCoord.GetValue(),
			RuntimeSettings->SelectedColor,
			5.0f,
			2* PreviewHeight);
	}
	if (HoveredGridCoord.IsSet())
	{
		DrawGridCellOutline(
			HoveredGridCoord.GetValue(),
			RuntimeSettings->HoverColor,
			5.0f,
			2* PreviewHeight);
	}
}

bool UE2GridEdMode::MouseMove(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	int32 X,
	int32 Y)
{
	TOptional<FIntPoint> NewHoveredGridCoord;
	if (CanShowPreview() && ViewportClient)
	{
		const FViewportCursorLocation MouseRay = ViewportClient->GetCursorWorldLocationFromMousePos();
		const FTransform PreviewTransform = GetPreviewTransform();
		const FVector PlaneOrigin = PreviewTransform.GetLocation();
		const FVector PlaneNormal = PreviewTransform.GetRotation().RotateVector(FVector::UpVector);
		const double RayPlaneDot = FVector::DotProduct(MouseRay.GetDirection(), PlaneNormal);
		if (!FMath::IsNearlyZero(RayPlaneDot))
		{
			const double HitDistance = FVector::DotProduct(
				PlaneOrigin - MouseRay.GetOrigin(),
				PlaneNormal) / RayPlaneDot;
			if (HitDistance >= 0.0)
			{
				const FVector WorldHit = MouseRay.GetOrigin() + MouseRay.GetDirection() * HitDistance;
				const FVector LocalHit = PreviewTransform.InverseTransformPosition(WorldHit);
				const double GridSize = static_cast<double>(Settings->GridSize);
				const double HalfWidth = static_cast<double>(Settings->GridDimension.X) * GridSize * 0.5;
				const double HalfHeight = static_cast<double>(Settings->GridDimension.Y) * GridSize * 0.5;
				const FIntPoint Coord(
					FMath::FloorToInt((LocalHit.X + HalfWidth) / GridSize),
					FMath::FloorToInt((LocalHit.Y + HalfHeight) / GridSize));
				if (Coord.X >= 0 && Coord.X < Settings->GridDimension.X &&
					Coord.Y >= 0 && Coord.Y < Settings->GridDimension.Y)
				{
					NewHoveredGridCoord = Coord;
				}
			}
		}
	}

	const bool bHoverChanged = HoveredGridCoord.IsSet() != NewHoveredGridCoord.IsSet() ||
		(HoveredGridCoord.IsSet() && HoveredGridCoord.GetValue() != NewHoveredGridCoord.GetValue());
	if (bHoverChanged)
	{
		HoveredGridCoord = NewHoveredGridCoord;
		ViewportClient->Invalidate(false, false);
	}

	return Super::MouseMove(ViewportClient, Viewport, X, Y);
}

bool UE2GridEdMode::MouseLeave(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	if (HoveredGridCoord.IsSet())
	{
		HoveredGridCoord.Reset();
		ViewportClient->Invalidate(false, false);
	}
	return Super::MouseLeave(ViewportClient, Viewport);
}

bool UE2GridEdMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	if (CanUseTransformWidget() &&
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
	const bool bClickedWidget = HitProxy && HitProxy->IsA(HWidgetAxis::StaticGetType());
	if (!bClickedWidget &&
		IsGridPageActive() &&
		IsToolActive(EE2GridEdModeTool::Edit) &&
		Click.GetKey() == EKeys::LeftMouseButton &&
		HoveredGridCoord.IsSet() &&
		ActiveGridManager.IsValid())
	{
		const FIntPoint Coord = HoveredGridCoord.GetValue();
		FE2GridCoord GridCoord;
		GridCoord.X = Coord.X;
		GridCoord.Y = Coord.Y;
		GridCoord.Layer = 0;

		FE2GridRuntimeData GridData;
		if (ActiveGridManager->TryGetGridData(GridCoord, GridData))
		{
			SelectedGridCoord = Coord;
			GridSettings->LoadFrom(GridData);
		}
		else
		{
			ClearSelectedGrid();
		}

		if (Toolkit.IsValid())
		{
			StaticCastSharedPtr<FE2GridEdModeToolkit>(Toolkit)->RefreshSettings();
		}
		if (GEditor)
		{
			GEditor->RedrawAllViewports(false);
		}
		return true;
	}

	return Super::HandleClick(InViewportClient, HitProxy, Click);
}

bool UE2GridEdMode::ShouldDrawWidget() const
{
	return CanUseTransformWidget();
}

bool UE2GridEdMode::UsesTransformWidget() const
{
	return CanUseTransformWidget();
}

bool UE2GridEdMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	return CanUseTransformWidget() &&
		(CheckMode == UE::Widget::WM_Translate || CheckMode == UE::Widget::WM_Rotate);
}

bool UE2GridEdMode::UsesPropertyWidgets() const
{
	return false;
}

EAxisList::Type UE2GridEdMode::GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const
{
	if (!CanUseTransformWidget())
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
	if (!CanUseTransformWidget())
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

bool UE2GridEdMode::CanUseTransformWidget() const
{
	return IsGridPageActive() && Settings != nullptr;
}

bool UE2GridEdMode::CanShowPreview() const
{
	return IsGridPageActive() && Settings && Settings->bShowPreview && Settings->IsValid();
}

FTransform UE2GridEdMode::GetPreviewTransform() const
{
	const FVector PreviewScale = ActiveGridManager.IsValid()
		? ActiveGridManager->GetActorScale3D()
		: FVector::OneVector;
	return Settings
		? FTransform(Settings->Rotation, Settings->Location, PreviewScale)
		: FTransform::Identity;
}

void UE2GridEdMode::ClearSelectedGrid()
{
	SelectedGridCoord.Reset();
	if (GridSettings)
	{
		GridSettings->Reset();
	}
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
		HoveredGridCoord.Reset();
		ClearSelectedGrid();
		if (GEditor)
		{
			GEditor->RedrawAllViewports(false);
		}
	}
}

bool UE2GridEdMode::CanActivateTool(EE2GridEdModeTool InTool) const
{
	return InTool == EE2GridEdModeTool::New || GridManagers.ContainsByPredicate(
		[](const TWeakObjectPtr<AE2GridManager>& GridManager)
		{
			return GridManager.IsValid();
		});
}

void UE2GridEdMode::SetActiveTool(EE2GridEdModeTool InTool)
{
	if (ActiveTool == InTool || !CanActivateTool(InTool) || !ResolvePendingSettings())
	{
		return;
	}

	ActiveTool = InTool;
	bSettingsDirty = false;
	ClearSelectedGrid();
	RefreshGridManagers();
}

void UE2GridEdMode::NotifySettingsChanged(bool bRefreshDetails)
{
	HoveredGridCoord.Reset();
	bSettingsDirty = IsCreatingGridManager() || (Settings && ActiveGridManager.IsValid() &&
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
	HoveredGridCoord.Reset();
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
	if (ActiveTool == EE2GridEdModeTool::Edit && GridManagers.IsEmpty())
	{
		ActiveTool = EE2GridEdModeTool::New;
	}

	const bool bPreviousManagerStillExists = PreviousGridManager && GridManagers.ContainsByPredicate(
		[PreviousGridManager](const TWeakObjectPtr<AE2GridManager>& GridManager)
		{
			return GridManager.Get() == PreviousGridManager;
		});

	AE2GridManager* NewGridManager = ActiveTool == EE2GridEdModeTool::Edit && bPreviousManagerStillExists
		? PreviousGridManager
		: nullptr;
	if (ActiveTool == EE2GridEdModeTool::Edit && !NewGridManager)
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
	if (ActiveTool == EE2GridEdModeTool::Edit && !NewGridManager && !GridManagers.IsEmpty())
	{
		NewGridManager = GridManagers[0].Get();
	}

	const bool bTargetChanged = ActiveGridManager.Get() != NewGridManager;
	ActiveGridManager = NewGridManager;
	if (bTargetChanged || !NewGridManager)
	{
		ClearSelectedGrid();
	}
	if (Settings && NewGridManager && (bTargetChanged || !bSettingsDirty))
	{
		Settings->LoadFromGridManager(*NewGridManager);
		bSettingsDirty = false;
	}
	else if (Settings && !NewGridManager && !bKeepCreateDraft)
	{
		Settings->ResetToDefaults();
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
	const bool bToolChanged = InGridManager && ActiveTool != EE2GridEdModeTool::Edit;
	if (ActiveGridManager.Get() == InGridManager && !bToolChanged)
	{
		return;
	}

	if (InGridManager)
	{
		ActiveTool = EE2GridEdModeTool::Edit;
	}
	HoveredGridCoord.Reset();
	ClearSelectedGrid();
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
		(ActiveGridManager.IsValid() && !Settings->MatchesGridManager(*ActiveGridManager)));
}

bool UE2GridEdMode::ResolvePendingSettings()
{
	if (!HasPendingSettings())
	{
		return true;
	}

	const EAppReturnType::Type Response = FMessageDialog::Open(
		EAppMsgType::YesNoCancel,
		LOCTEXT(
			"PendingGridSettings",
			"The current grid settings have uncommitted changes.\n\n"
			"Yes: Commit changes\nNo: Discard changes\nCancel: Keep editing"));
	if (Response == EAppReturnType::Cancel)
	{
		return false;
	}
	if (Response == EAppReturnType::Yes)
	{
		return CommitSettings();
	}

	RevertSettings();
	return true;
}

bool UE2GridEdMode::CanCommitSettings() const
{
	if (!GEditor || !Settings || !Settings->IsValid())
	{
		return false;
	}

	if (!IsCreatingGridManager())
	{
		return ActiveGridManager.IsValid() && HasPendingSettings();
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
		GridManager->Generate();
		GridManager->MarkPackageDirty();

		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(GridManager, true, true, true);
		ActiveTool = EE2GridEdModeTool::Edit;
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
		Settings->GridSize != GridManager->GridSize;

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
	if (bGridChanged)
	{
		GridManager->Generate();
		ClearSelectedGrid();
	}

	GridManager->MarkPackageDirty();
	Settings->LoadFromGridManager(*GridManager);
	HoveredGridCoord.Reset();
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
	HoveredGridCoord.Reset();
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

