#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "E2GridRuntimeData.h"
#include "E2GridUnitComponent.generated.h"

class UE2GridSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FE2GridRegistrationChanged,
	bool, bRegistered,
	EE2GridRegistrationStatus, Status);

UCLASS(ClassGroup = (E2Grid), meta = (BlueprintSpawnableComponent))
class E2GRID_API UE2GridUnitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UE2GridUnitComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool IsRegistered() const { return bRegistered; }

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	int32 GetCurrentCellKey() const { return CurrentCellKey; }

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	EE2GridRegistrationStatus RequestRegistration();

	UPROPERTY(BlueprintAssignable, Category = "E2Grid")
	FE2GridRegistrationChanged OnRegistrationChanged;

private:
	void SetPlacementState(bool bInRegistered, int32 InCellKey, EE2GridRegistrationStatus Status);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "E2Grid", meta = (AllowPrivateAccess = "true"))
	bool bRegistered = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "E2Grid", meta = (AllowPrivateAccess = "true"))
	int32 CurrentCellKey = INVALID_GRID_KEY;

	friend class UE2GridSubsystem;
};
