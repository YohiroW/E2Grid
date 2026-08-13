#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "E2GridRuntimeData.h"
#include "E2GridMovementComponent.generated.h"

class UE2GridUnitComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FE2GridMovementFinished,
	bool, bSucceeded,
	int32, GoalCellKey);

UCLASS(ClassGroup = (E2Grid), meta = (BlueprintSpawnableComponent))
class E2GRID_API UE2GridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UE2GridMovementComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool MoveToCell(int32 GoalCellKey);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	void CancelMove();

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool IsMoving() const { return bMoving; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "1.0"))
	float MovementSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bSweepDuringMovement = true;

	UPROPERTY(BlueprintAssignable, Category = "Movement")
	FE2GridMovementFinished OnMovementFinished;

private:
	bool BeginNextStep();
	void FinishMove(bool bSucceeded);
	void RestoreOccupiedLocation();

	UPROPERTY(Transient)
	TObjectPtr<UE2GridUnitComponent> UnitComponent;

	TArray<FE2GridPathStep> PathSteps;
	int32 NextStepIndex = 0;
	int32 RequestedGoalCellKey = INVALID_GRID_KEY;
	FVector CurrentStepTarget = FVector::ZeroVector;
	bool bMoving = false;
};
