#include "E2GridMovementComponent.h"

#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UE2GridMovementComponent::UE2GridMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UE2GridMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	UnitComponent = GetOwner()->FindComponentByClass<UE2GridUnitComponent>();
	SetComponentTickEnabled(false);
}

void UE2GridMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bMoving)
	{
		RestoreOccupiedLocation();
	}
	bMoving = false;
	Super::EndPlay(EndPlayReason);
}

bool UE2GridMovementComponent::MoveToCell(int32 GoalCellKey)
{
	if (bMoving || !UnitComponent || !UnitComponent->IsRegistered())
	{
		return false;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	FE2GridPathResult PathResult;
	if (!GridSubsystem || !GridSubsystem->FindPath(UnitComponent, GoalCellKey, PathResult) ||
		PathResult.Status != EE2GridPathStatus::Success)
	{
		return false;
	}

	RequestedGoalCellKey = GoalCellKey;
	PathCellKeys = MoveTemp(PathResult.CellKeys);
	NextWaypointIndex = 0;
	if (PathCellKeys.IsEmpty())
	{
		OnMovementFinished.Broadcast(true, RequestedGoalCellKey);
		RequestedGoalCellKey = INVALID_GRID_KEY;
		return true;
	}

	bMoving = true;
	SetComponentTickEnabled(true);
	return true;
}

void UE2GridMovementComponent::CancelMove()
{
	if (!bMoving)
	{
		return;
	}
	RestoreOccupiedLocation();
	FinishMove(false);
}

void UE2GridMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bMoving || !PathCellKeys.IsValidIndex(NextWaypointIndex))
	{
		return;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	FVector Waypoint;
	if (!GridSubsystem || !GridSubsystem->CellToWorld(PathCellKeys[NextWaypointIndex], Waypoint))
	{
		CancelMove();
		return;
	}

	AActor* Owner = GetOwner();
	const FVector NewLocation = FMath::VInterpConstantTo(
		Owner->GetActorLocation(),
		Waypoint,
		DeltaTime,
		MovementSpeed);
	FHitResult SweepHit;
	const bool bMoved = Owner->SetActorLocation(
		NewLocation,
		bSweepDuringMovement,
		bSweepDuringMovement ? &SweepHit : nullptr,
		ETeleportType::None);
	if (!bMoved || (bSweepDuringMovement && SweepHit.bBlockingHit))
	{
		CancelMove();
		return;
	}

	if (!NewLocation.Equals(Waypoint, 0.1f))
	{
		return;
	}

	Owner->SetActorLocation(Waypoint, false, nullptr, ETeleportType::TeleportPhysics);
	++NextWaypointIndex;
	if (NextWaypointIndex >= PathCellKeys.Num())
	{
		const bool bCommitted = GridSubsystem->CommitUnitMove(UnitComponent, RequestedGoalCellKey);
		if (!bCommitted)
		{
			RestoreOccupiedLocation();
		}
		FinishMove(bCommitted);
	}
}

void UE2GridMovementComponent::FinishMove(bool bSucceeded)
{
	const int32 FinishedGoal = RequestedGoalCellKey;
	bMoving = false;
	PathCellKeys.Reset();
	NextWaypointIndex = 0;
	RequestedGoalCellKey = INVALID_GRID_KEY;
	SetComponentTickEnabled(false);
	OnMovementFinished.Broadcast(bSucceeded, FinishedGoal);
}

void UE2GridMovementComponent::RestoreOccupiedLocation()
{
	if (!UnitComponent || !UnitComponent->IsRegistered())
	{
		return;
	}
	if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		FVector OccupiedCenter;
		if (GridSubsystem->CellToWorld(UnitComponent->GetCurrentCellKey(), OccupiedCenter))
		{
			GetOwner()->SetActorLocation(OccupiedCenter, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}
