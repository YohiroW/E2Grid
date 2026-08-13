#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "E2GridRuntimeData.h"
#include "E2GridManager.generated.h"

class UE2GridMapAsset;
class UE2GridVisualizeComponent;

UCLASS()
class E2GRID_API AE2GridManager : public AActor
{
	GENERATED_BODY()

public:
	AE2GridManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const FE2GridMapLayout* GetLayout() const;
	const TMap<int32, FE2GridCellData>* GetCells() const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool HasValidGrid() const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool IsValidGridKey(int32 CellKey) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool IsValidGridCoord(const FE2GridCoord& Coord) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	int32 GetGridKey(const FE2GridCoord& Coord) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool GetCoordByKey(int32 CellKey, FE2GridCoord& OutCoord) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool TryGetCellData(int32 CellKey, FE2GridCellData& OutCellData) const;

	const FE2GridCellData* FindCell(int32 CellKey) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool WorldToCell(const FVector& WorldPosition, int32& OutCellKey) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool CellToWorld(int32 CellKey, FVector& OutWorldPosition) const;

	FVector GetCellWorldCenterChecked(int32 CellKey) const;

	void ForEachCell(
		TFunctionRef<void(int32 CellKey, const FE2GridCellData& Cell, const FVector& WorldPosition)> Visitor) const;

	void ForEachTraversableNeighbor(
		int32 FromCellKey,
		TFunctionRef<void(int32 ToCellKey, float Cost)> Visitor) const;

#if WITH_EDITOR
	void RefreshVisualization();
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UE2GridMapAsset> GridMapAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bShowVisualizedGrid = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
	TObjectPtr<UE2GridVisualizeComponent> GridVisualizeComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Build")
	FE2GridBuildSettings BuildSettings;
#endif
};
