#include "E2GridPathFinding.h"

#include "Algo/Reverse.h"
#include "E2GridManager.h"
#include "E2GridUnitComponent.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

DEFINE_LOG_CATEGORY_STATIC(LogE2GridPathFinding, Log, All);

namespace
{
	struct FNodeRecord
	{
		float CostFromStart = TNumericLimits<float>::Max();
		float EstimatedTotalCost = TNumericLimits<float>::Max();
		int32 ParentKey = INVALID_GRID_KEY;
		bool bClosed = false;
	};

	struct FCostEntry
	{
		int32 CellKey = INVALID_GRID_KEY;
		float Cost = TNumericLimits<float>::Max();
	};

	struct FOpenEntry
	{
		int32 CellKey = INVALID_GRID_KEY;
		float EstimatedTotalCost = TNumericLimits<float>::Max();
	};

	struct FOpenEntryPredicate
	{
		bool operator()(const FOpenEntry& Left, const FOpenEntry& Right) const
		{
			if (FMath::IsNearlyEqual(Left.EstimatedTotalCost, Right.EstimatedTotalCost))
			{
				return Left.CellKey < Right.CellKey;
			}
			return Left.EstimatedTotalCost < Right.EstimatedTotalCost;
		}
	};

	struct FCostEntryPredicate
	{
		bool operator()(const FCostEntry& Left, const FCostEntry& Right) const
		{
			if (FMath::IsNearlyEqual(Left.Cost, Right.Cost))
			{
				return Left.CellKey < Right.CellKey;
			}
			return Left.Cost < Right.Cost;
		}
	};

	float OctileDistance(const FE2GridCoord& From, const FE2GridCoord& To)
	{
		const float DeltaX = static_cast<float>(FMath::Abs(From.X - To.X));
		const float DeltaY = static_cast<float>(FMath::Abs(From.Y - To.Y));
		return FMath::Max(DeltaX, DeltaY) + (UE_SQRT_2 - 1.0f) * FMath::Min(DeltaX, DeltaY);
	}

	bool IsOccupiedByOther(
		const TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>>& CellOwners,
		const UE2GridUnitComponent* RequestingUnit,
		int32 CellKey)
	{
		const TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwners.Find(CellKey);
		return Owner && Owner->IsValid() && Owner->Get() != RequestingUnit;
	}

	bool CanTraverseCell(
		const AE2GridManager& Manager,
		const TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>>& CellOwners,
		const UE2GridUnitComponent* RequestingUnit,
		int32 CellKey)
	{
		const FE2GridCellData* Cell = Manager.FindCell(CellKey);
		return Cell && Cell->CanWalkThrough() &&
			!IsOccupiedByOther(CellOwners, RequestingUnit, CellKey);
	}

	bool IsBetterCost(float NewCost, float ExistingCost)
	{
		return NewCost < ExistingCost && !FMath::IsNearlyEqual(NewCost, ExistingCost);
	}

	bool IsStableParentImprovement(float NewCost, int32 NewParent, const FNodeRecord& Existing)
	{
		return FMath::IsNearlyEqual(NewCost, Existing.CostFromStart) &&
			(Existing.ParentKey == INVALID_GRID_KEY || NewParent < Existing.ParentKey);
	}
}

