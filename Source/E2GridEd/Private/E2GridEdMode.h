#pragma once

#include "EditorModeManager.h"
#include "Tools/LegacyEdModeWidgetHelpers.h"
#include "E2GridEdMode.generated.h"

class UE2GridEdModeSettings;
class AE2GridManager;
class AActor;

enum class EE2GridEdModePage : uint8
{
	Grid,
	Bake,
	Debug
};

enum class EE2GridEdModeTool : uint8
{
	New,
	Edit
};

UCLASS()
class UE2GridEdMode : public UBaseLegacyWidgetEdMode
{
public:
	GENERATED_BODY()

	static const FEditorModeID EM_E2GridEdModeId;

	UE2GridEdMode();
	virtual ~UE2GridEdMode();

	virtual void Initialize() override;
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void CreateToolkit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y) override;
	virtual bool MouseLeave(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesTransformWidget() const override;
	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override;
	virtual bool UsesPropertyWidgets() const override;
	virtual EAxisList::Type GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const override;
	virtual FVector GetWidgetLocation() const override;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	virtual bool GetCustomInputCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

	UE2GridEdModeSettings* GetSettings() const { return Settings; }
	const TArray<TWeakObjectPtr<AE2GridManager>>& GetGridManagers() const { return GridManagers; }
	AE2GridManager* GetActiveGridManager() const { return ActiveGridManager.Get(); }
	EE2GridEdModeTool GetActiveTool() const { return ActiveTool; }
	bool IsToolActive(EE2GridEdModeTool InTool) const { return ActiveTool == InTool; }
	bool CanActivateTool(EE2GridEdModeTool InTool) const;
	bool IsCreatingGridManager() const { return IsToolActive(EE2GridEdModeTool::New); }
	void SetActivePage(EE2GridEdModePage InPage);
	void SetActiveTool(EE2GridEdModeTool InTool);
	void NotifySettingsChanged(bool bRefreshDetails);
	void RefreshGridManagers();
	void SetActiveGridManager(AE2GridManager* InGridManager);
	bool HasPendingSettings() const;
	bool ResolvePendingSettings();
	bool CanCommitSettings() const;
	bool CommitSettings();
	void RevertSettings();

protected:
	/** Binds UI commands to actions for the mesh paint mode */
	virtual void BindCommands() override;

	// UEdMode interface
	virtual void OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	virtual void OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void ElementSelectionChangeNotify() override;
	virtual void ActorPropChangeNotify() override;
	virtual void ActivateDefaultTool() override;
	virtual void UpdateOnPaletteChange(FName NewPalette);
	// end UEdMode Interface

	bool IsGridPageActive() const;
	bool CanShowPreview() const;
	bool CanUseTransformWidget() const;
	FTransform GetPreviewTransform() const;
	void HandleLevelActorAdded(AActor* InActor);
	void HandleLevelActorDeleted(AActor* InActor);

	// Start command bindings

	// End command bindings

protected:
	UPROPERTY(Transient)
	TObjectPtr<UE2GridEdModeSettings> Settings;

	TArray<TWeakObjectPtr<AE2GridManager>> GridManagers;

	TWeakObjectPtr<AE2GridManager> ActiveGridManager;

	EE2GridEdModePage ActivePage = EE2GridEdModePage::Grid;
	EE2GridEdModeTool ActiveTool = EE2GridEdModeTool::New;
	TOptional<FIntPoint> HoveredGridCoord;
	bool bSettingsDirty = false;

	FDelegateHandle PaletteChangedHandle;
	FConsoleVariableSinkHandle CVarDelegateHandle;
	FDelegateHandle LevelActorAddedHandle;
	FDelegateHandle LevelActorDeletedHandle;
};
