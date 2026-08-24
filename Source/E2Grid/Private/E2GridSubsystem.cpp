#include "E2GridSubsystem.h"

#include "E2GridManager.h"
#include "E2GridPathFinding.h"
#include "E2GridUnitComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogE2GridRuntime, Log, All);

void UE2GridSubsystem::Deinitialize()
{
	for (const TWeakObjectPtr<UE2GridUnitComponent>& Unit : RegisteredUnits)
	{
		if (Unit.IsValid())
		{
			Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::PendingManager);
		}
	}
	ActiveManager.Reset();
	RegisteredUnits.Reset();
	PendingUnits.Reset();
	CellOwnersByKey.Reset();
	RuntimeRevision = 0;
	Super::Deinitialize();
}

bool UE2GridSubsystem::RegisterManager(AE2GridManager* Manager)
{
	CompactWeakState();
	if (!IsValid(Manager) || !Manager->HasValidGrid())
	{
		UE_LOG(LogE2GridRuntime, Error, TEXT("Cannot register invalid grid manager %s."), *GetNameSafe(Manager));
		return false;
	}

	if (ActiveManager.IsValid())
	{
		if (ActiveManager.Get() == Manager)
		{
			return true;
		}
		UE_LOG(
			LogE2GridRuntime,
			Error,
			TEXT("Only one active E2Grid manager is supported. Keeping %s and rejecting %s."),
			*GetNameSafe(ActiveManager.Get()),
			*GetNameSafe(Manager));
		return false;
	}

	ActiveManager = Manager;
	AdvanceRevision(EE2GridStateChangeKind::ManagerChanged);
	RetryPendingUnits();
	return true;
}

void UE2GridSubsystem::UnregisterManager(AE2GridManager* Manager)
{
	if (ActiveManager.Get() != Manager)
	{
		return;
	}

	for (const TWeakObjectPtr<UE2GridUnitComponent>& Unit : RegisteredUnits)
	{
		if (Unit.IsValid())
		{
			Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::PendingManager);
			PendingUnits.Add(Unit);
		}
	}
	RegisteredUnits.Reset();
	CellOwnersByKey.Reset();
	ActiveManager.Reset();
	AdvanceRevision(EE2GridStateChangeKind::ManagerChanged);
}

EE2GridRegistrationStatus UE2GridSubsystem::RegisterUnit(UE2GridUnitComponent* Unit)
{
	CompactWeakState();
	if (!IsValid(Unit) || !IsValid(Unit->GetOwner()))
	{
		return EE2GridRegistrationStatus::InvalidLocation;
	}

	const TWeakObjectPtr<UE2GridUnitComponent> WeakUnit(Unit);
	if (RegisteredUnits.Contains(WeakUnit))
	{
		return EE2GridRegistrationStatus::AlreadyRegistered;
	}

	if (!ActiveManager.IsValid())
	{
		PendingUnits.Add(WeakUnit);
		Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::PendingManager);
		return EE2GridRegistrationStatus::PendingManager;
	}

	int32 CellKey = INVALID_GRID_KEY;
	if (!ActiveManager->WorldToCell(Unit->GetOwner()->GetActorLocation(), CellKey))
	{
		PendingUnits.Remove(WeakUnit);
		Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::InvalidLocation);
		return EE2GridRegistrationStatus::InvalidLocation;
	}

	const FE2GridPlacementResult Placement = QueryPlacement(Unit, CellKey);
	if (Placement.Status != EE2GridQueryStatus::Success)
	{
		PendingUnits.Remove(WeakUnit);
		const EE2GridRegistrationStatus RegistrationStatus =
			Placement.Status == EE2GridQueryStatus::Occupied
				? EE2GridRegistrationStatus::CellOccupied
				: EE2GridRegistrationStatus::CellNotStandable;
		Unit->SetPlacementState(false, INVALID_GRID_KEY, RegistrationStatus);
		return RegistrationStatus;
	}

	RegisteredUnits.Add(WeakUnit);
	PendingUnits.Remove(WeakUnit);
	CellOwnersByKey.Add(CellKey, WeakUnit);
	Unit->SetPlacementState(true, CellKey, EE2GridRegistrationStatus::Registered);
	Unit->GetOwner()->SetActorLocation(
		ActiveManager->GetCellWorldCenterChecked(CellKey),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	AdvanceRevision(
		EE2GridStateChangeKind::UnitRegistered,
		Unit,
		INVALID_GRID_KEY,
		CellKey);
	return EE2GridRegistrationStatus::Registered;
}