bool FE2GridPathFinding::FindPath(
	const AE2GridManager& Manager,
	const TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>>& CellOwners,
	const UE2GridUnitComponent* RequestingUnit,
	int32 StartCellKey,
	int32 GoalCellKey,
	FE2GridPathResult& OutResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_AStar);
	const double StartTime = FPlatformTime::Seconds();
	OutResult.Steps.Reset();
	OutResult.TotalCost = 0.0f;

	const FE2GridCellData* GoalCell = Manager.FindCell(GoalCellKey);
	if (!GoalCell)
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::InvalidCell);
		return false;
	}
	if (!GoalCell->CanStandOn())
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::NotStandable);
		return false;
	}
	if (IsOccupiedByOther(CellOwners, RequestingUnit, GoalCellKey))
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::Occupied);
		return false;
	}
	if (StartCellKey == GoalCellKey)
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::Success);
		return true;
	}

	const FE2GridMapLayout* Layout = Manager.GetLayout();
	FE2GridCoord StartCoord;
	FE2GridCoord GoalCoord;
	if (!Layout || !Layout->KeyToCoord(StartCellKey, StartCoord) || !Layout->KeyToCoord(GoalCellKey, GoalCoord))
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::InvalidUnit);
		return false;
	}

	TMap<int32, FNodeRecord> Records;
	TArray<FOpenEntry> OpenHeap;
	FNodeRecord& StartRecord = Records.Add(StartCellKey);
	StartRecord.CostFromStart = 0.0f;
	StartRecord.EstimatedTotalCost = OctileDistance(StartCoord, GoalCoord);
	OpenHeap.HeapPush(FOpenEntry{StartCellKey, StartRecord.EstimatedTotalCost}, FOpenEntryPredicate());
	int32 ExpandedNodes = 0;

	while (!OpenHeap.IsEmpty())
	{
		FOpenEntry CurrentEntry;
		OpenHeap.HeapPop(CurrentEntry, FOpenEntryPredicate(), EAllowShrinking::No);
		FNodeRecord* CurrentRecord = Records.Find(CurrentEntry.CellKey);
		if (!CurrentRecord || CurrentRecord->bClosed ||
			!FMath::IsNearlyEqual(CurrentRecord->EstimatedTotalCost, CurrentEntry.EstimatedTotalCost))
		{
			continue;
		}

		if (CurrentEntry.CellKey == GoalCellKey)
		{
			int32 PathKey = GoalCellKey;
			while (PathKey != StartCellKey)
			{
				OutResult.Steps.Add(FE2GridPathStep{PathKey});
				PathKey = Records.FindChecked(PathKey).ParentKey;
			}
			Algo::Reverse(OutResult.Steps);
			OutResult.TotalCost = CurrentRecord->CostFromStart;
			OutResult.SetQueryStatus(EE2GridQueryStatus::Success);
			UE_LOG(
				LogE2GridPathFinding,
				VeryVerbose,
				TEXT("A* succeeded in %.3f ms after expanding %d nodes."),
				(FPlatformTime::Seconds() - StartTime) * 1000.0,
				ExpandedNodes);
			return true;
		}

		CurrentRecord->bClosed = true;
		++ExpandedNodes;
		const float CurrentCost = CurrentRecord->CostFromStart;
		const int32 CurrentKey = CurrentEntry.CellKey;
		Manager.ForEachTraversableNeighbor(
			CurrentKey,
			[&](int32 NeighborKey, float StepCost)
			{
				FNodeRecord& NeighborRecord = Records.FindOrAdd(NeighborKey);
				if (NeighborRecord.bClosed)
				{
					return;
				}

				if (!CanTraverseCell(Manager, CellOwners, RequestingUnit, NeighborKey))
				{
					return;
				}

				const float NewCost = CurrentCost + StepCost;
				if (!IsBetterCost(NewCost, NeighborRecord.CostFromStart) &&
					!IsStableParentImprovement(NewCost, CurrentKey, NeighborRecord))
				{
					return;
				}
				if (FMath::IsNearlyEqual(NewCost, NeighborRecord.CostFromStart))
				{
					NeighborRecord.ParentKey = CurrentKey;
					return;
				}

				FE2GridCoord NeighborCoord;
				if (!Layout->KeyToCoord(NeighborKey, NeighborCoord))
				{
					return;
				}
				NeighborRecord.CostFromStart = NewCost;
				NeighborRecord.EstimatedTotalCost = NewCost + OctileDistance(NeighborCoord, GoalCoord);
				NeighborRecord.ParentKey = CurrentKey;
				OpenHeap.HeapPush(
					FOpenEntry{NeighborKey, NeighborRecord.EstimatedTotalCost},
					FOpenEntryPredicate());
			});
	}

	OutResult.SetQueryStatus(EE2GridQueryStatus::NoPath);
	UE_LOG(
		LogE2GridPathFinding,
		VeryVerbose,
		TEXT("A* found no path in %.3f ms after expanding %d nodes."),
		(FPlatformTime::Seconds() - StartTime) * 1000.0,
		ExpandedNodes);
	return false;
}

