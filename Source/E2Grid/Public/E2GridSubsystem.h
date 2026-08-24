#pragma once

#include "CoreMinimal.h"
#include "E2GridRuntimeData.h"
#include "Subsystems/WorldSubsystem.h"
#include "E2GridSubsystem.generated.h"

class AE2GridManager;
class UE2GridUnitComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FE2GridStateChanged,
	const FE2GridStateDelta&, Delta);

UCLASS()
class E2GRID_API UE2GridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	bool RegisterManager(AE2GridManager* Manager);
	void UnregisterManager(AE2GridManager* Manager);

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	AE2GridManager* GetActiveManager() const { return ActiveManager.Get(); }

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	int64 GetRuntimeRevision() const { return RuntimeRevision; }

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	EE2GridRegistrationStatus RegisterUnit(UE2GridUnitComponent* Unit);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	void UnregisterUnit(UE2GridUnitComponent* Unit);

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool WorldToCell(const FVector& WorldPosition, int32& OutCellKey) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool CellToWorld(int32 CellKey, FVector& OutWorldPosition) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool GetCell(int32 CellKey, FE2GridCellData& OutCellData) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	UE2GridUnitComponent* GetCellOwner(int32 CellKey) const;

	/** Side-effect-free placement validation against the current runtime snapshot. */
	UFUNCTION(BlueprintPure, Category = "E2Grid")
	FE2GridPlacementResult QueryPlacement(
		const UE2GridUnitComponent* Unit,
		int32 CellKey) const;

	/** Legacy bool wrapper around QueryPlacement. */
	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool CanPlaceUnit(const UE2GridUnitComponent* Unit, int32 CellKey) const;

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool CommitUnitMove(
		UE2GridUnitComponent* Unit,
		const TArray<FE2GridPathStep>& TraversedPath);

	/** Validates the full path and expected revision without changing state. */
	UFUNCTION(BlueprintPure, Category = "E2Grid")
	EE2GridQueryStatus ValidatePath(
		const UE2GridUnitComponent* Unit,
		const FE2GridPathResult& PathResult) const;

	/** Atomically commits a versioned path result and reports a structured failure reason. */
	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool CommitUnitMoveFromPath(
		UE2GridUnitComponent* Unit,
		const FE2GridPathResult& PathResult,
		FE2GridMoveCommitResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool FindPath(
		const UE2GridUnitComponent* RequestingUnit,
		int32 GoalCellKey,
		FE2GridPathResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool FindReachableCells(
		const UE2GridUnitComponent* RequestingUnit,
		float MovementBudget,
		FE2GridReachableResult& OutResult) const;

	/** Reconstructs one shortest path from a Reachable result without rerunning A*. */
	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool BuildPathFromReachableResult(
		const FE2GridReachableResult& ReachableResult,
		int32 GoalCellKey,
		FE2GridPathResult& OutResult) const;

	/** Gameplay-agnostic topology range. Occupancy and teams do not affect this query. */
	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool FindCellsInRange(
		int32 StartCellKey,
		float MaxRange,
		EE2GridRangeMetric Metric,
		FE2GridRangeResult& OutResult) const;

	bool IsUnitRegistered(const UE2GridUnitComponent* Unit) const;

	UPROPERTY(BlueprintAssignable, Category = "E2Grid")
	FE2GridStateChanged OnStateChanged;

private:
	EE2GridQueryStatus ValidatePathForCommit(
		const UE2GridUnitComponent* Unit,
		const FE2GridPathResult& PathResult,
		int32& OutGoalCellKey) const;
	void AdvanceRevision(
		EE2GridStateChangeKind ChangeKind,
		UE2GridUnitComponent* Unit = nullptr,
		int32 FromCellKey = INVALID_GRID_KEY,
		int32 ToCellKey = INVALID_GRID_KEY);
	void RetryPendingUnits();
	void CompactWeakState();

	UPROPERTY(Transient)
	int64 RuntimeRevision = 0;

	TWeakObjectPtr<AE2GridManager> ActiveManager;
	TSet<TWeakObjectPtr<UE2GridUnitComponent>> RegisteredUnits;
	TSet<TWeakObjectPtr<UE2GridUnitComponent>> PendingUnits;
	TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>> CellOwnersByKey;
};