void UE2GridSubsystem::UnregisterUnit(UE2GridUnitComponent* Unit)
{
	if (!Unit)
	{
		return;
	}

	const TWeakObjectPtr<UE2GridUnitComponent> WeakUnit(Unit);
	PendingUnits.Remove(WeakUnit);
	const bool bWasRegistered = RegisteredUnits.Remove(WeakUnit) > 0;
	const int32 FromCellKey = Unit->GetCurrentCellKey();
	bool bRemovedOccupancy = false;
	if (TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwnersByKey.Find(FromCellKey);
		Owner && Owner->Get() == Unit)
	{
		CellOwnersByKey.Remove(FromCellKey);
		bRemovedOccupancy = true;
	}
	Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::Unregistered);
	if (bWasRegistered || bRemovedOccupancy)
	{
		AdvanceRevision(
			EE2GridStateChangeKind::UnitUnregistered,
			Unit,
			FromCellKey,
			INVALID_GRID_KEY);
	}
}

bool UE2GridSubsystem::WorldToCell(const FVector& WorldPosition, int32& OutCellKey) const
{
	return ActiveManager.IsValid() && ActiveManager->WorldToCell(WorldPosition, OutCellKey);
}

bool UE2GridSubsystem::CellToWorld(int32 CellKey, FVector& OutWorldPosition) const
{
	return ActiveManager.IsValid() && ActiveManager->CellToWorld(CellKey, OutWorldPosition);
}

bool UE2GridSubsystem::GetCell(int32 CellKey, FE2GridCellData& OutCellData) const
{
	return ActiveManager.IsValid() && ActiveManager->TryGetCellData(CellKey, OutCellData);
}

UE2GridUnitComponent* UE2GridSubsystem::GetCellOwner(int32 CellKey) const
{
	if (const TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwnersByKey.Find(CellKey);
		Owner && Owner->IsValid())
	{
		return Owner->Get();
	}
	return nullptr;
}

FE2GridPlacementResult UE2GridSubsystem::QueryPlacement(
	const UE2GridUnitComponent* Unit,
	int32 CellKey) const
{
	FE2GridPlacementResult Result;
	Result.CellKey = CellKey;
	Result.RuntimeRevision = RuntimeRevision;
	if (!ActiveManager.IsValid())
	{
		Result.Status = EE2GridQueryStatus::NoActiveGrid;
		return Result;
	}
	if (!IsValid(Unit) || !IsValid(Unit->GetOwner()))
	{
		Result.Status = EE2GridQueryStatus::InvalidUnit;
		return Result;
	}

	const FE2GridCellData* Cell = ActiveManager->FindCell(CellKey);
	if (!Cell)
	{
		Result.Status = EE2GridQueryStatus::InvalidCell;
		return Result;
	}
	if (!Cell->CanStandOn())
	{
		Result.Status = EE2GridQueryStatus::NotStandable;
		return Result;
	}
	const UE2GridUnitComponent* Owner = GetCellOwner(CellKey);
	if (Owner && Owner != Unit)
	{
		Result.Status = EE2GridQueryStatus::Occupied;
		return Result;
	}

	Result.Status = EE2GridQueryStatus::Success;
	return Result;
}

bool UE2GridSubsystem::CanPlaceUnit(const UE2GridUnitComponent* Unit, int32 CellKey) const
{
	return QueryPlacement(Unit, CellKey).Status == EE2GridQueryStatus::Success;
}