bool FE2GridPathFinding::FindReachableCells(
	const AE2GridManager& Manager,
	const TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>>& CellOwners,
	const UE2GridUnitComponent* RequestingUnit,
	int32 StartCellKey,
	float MovementBudget,
	FE2GridReachableResult& OutResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_ReachableDijkstra);
	const double StartTime = FPlatformTime::Seconds();
	OutResult.Cells.Reset();
	OutResult.TraversalTree.Reset();

	TMap<int32, FNodeRecord> Records;
	TArray<FCostEntry> OpenHeap;
	FNodeRecord& StartRecord = Records.Add(StartCellKey);
	StartRecord.CostFromStart = 0.0f;
	OpenHeap.HeapPush(FCostEntry{StartCellKey, 0.0f}, FCostEntryPredicate());
	int32 ExpandedNodes = 0;

	while (!OpenHeap.IsEmpty())
	{
		FCostEntry CurrentEntry;
		OpenHeap.HeapPop(CurrentEntry, FCostEntryPredicate(), EAllowShrinking::No);
		FNodeRecord* CurrentRecord = Records.Find(CurrentEntry.CellKey);
		if (!CurrentRecord || CurrentRecord->bClosed ||
			!FMath::IsNearlyEqual(CurrentRecord->CostFromStart, CurrentEntry.Cost))
		{
			continue;
		}

		CurrentRecord->bClosed = true;
		++ExpandedNodes;
		const float CurrentCost = CurrentRecord->CostFromStart;
		const int32 CurrentKey = CurrentEntry.CellKey;
		Manager.ForEachTraversableNeighbor(
			CurrentKey,
			[&](int32 NeighborKey, float StepCost)
			{
				if (!CanTraverseCell(Manager, CellOwners, RequestingUnit, NeighborKey))
				{
					return;
				}

				const float NewCost = CurrentCost + StepCost;
				if (NewCost > MovementBudget && !FMath::IsNearlyEqual(NewCost, MovementBudget))
				{
					return;
				}

				FNodeRecord& NeighborRecord = Records.FindOrAdd(NeighborKey);
				if (NeighborRecord.bClosed ||
					(!IsBetterCost(NewCost, NeighborRecord.CostFromStart) &&
						!IsStableParentImprovement(NewCost, CurrentKey, NeighborRecord)))
				{
					return;
				}
				if (FMath::IsNearlyEqual(NewCost, NeighborRecord.CostFromStart))
				{
					NeighborRecord.ParentKey = CurrentKey;
					return;
				}

				NeighborRecord.CostFromStart = NewCost;
				NeighborRecord.ParentKey = CurrentKey;
				OpenHeap.HeapPush(FCostEntry{NeighborKey, NewCost}, FCostEntryPredicate());
			});
	}

	for (const TPair<int32, FNodeRecord>& Pair : Records)
	{
		if (!FMath::IsFinite(Pair.Value.CostFromStart))
		{
			continue;
		}
		const FE2GridReachableCell ReachableCell{
			Pair.Key,
			Pair.Value.CostFromStart,
			Pair.Value.ParentKey};
		OutResult.TraversalTree.Add(ReachableCell);
		const FE2GridCellData* Cell = Manager.FindCell(Pair.Key);
		if (Pair.Key == StartCellKey || (Cell && Cell->CanStandOn()))
		{
			OutResult.Cells.Add(ReachableCell);
		}
	}

	const auto StableCostOrder = [](const FE2GridReachableCell& Left, const FE2GridReachableCell& Right)
	{
		return FMath::IsNearlyEqual(Left.Cost, Right.Cost)
			? Left.CellKey < Right.CellKey
			: Left.Cost < Right.Cost;
	};
	OutResult.TraversalTree.Sort(StableCostOrder);
	OutResult.Cells.Sort(StableCostOrder);
	OutResult.Status = EE2GridQueryStatus::Success;
	UE_LOG(
		LogE2GridPathFinding,
		VeryVerbose,
		TEXT("Reachable query returned %d standing cells in %.3f ms after expanding %d nodes."),
		OutResult.Cells.Num(),
		(FPlatformTime::Seconds() - StartTime) * 1000.0,
		ExpandedNodes);
	return true;
}

