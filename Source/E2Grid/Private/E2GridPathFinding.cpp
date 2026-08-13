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

	float OctileDistance(const FE2GridCoord& From, const FE2GridCoord& To)
	{
		const float DeltaX = static_cast<float>(FMath::Abs(From.X - To.X));
		const float DeltaY = static_cast<float>(FMath::Abs(From.Y - To.Y));
		return FMath::Max(DeltaX, DeltaY) + (UE_SQRT_2 - 1.0f) * FMath::Min(DeltaX, DeltaY);
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
	OutResult.Reset();

	const FE2GridCellData* GoalCell = Manager.FindCell(GoalCellKey);
	if (!GoalCell || !GoalCell->CanStandOn())
	{
		OutResult.Status = EE2GridPathStatus::InvalidGoal;
		return false;
	}
	if (const TWeakObjectPtr<UE2GridUnitComponent>* GoalOwner = CellOwners.Find(GoalCellKey);
		GoalOwner && GoalOwner->IsValid() && GoalOwner->Get() != RequestingUnit)
	{
		OutResult.Status = EE2GridPathStatus::GoalOccupied;
		return false;
	}
	if (StartCellKey == GoalCellKey)
	{
		OutResult.Status = EE2GridPathStatus::Success;
		return true;
	}

	const FE2GridMapLayout* Layout = Manager.GetLayout();
	FE2GridCoord StartCoord;
	FE2GridCoord GoalCoord;
	if (!Layout || !Layout->KeyToCoord(StartCellKey, StartCoord) || !Layout->KeyToCoord(GoalCellKey, GoalCoord))
	{
		OutResult.Status = EE2GridPathStatus::InvalidUnit;
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
			OutResult.Status = EE2GridPathStatus::Success;
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

				const bool bIsGoal = NeighborKey == GoalCellKey;
				const FE2GridCellData* NeighborCell = Manager.FindCell(NeighborKey);
				if (!NeighborCell || (bIsGoal ? !NeighborCell->CanStandOn() : !NeighborCell->CanWalkThrough()))
				{
					return;
				}
				if (const TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwners.Find(NeighborKey);
					Owner && Owner->IsValid() && Owner->Get() != RequestingUnit)
				{
					return;
				}

				const float NewCost = CurrentCost + StepCost;
				if (NewCost >= NeighborRecord.CostFromStart)
				{
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

	OutResult.Status = EE2GridPathStatus::NoPath;
	return false;
}