EE2GridQueryStatus UE2GridSubsystem::ValidatePathForCommit(
	const UE2GridUnitComponent* Unit,
	const FE2GridPathResult& PathResult,
	int32& OutGoalCellKey) const
{
	OutGoalCellKey = INVALID_GRID_KEY;
	if (!ActiveManager.IsValid())
	{
		return EE2GridQueryStatus::NoActiveGrid;
	}
	if (!Unit || !IsUnitRegistered(Unit))
	{
		return EE2GridQueryStatus::InvalidUnit;
	}
	if (PathResult.QueryStatus != EE2GridQueryStatus::Success)
	{
		return PathResult.QueryStatus;
	}
	if (PathResult.RuntimeRevision != RuntimeRevision)
	{
		return EE2GridQueryStatus::StaleRevision;
	}

	const int32 StartCellKey = Unit->GetCurrentCellKey();
	if (PathResult.StartCellKey != StartCellKey || PathResult.GoalCellKey == INVALID_GRID_KEY)
	{
		return EE2GridQueryStatus::InvalidRequest;
	}

	int32 FromCellKey = StartCellKey;
	float RevalidatedTotalCost = 0.0f;
	for (int32 StepIndex = 0; StepIndex < PathResult.Steps.Num(); ++StepIndex)
	{
		const int32 ToCellKey = PathResult.Steps[StepIndex].ToCellKey;
		const FE2GridCellData* Cell = ActiveManager->FindCell(ToCellKey);
		if (!Cell)
		{
			return EE2GridQueryStatus::InvalidCell;
		}

		bool bIsTraversableNeighbor = false;
		float StepCost = 0.0f;
		ActiveManager->ForEachTraversableNeighbor(
			FromCellKey,
			[ToCellKey, &bIsTraversableNeighbor, &StepCost](int32 NeighborKey, float NeighborCost)
			{
				if (NeighborKey == ToCellKey)
				{
					bIsTraversableNeighbor = true;
					StepCost = NeighborCost;
				}
			});
		if (!bIsTraversableNeighbor)
		{
			return EE2GridQueryStatus::NotTraversable;
		}

		const bool bIsGoal = StepIndex == PathResult.Steps.Num() - 1;
		if (bIsGoal ? !Cell->CanStandOn() : !Cell->CanWalkThrough())
		{
			return bIsGoal
				? EE2GridQueryStatus::NotStandable
				: EE2GridQueryStatus::NotTraversable;
		}
		const UE2GridUnitComponent* Owner = GetCellOwner(ToCellKey);
		if (Owner && Owner != Unit)
		{
			return EE2GridQueryStatus::Occupied;
		}
		RevalidatedTotalCost += StepCost;
		FromCellKey = ToCellKey;
	}

	if (FromCellKey != PathResult.GoalCellKey ||
		!FMath::IsNearlyEqual(RevalidatedTotalCost, PathResult.TotalCost))
	{
		return EE2GridQueryStatus::InvalidRequest;
	}
	const FE2GridPlacementResult Placement = QueryPlacement(Unit, FromCellKey);
	if (Placement.Status != EE2GridQueryStatus::Success)
	{
		return Placement.Status;
	}

	OutGoalCellKey = FromCellKey;
	return EE2GridQueryStatus::Success;
}

EE2GridQueryStatus UE2GridSubsystem::ValidatePath(
	const UE2GridUnitComponent* Unit,
	const FE2GridPathResult& PathResult) const
{
	int32 GoalCellKey = INVALID_GRID_KEY;
	return ValidatePathForCommit(Unit, PathResult, GoalCellKey);
}