bool FE2GridPathFinding::BuildPathFromReachableResult(
	const FE2GridReachableResult& ReachableResult,
	int32 GoalCellKey,
	FE2GridPathResult& OutResult)
{
	OutResult.Reset(EE2GridQueryStatus::InvalidRequest);
	OutResult.StartCellKey = ReachableResult.StartCellKey;
	OutResult.GoalCellKey = GoalCellKey;
	OutResult.RuntimeRevision = ReachableResult.RuntimeRevision;
	if (ReachableResult.Status != EE2GridQueryStatus::Success)
	{
		OutResult.SetQueryStatus(ReachableResult.Status);
		return false;
	}

	const FE2GridReachableCell* GoalCell = ReachableResult.Cells.FindByPredicate(
		[GoalCellKey](const FE2GridReachableCell& Cell)
		{
			return Cell.CellKey == GoalCellKey;
		});
	if (!GoalCell)
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::InvalidCell);
		return false;
	}

	int32 PathKey = GoalCellKey;
	int32 RemainingNodes = ReachableResult.TraversalTree.Num() + 1;
	while (PathKey != ReachableResult.StartCellKey && RemainingNodes-- > 0)
	{
		const FE2GridReachableCell* PathCell = ReachableResult.TraversalTree.FindByPredicate(
			[PathKey](const FE2GridReachableCell& Cell)
			{
				return Cell.CellKey == PathKey;
			});
		if (!PathCell || PathCell->ParentCellKey == INVALID_GRID_KEY)
		{
			OutResult.Reset(EE2GridQueryStatus::InvalidRequest);
			OutResult.StartCellKey = ReachableResult.StartCellKey;
			OutResult.GoalCellKey = GoalCellKey;
			OutResult.RuntimeRevision = ReachableResult.RuntimeRevision;
			return false;
		}
		OutResult.Steps.Add(FE2GridPathStep{PathKey});
		PathKey = PathCell->ParentCellKey;
	}
	if (PathKey != ReachableResult.StartCellKey)
	{
		OutResult.Reset(EE2GridQueryStatus::InvalidRequest);
		OutResult.StartCellKey = ReachableResult.StartCellKey;
		OutResult.GoalCellKey = GoalCellKey;
		OutResult.RuntimeRevision = ReachableResult.RuntimeRevision;
		return false;
	}

	Algo::Reverse(OutResult.Steps);
	OutResult.TotalCost = GoalCell->Cost;
	OutResult.SetQueryStatus(EE2GridQueryStatus::Success);
	return true;
}

bool FE2GridPathFinding::FindCellsInRange(
	const AE2GridManager& Manager,
	int32 StartCellKey,
	float MaxRange,
	EE2GridRangeMetric Metric,
	FE2GridRangeResult& OutResult)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_TopologyRange);
	const double StartTime = FPlatformTime::Seconds();
	TMap<int32, float> Distances;
	TSet<int32> Closed;
	TArray<FCostEntry> OpenHeap;
	Distances.Add(StartCellKey, 0.0f);
	OpenHeap.HeapPush(FCostEntry{StartCellKey, 0.0f}, FCostEntryPredicate());
	int32 ExpandedNodes = 0;

	while (!OpenHeap.IsEmpty())
	{
		FCostEntry CurrentEntry;
		OpenHeap.HeapPop(CurrentEntry, FCostEntryPredicate(), EAllowShrinking::No);
		const float* CurrentDistance = Distances.Find(CurrentEntry.CellKey);
		if (!CurrentDistance || Closed.Contains(CurrentEntry.CellKey) ||
			!FMath::IsNearlyEqual(*CurrentDistance, CurrentEntry.Cost))
		{
			continue;
		}
		Closed.Add(CurrentEntry.CellKey);
		++ExpandedNodes;
		Manager.ForEachTraversableNeighbor(
			CurrentEntry.CellKey,
			[&](int32 NeighborKey, float TraversalCost)
			{
				const float StepCost = Metric == EE2GridRangeMetric::StepCount ? 1.0f : TraversalCost;
				const float NewDistance = CurrentEntry.Cost + StepCost;
				if (NewDistance > MaxRange && !FMath::IsNearlyEqual(NewDistance, MaxRange))
				{
					return;
				}
				const float* ExistingDistance = Distances.Find(NeighborKey);
				if (ExistingDistance && !IsBetterCost(NewDistance, *ExistingDistance))
				{
					return;
				}
				Distances.Add(NeighborKey, NewDistance);
				OpenHeap.HeapPush(FCostEntry{NeighborKey, NewDistance}, FCostEntryPredicate());
			});
	}

	OutResult.Cells.Reset(Distances.Num());
	for (const TPair<int32, float>& Pair : Distances)
	{
		OutResult.Cells.Add(FE2GridRangeCell{Pair.Key, Pair.Value});
	}
	OutResult.Cells.Sort([](const FE2GridRangeCell& Left, const FE2GridRangeCell& Right)
	{
		return FMath::IsNearlyEqual(Left.Distance, Right.Distance)
			? Left.CellKey < Right.CellKey
			: Left.Distance < Right.Distance;
	});
	OutResult.Status = EE2GridQueryStatus::Success;
	UE_LOG(
		LogE2GridPathFinding,
		VeryVerbose,
		TEXT("Range query returned %d cells in %.3f ms after expanding %d nodes."),
		OutResult.Cells.Num(),
		(FPlatformTime::Seconds() - StartTime) * 1000.0,
		ExpandedNodes);
	return true;
}
