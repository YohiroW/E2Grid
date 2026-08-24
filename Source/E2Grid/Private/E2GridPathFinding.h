#pragma once

#include "CoreMinimal.h"
#include "E2GridRuntimeData.h"

class AE2GridManager;
class UE2GridUnitComponent;

class FE2GridPathFinding
{
public:
	static bool FindPath(
		const AE2GridManager& Manager,
		const TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>>& CellOwners,
		const UE2GridUnitComponent* RequestingUnit,
		int32 StartCellKey,
		int32 GoalCellKey,
		FE2GridPathResult& OutResult);

	static bool FindReachableCells(
		const AE2GridManager& Manager,
		const TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>>& CellOwners,
		const UE2GridUnitComponent* RequestingUnit,
		int32 StartCellKey,
		float MovementBudget,
		FE2GridReachableResult& OutResult);

	static bool BuildPathFromReachableResult(
		const FE2GridReachableResult& ReachableResult,
		int32 GoalCellKey,
		FE2GridPathResult& OutResult);

	static bool FindCellsInRange(
		const AE2GridManager& Manager,
		int32 StartCellKey,
		float MaxRange,
		EE2GridRangeMetric Metric,
		FE2GridRangeResult& OutResult);
};