bool UE2GridSubsystem::CommitUnitMoveFromPath(
	UE2GridUnitComponent* Unit,
	const FE2GridPathResult& PathResult,
	FE2GridMoveCommitResult& OutResult)
{
	CompactWeakState();
	OutResult = FE2GridMoveCommitResult();
	OutResult.FromCellKey = Unit ? Unit->GetCurrentCellKey() : INVALID_GRID_KEY;
	OutResult.ToCellKey = PathResult.GoalCellKey;
	OutResult.RuntimeRevision = RuntimeRevision;

	int32 GoalCellKey = INVALID_GRID_KEY;
	OutResult.Status = ValidatePathForCommit(Unit, PathResult, GoalCellKey);
	if (OutResult.Status != EE2GridQueryStatus::Success)
	{
		return false;
	}

	const int32 SourceCellKey = Unit->GetCurrentCellKey();
	const TWeakObjectPtr<UE2GridUnitComponent>* SourceOwner = CellOwnersByKey.Find(SourceCellKey);
	if (!SourceOwner || SourceOwner->Get() != Unit)
	{
		UE_LOG(LogE2GridRuntime, Error, TEXT("Occupancy invariant broken for %s."), *GetNameSafe(Unit->GetOwner()));
		OutResult.Status = EE2GridQueryStatus::InvalidUnit;
		return false;
	}

	if (SourceCellKey != GoalCellKey)
	{
		CellOwnersByKey.Remove(SourceCellKey);
		CellOwnersByKey.Add(GoalCellKey, Unit);
	}
	Unit->SetPlacementState(true, GoalCellKey, EE2GridRegistrationStatus::Registered);
	Unit->GetOwner()->SetActorLocation(
		ActiveManager->GetCellWorldCenterChecked(GoalCellKey),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (SourceCellKey != GoalCellKey)
	{
		AdvanceRevision(
			EE2GridStateChangeKind::OccupancyMoved,
			Unit,
			SourceCellKey,
			GoalCellKey);
	}
	OutResult.Status = EE2GridQueryStatus::Success;
	OutResult.FromCellKey = SourceCellKey;
	OutResult.ToCellKey = GoalCellKey;
	OutResult.RuntimeRevision = RuntimeRevision;
	return true;
}

bool UE2GridSubsystem::CommitUnitMove(
	UE2GridUnitComponent* Unit,
	const TArray<FE2GridPathStep>& TraversedPath)
{
	FE2GridPathResult LegacyPath;
	LegacyPath.Reset(EE2GridQueryStatus::Success);
	LegacyPath.StartCellKey = Unit ? Unit->GetCurrentCellKey() : INVALID_GRID_KEY;
	LegacyPath.GoalCellKey = TraversedPath.IsEmpty()
		? LegacyPath.StartCellKey
		: TraversedPath.Last().ToCellKey;
	LegacyPath.RuntimeRevision = RuntimeRevision;
	LegacyPath.Steps = TraversedPath;
	int32 FromCellKey = LegacyPath.StartCellKey;
	for (const FE2GridPathStep& Step : TraversedPath)
	{
		if (ActiveManager.IsValid())
		{
			ActiveManager->ForEachTraversableNeighbor(
				FromCellKey,
				[&LegacyPath, &Step](int32 NeighborKey, float StepCost)
				{
					if (NeighborKey == Step.ToCellKey)
					{
						LegacyPath.TotalCost += StepCost;
					}
				});
		}
		FromCellKey = Step.ToCellKey;
	}
	FE2GridMoveCommitResult CommitResult;
	return CommitUnitMoveFromPath(Unit, LegacyPath, CommitResult);
}

bool UE2GridSubsystem::FindPath(
	const UE2GridUnitComponent* RequestingUnit,
	int32 GoalCellKey,
	FE2GridPathResult& OutResult) const
{
	OutResult.Reset(EE2GridQueryStatus::InvalidRequest);
	OutResult.GoalCellKey = GoalCellKey;
	OutResult.RuntimeRevision = RuntimeRevision;
	if (!ActiveManager.IsValid())
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::NoActiveGrid);
		return false;
	}
	if (!RequestingUnit || !IsUnitRegistered(RequestingUnit))
	{
		OutResult.SetQueryStatus(EE2GridQueryStatus::InvalidUnit);
		return false;
	}

	OutResult.StartCellKey = RequestingUnit->GetCurrentCellKey();
	return FE2GridPathFinding::FindPath(
		*ActiveManager,
		CellOwnersByKey,
		RequestingUnit,
		OutResult.StartCellKey,
		GoalCellKey,
		OutResult);
}

bool UE2GridSubsystem::FindReachableCells(
	const UE2GridUnitComponent* RequestingUnit,
	float MovementBudget,
	FE2GridReachableResult& OutResult) const
{
	OutResult.Reset();
	OutResult.Budget = MovementBudget;
	OutResult.RuntimeRevision = RuntimeRevision;
	if (!ActiveManager.IsValid())
	{
		OutResult.Status = EE2GridQueryStatus::NoActiveGrid;
		return false;
	}
	if (!RequestingUnit || !IsUnitRegistered(RequestingUnit))
	{
		OutResult.Status = EE2GridQueryStatus::InvalidUnit;
		return false;
	}
	OutResult.StartCellKey = RequestingUnit->GetCurrentCellKey();
	if (!FMath::IsFinite(MovementBudget) || MovementBudget < 0.0f)
	{
		OutResult.Status = EE2GridQueryStatus::InvalidRequest;
		return false;
	}

	return FE2GridPathFinding::FindReachableCells(
		*ActiveManager,
		CellOwnersByKey,
		RequestingUnit,
		OutResult.StartCellKey,
		MovementBudget,
		OutResult);
}

