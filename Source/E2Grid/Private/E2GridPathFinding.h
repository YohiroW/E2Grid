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
};
