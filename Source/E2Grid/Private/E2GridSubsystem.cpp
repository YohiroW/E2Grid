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
	Super::Deinitialize();
}

bool UE2GridSubsystem::RegisterManager(AE2GridManager* Manager)
{
	if (!IsValid(Manager) || !Manager->HasValidGrid())
	{
		UE_LOG(LogE2GridRuntime, Error, TEXT("Cannot register invalid grid manager %s."), *GetNameSafe(Manager));
		return false;
	}

	if (ActiveManager.IsValid() && ActiveManager.Get() != Manager)
	{
		UE_LOG(
			LogE2GridRuntime,
			Error,
			TEXT("Only one active E2Grid manager is supported. Keeping %s and rejecting %s."),
			*GetNameSafe(ActiveManager.Get()),
			*GetNameSafe(Manager));
		return false;
	}

	ActiveManager = Manager;
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

	const FE2GridCellData* Cell = ActiveManager->FindCell(CellKey);
	if (!Cell || !Cell->CanStandOn())
	{
		PendingUnits.Remove(WeakUnit);
		Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::CellNotStandable);
		return EE2GridRegistrationStatus::CellNotStandable;
	}

	if (UE2GridUnitComponent* ExistingOwner = GetCellOwner(CellKey))
	{
		PendingUnits.Remove(WeakUnit);
		Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::CellOccupied);
		UE_LOG(
			LogE2GridRuntime,
			Verbose,
			TEXT("Unit %s cannot occupy cell %d; it is owned by %s."),
			*GetNameSafe(Unit->GetOwner()),
			CellKey,
			*GetNameSafe(ExistingOwner->GetOwner()));
		return EE2GridRegistrationStatus::CellOccupied;
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
	RegisteredUnits.Remove(WeakUnit);
	if (TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwnersByKey.Find(Unit->GetCurrentCellKey());
		Owner && Owner->Get() == Unit)
	{
		CellOwnersByKey.Remove(Unit->GetCurrentCellKey());
	}
	Unit->SetPlacementState(false, INVALID_GRID_KEY, EE2GridRegistrationStatus::PendingManager);
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

UE2GridUnitComponent* UE2GridSubsystem::GetCellOwner(int32 CellKey)
{
	if (TWeakObjectPtr<UE2GridUnitComponent>* Owner = CellOwnersByKey.Find(CellKey))
	{
		if (Owner->IsValid())
		{
			return Owner->Get();
		}
		CellOwnersByKey.Remove(CellKey);
	}
	return nullptr;
}

bool UE2GridSubsystem::CanPlaceUnit(const UE2GridUnitComponent* Unit, int32 CellKey)
{
	if (!ActiveManager.IsValid())
	{
		return false;
	}
	const FE2GridCellData* Cell = ActiveManager->FindCell(CellKey);
	if (!Cell || !Cell->CanStandOn())
	{
		return false;
	}
	const UE2GridUnitComponent* Owner = GetCellOwner(CellKey);
	return !Owner || Owner == Unit;
}

bool UE2GridSubsystem::CommitUnitMove(UE2GridUnitComponent* Unit, int32 GoalCellKey)
{
	if (!Unit || !IsUnitRegistered(Unit) || !CanPlaceUnit(Unit, GoalCellKey))
	{
		return false;
	}

	const int32 SourceCellKey = Unit->GetCurrentCellKey();
	const TWeakObjectPtr<UE2GridUnitComponent>* SourceOwner = CellOwnersByKey.Find(SourceCellKey);
	if (!SourceOwner || SourceOwner->Get() != Unit)
	{
		UE_LOG(LogE2GridRuntime, Error, TEXT("Occupancy invariant broken for %s."), *GetNameSafe(Unit->GetOwner()));
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
	return true;
}

bool UE2GridSubsystem::FindPath(
	const UE2GridUnitComponent* RequestingUnit,
	int32 GoalCellKey,
	FE2GridPathResult& OutResult) const
{
	if (!RequestingUnit || !IsUnitRegistered(RequestingUnit) || !ActiveManager.IsValid())
	{
		OutResult.Reset(EE2GridPathStatus::InvalidUnit);
		return false;
	}

	return FE2GridPathFinding::FindPath(
		*ActiveManager,
		CellOwnersByKey,
		RequestingUnit,
		RequestingUnit->GetCurrentCellKey(),
		GoalCellKey,
		OutResult);
}

bool UE2GridSubsystem::IsUnitRegistered(const UE2GridUnitComponent* Unit) const
{
	return Unit && RegisteredUnits.Contains(
		TWeakObjectPtr<UE2GridUnitComponent>(const_cast<UE2GridUnitComponent*>(Unit)));
}

void UE2GridSubsystem::RetryPendingUnits()
{
	TArray<TWeakObjectPtr<UE2GridUnitComponent>> UnitsToRetry = PendingUnits.Array();
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
	for (auto It = CellOwnersByKey.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}