bool UE2GridSubsystem::BuildPathFromReachableResult(
	const FE2GridReachableResult& ReachableResult,
	int32 GoalCellKey,
	FE2GridPathResult& OutResult) const
{
	if (!ActiveManager.IsValid())
	{
		OutResult.Reset(EE2GridQueryStatus::NoActiveGrid);
		OutResult.StartCellKey = ReachableResult.StartCellKey;
		OutResult.GoalCellKey = GoalCellKey;
		OutResult.RuntimeRevision = ReachableResult.RuntimeRevision;
		return false;
	}
	if (ReachableResult.RuntimeRevision != RuntimeRevision)
	{
		OutResult.Reset(EE2GridQueryStatus::StaleRevision);
		OutResult.StartCellKey = ReachableResult.StartCellKey;
		OutResult.GoalCellKey = GoalCellKey;
		OutResult.RuntimeRevision = ReachableResult.RuntimeRevision;
		return false;
	}
	return FE2GridPathFinding::BuildPathFromReachableResult(
		ReachableResult,
		GoalCellKey,
		OutResult);
}

bool UE2GridSubsystem::FindCellsInRange(
	int32 StartCellKey,
	float MaxRange,
	EE2GridRangeMetric Metric,
	FE2GridRangeResult& OutResult) const
{
	OutResult.Reset();
	OutResult.StartCellKey = StartCellKey;
	OutResult.MaxRange = MaxRange;
	OutResult.Metric = Metric;
	OutResult.RuntimeRevision = RuntimeRevision;
	if (!ActiveManager.IsValid())
	{
		OutResult.Status = EE2GridQueryStatus::NoActiveGrid;
		return false;
	}
	if (!ActiveManager->FindCell(StartCellKey))
	{
		OutResult.Status = EE2GridQueryStatus::InvalidCell;
		return false;
	}
	if (!FMath::IsFinite(MaxRange) || MaxRange < 0.0f)
	{
		OutResult.Status = EE2GridQueryStatus::InvalidRequest;
		return false;
	}

	return FE2GridPathFinding::FindCellsInRange(
		*ActiveManager,
		StartCellKey,
		MaxRange,
		Metric,
		OutResult);
}

bool UE2GridSubsystem::IsUnitRegistered(const UE2GridUnitComponent* Unit) const
{
	return Unit && RegisteredUnits.Contains(
		TWeakObjectPtr<UE2GridUnitComponent>(const_cast<UE2GridUnitComponent*>(Unit)));
}

void UE2GridSubsystem::AdvanceRevision(
	EE2GridStateChangeKind ChangeKind,
	UE2GridUnitComponent* Unit,
	int32 FromCellKey,
	int32 ToCellKey)
{
	checkf(RuntimeRevision < TNumericLimits<int64>::Max(), TEXT("E2Grid runtime revision overflowed."));
	++RuntimeRevision;
	FE2GridStateDelta Delta;
	Delta.ChangeKind = ChangeKind;
	Delta.Unit = Unit;
	Delta.FromCellKey = FromCellKey;
	Delta.ToCellKey = ToCellKey;
	Delta.RuntimeRevision = RuntimeRevision;
	OnStateChanged.Broadcast(Delta);
}

void UE2GridSubsystem::RetryPendingUnits()
{
	TArray<TWeakObjectPtr<UE2GridUnitComponent>> UnitsToRetry = PendingUnits.Array();
	UnitsToRetry.Sort([](
		const TWeakObjectPtr<UE2GridUnitComponent>& Left,
		const TWeakObjectPtr<UE2GridUnitComponent>& Right)
	{
		return GetPathNameSafe(Left.Get()) < GetPathNameSafe(Right.Get());
	});
	for (const TWeakObjectPtr<UE2GridUnitComponent>& Unit : UnitsToRetry)
	{
		if (Unit.IsValid())
		{
			RegisterUnit(Unit.Get());
		}
	}
	CompactWeakState();
}

void UE2GridSubsystem::CompactWeakState()
{
	for (auto It = RegisteredUnits.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = PendingUnits.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	TArray<int32> ReleasedCellKeys;
	for (auto It = CellOwnersByKey.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid())
		{
			ReleasedCellKeys.Add(It.Key());
			It.RemoveCurrent();
		}
	}
	ReleasedCellKeys.Sort();
	for (int32 ReleasedCellKey : ReleasedCellKeys)
	{
		AdvanceRevision(
			EE2GridStateChangeKind::UnitUnregistered,
			nullptr,
			ReleasedCellKey,
			INVALID_GRID_KEY);
	}
}
