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
	PathSteps = MoveTemp(PathResult.Steps);
	NextStepIndex = 0;
	if (PathSteps.IsEmpty())
	{
		OnMovementFinished.Broadcast(true, RequestedGoalCellKey);
		RequestedGoalCellKey = INVALID_GRID_KEY;
		return true;
	}

	bMoving = true;
	if (!BeginNextStep())
	{
		FinishMove(false);
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
	RestoreOccupiedLocation();
	FinishMove(false);
}

bool UE2GridMovementComponent::BeginNextStep()
{
	if (!PathSteps.IsValidIndex(NextStepIndex))
	{
		return false;
	}
	if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		return GridSubsystem->CellToWorld(PathSteps[NextStepIndex].ToCellKey, CurrentStepTarget);
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
				CancelMove();
				return;
			}
		}
	}

	if (!Owner->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None))
	{
		CancelMove();
		return;
	}

	if (!NewLocation.Equals(CurrentStepTarget, 0.1f))
	{
		return;
	}

	Owner->SetActorLocation(CurrentStepTarget, false, nullptr, ETeleportType::TeleportPhysics);
	++NextStepIndex;
	if (NextStepIndex < PathSteps.Num())
	{
		if (!BeginNextStep())
		{
			CancelMove();
		}
		return;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	const bool bCommitted = GridSubsystem &&
		GridSubsystem->CommitUnitMove(UnitComponent, PathSteps);
	if (!bCommitted)
	{
		RestoreOccupiedLocation();
	}
	FinishMove(bCommitted);
}

void UE2GridMovementComponent::FinishMove(bool bSucceeded)
{
	const int32 FinishedGoal = RequestedGoalCellKey;
	bMoving = false;
	PathSteps.Reset();
	NextStepIndex = 0;
	RequestedGoalCellKey = INVALID_GRID_KEY;
	CurrentStepTarget = FVector::ZeroVector;
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
