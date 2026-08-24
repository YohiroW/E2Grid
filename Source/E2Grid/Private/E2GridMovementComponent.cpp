#include "E2GridMovementComponent.h"

#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "Components/PrimitiveComponent.h"
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
	if (!UnitComponent && GetOwner())
	{
		UnitComponent = GetOwner()->FindComponentByClass<UE2GridUnitComponent>();
	}
	if (bMoving)
	{
		BroadcastPreflightFailure(EE2GridQueryStatus::InvalidRequest, GoalCellKey);
		return false;
	}
	if (!UnitComponent || !UnitComponent->IsRegistered())
	{
		BroadcastPreflightFailure(EE2GridQueryStatus::InvalidUnit, GoalCellKey);
		return false;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	FE2GridPathResult PathResult;
	if (!GridSubsystem)
	{
		BroadcastPreflightFailure(EE2GridQueryStatus::NoActiveGrid, GoalCellKey);
		return false;
	}
	if (!GridSubsystem->FindPath(UnitComponent, GoalCellKey, PathResult))
	{
		BroadcastPreflightFailure(PathResult.QueryStatus, GoalCellKey);
		return false;
	}
	const EE2GridQueryStatus ValidationStatus = GridSubsystem->ValidatePath(UnitComponent, PathResult);
	if (ValidationStatus != EE2GridQueryStatus::Success)
	{
		BroadcastPreflightFailure(ValidationStatus, GoalCellKey);
		return false;
	}

	RequestedGoalCellKey = GoalCellKey;
	ActivePathResult = MoveTemp(PathResult);
	NextStepIndex = 0;
	if (ActivePathResult.Steps.IsEmpty())
	{
		FE2GridMoveCommitResult Result;
		Result.Status = EE2GridQueryStatus::Success;
		Result.FromCellKey = UnitComponent->GetCurrentCellKey();
		Result.ToCellKey = RequestedGoalCellKey;
		Result.RuntimeRevision = GridSubsystem->GetRuntimeRevision();
		OnMovementCompleted.Broadcast(Result);
		OnMovementFinished.Broadcast(true, RequestedGoalCellKey);
		ActivePathResult.Reset();
		RequestedGoalCellKey = INVALID_GRID_KEY;
		return true;
	}

	bMoving = true;
	if (!BeginNextStep())
	{
		FailMove(EE2GridQueryStatus::InvalidCell);
		return false;
	}
	SetComponentTickEnabled(true);
	return true;
}

void UE2GridMovementComponent::CancelMove()
{
	if (!bMoving)
	{
		return;
	}
	FailMove(EE2GridQueryStatus::InvalidRequest);
}

bool UE2GridMovementComponent::BeginNextStep()
{
	if (!ActivePathResult.Steps.IsValidIndex(NextStepIndex))
	{
		return false;
	}
	if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		return GridSubsystem->CellToWorld(
			ActivePathResult.Steps[NextStepIndex].ToCellKey,
			CurrentStepTarget);
	}
	return false;
}

void UE2GridMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bMoving)
	{
		return;
	}

	AActor* Owner = GetOwner();
	const FVector NewLocation = FMath::VInterpConstantTo(
		Owner->GetActorLocation(),
		CurrentStepTarget,
		DeltaTime,
		MovementSpeed);
	if (bSweepDuringMovement)
	{
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
		UPrimitiveComponent* CollisionComponent = nullptr;
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (PrimitiveComponent && PrimitiveComponent->IsCollisionEnabled())
			{
				CollisionComponent = PrimitiveComponent;
				break;
			}
		}
		if (CollisionComponent)
		{
			TArray<FHitResult> SweepHits;
			FComponentQueryParams QueryParams(SCENE_QUERY_STAT(E2GridMovement), Owner);
			if (GetWorld()->ComponentSweepMulti(
				SweepHits,
				CollisionComponent,
				CollisionComponent->GetComponentLocation(),
				CollisionComponent->GetComponentLocation() + (NewLocation - Owner->GetActorLocation()),
				CollisionComponent->GetComponentQuat(),
				QueryParams) && SweepHits.ContainsByPredicate([](const FHitResult& Hit)
				{
					return Hit.bBlockingHit;
				}))
			{
				FailMove(EE2GridQueryStatus::NotTraversable);
				return;
			}
		}
	}

	if (!Owner->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None))
	{
		FailMove(EE2GridQueryStatus::NotTraversable);
		return;
	}

	if (!NewLocation.Equals(CurrentStepTarget, 0.1f))
	{
		return;
	}

	Owner->SetActorLocation(CurrentStepTarget, false, nullptr, ETeleportType::TeleportPhysics);
	++NextStepIndex;
	if (NextStepIndex < ActivePathResult.Steps.Num())
	{
		if (!BeginNextStep())
		{
			FailMove(EE2GridQueryStatus::InvalidCell);
		}
		return;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	FE2GridMoveCommitResult CommitResult;
	const bool bCommitted = GridSubsystem && GridSubsystem->CommitUnitMoveFromPath(
		UnitComponent,
		ActivePathResult,
		CommitResult);
	if (!GridSubsystem)
	{
		CommitResult.Status = EE2GridQueryStatus::NoActiveGrid;
		CommitResult.FromCellKey = UnitComponent ? UnitComponent->GetCurrentCellKey() : INVALID_GRID_KEY;
		CommitResult.ToCellKey = RequestedGoalCellKey;
	}
	if (!bCommitted)
	{
		RestoreOccupiedLocation();
	}
	FinishMove(CommitResult);
}

void UE2GridMovementComponent::FailMove(EE2GridQueryStatus Status)
{
	RestoreOccupiedLocation();
	FE2GridMoveCommitResult Result;
	Result.Status = Status;
	Result.FromCellKey = UnitComponent ? UnitComponent->GetCurrentCellKey() : INVALID_GRID_KEY;
	Result.ToCellKey = RequestedGoalCellKey;
	if (const UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		Result.RuntimeRevision = GridSubsystem->GetRuntimeRevision();
	}
	FinishMove(Result);
}

void UE2GridMovementComponent::FinishMove(const FE2GridMoveCommitResult& Result)
{
	const int32 FinishedGoal = RequestedGoalCellKey;
	bMoving = false;
	ActivePathResult.Reset();
	NextStepIndex = 0;
	RequestedGoalCellKey = INVALID_GRID_KEY;
	CurrentStepTarget = FVector::ZeroVector;
	SetComponentTickEnabled(false);
	OnMovementCompleted.Broadcast(Result);
	OnMovementFinished.Broadcast(Result.Status == EE2GridQueryStatus::Success, FinishedGoal);
}

void UE2GridMovementComponent::BroadcastPreflightFailure(
	EE2GridQueryStatus Status,
	int32 GoalCellKey)
{
	FE2GridMoveCommitResult Result;
	Result.Status = Status;
	Result.FromCellKey = UnitComponent ? UnitComponent->GetCurrentCellKey() : INVALID_GRID_KEY;
	Result.ToCellKey = GoalCellKey;
	if (const UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		Result.RuntimeRevision = GridSubsystem->GetRuntimeRevision();
	}
	OnMovementCompleted.Broadcast(Result);
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
