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
	TSet<int32> OpenCells;
	TSet<int32> ClosedCells;
	FNodeRecord& StartRecord = Records.Add(StartCellKey);
	StartRecord.CostFromStart = 0.0f;
	StartRecord.EstimatedTotalCost = OctileDistance(StartCoord, GoalCoord);
	OpenCells.Add(StartCellKey);
	int32 ExpandedNodes = 0;

	while (!OpenCells.IsEmpty())
	{
		int32 CurrentKey = INVALID_GRID_KEY;
		float BestEstimatedCost = TNumericLimits<float>::Max();
		for (int32 CandidateKey : OpenCells)
		{
			const FNodeRecord& Candidate = Records.FindChecked(CandidateKey);
			if (Candidate.EstimatedTotalCost < BestEstimatedCost ||
				(FMath::IsNearlyEqual(Candidate.EstimatedTotalCost, BestEstimatedCost) &&
					(CurrentKey == INVALID_GRID_KEY || CandidateKey < CurrentKey)))
			{
				CurrentKey = CandidateKey;
				BestEstimatedCost = Candidate.EstimatedTotalCost;
			}
		}

		if (CurrentKey == GoalCellKey)
		{
			int32 PathKey = GoalCellKey;
			while (PathKey != StartCellKey)
			{
				OutResult.CellKeys.Add(PathKey);
				PathKey = Records.FindChecked(PathKey).ParentKey;
			}
			Algo::Reverse(OutResult.CellKeys);
			OutResult.TotalCost = Records.FindChecked(GoalCellKey).CostFromStart;
			OutResult.Status = EE2GridPathStatus::Success;
			UE_LOG(
				LogE2GridPathFinding,
				VeryVerbose,
				TEXT("A* succeeded in %.3f ms after expanding %d nodes."),
				(FPlatformTime::Seconds() - StartTime) * 1000.0,
				ExpandedNodes);
			return true;
		}

		OpenCells.Remove(CurrentKey);
		ClosedCells.Add(CurrentKey);
		++ExpandedNodes;

		const FE2GridCellData* CurrentCell = Manager.FindCell(CurrentKey);
		FE2GridCoord CurrentCoord;
		if (!CurrentCell || !Layout->KeyToCoord(CurrentKey, CurrentCoord))
		{
			continue;
		}

		for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
		{
			if ((CurrentCell->NeighborMask & (1u << DirectionIndex)) == 0)
			{
				continue;
			}

			const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
			const FE2GridCoord NeighborCoord(CurrentCoord.X + Offset.X, CurrentCoord.Y + Offset.Y);
			const int32 NeighborKey = Layout->CoordToKey(NeighborCoord);
			if (NeighborKey == INVALID_GRID_KEY || ClosedCells.Contains(NeighborKey))
			{
				continue;
			}

			const FE2GridCellData* NeighborCell = Manager.FindCell(NeighborKey);
			const bool bIsGoal = NeighborKey == GoalCellKey;
			if (!NeighborCell || (bIsGoal ? !NeighborCell->CanStandOn() : !NeighborCell->CanWalkThrough()))
			{
				continue;
			}
			if (const TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwners.Find(NeighborKey);
				Owner && Owner->IsValid() && Owner->Get() != RequestingUnit)
			{
				continue;
			}

			const float StepCost = E2GridDirections::IsDiagonal(DirectionIndex) ? UE_SQRT_2 : 1.0f;
			const float NewCost = Records.FindChecked(CurrentKey).CostFromStart + StepCost;
			FNodeRecord& NeighborRecord = Records.FindOrAdd(NeighborKey);
			if (NewCost >= NeighborRecord.CostFromStart)
			{
				continue;
			}

			NeighborRecord.CostFromStart = NewCost;
			NeighborRecord.EstimatedTotalCost = NewCost + OctileDistance(NeighborCoord, GoalCoord);
			NeighborRecord.ParentKey = CurrentKey;
			OpenCells.Add(NeighborKey);
		}
	}

	OutResult.Status = EE2GridPathStatus::NoPath;
	UE_LOG(
		LogE2GridPathFinding,
		VeryVerbose,
		TEXT("A* found no path in %.3f ms after expanding %d nodes."),
		(FPlatformTime::Seconds() - StartTime) * 1000.0,
		ExpandedNodes);
	return false;
}
