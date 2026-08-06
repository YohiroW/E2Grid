#pragma once

#include "CoreMinimal.h"
#include "E2GridRuntimeData.h"
#include "Subsystems/WorldSubsystem.h"
#include "E2GridSubsystem.generated.h"

class AE2GridManager;
class UE2GridUnitComponent;

UCLASS()
class E2GRID_API UE2GridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	bool RegisterManager(AE2GridManager* Manager);
	void UnregisterManager(AE2GridManager* Manager);

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	AE2GridManager* GetActiveManager() const { return ActiveManager.Get(); }

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	EE2GridRegistrationStatus RegisterUnit(UE2GridUnitComponent* Unit);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	void UnregisterUnit(UE2GridUnitComponent* Unit);

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool WorldToCell(const FVector& WorldPosition, int32& OutCellKey) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool CellToWorld(int32 CellKey, FVector& OutWorldPosition) const;

	UFUNCTION(BlueprintPure, Category = "E2Grid")
	bool GetCell(int32 CellKey, FE2GridCellData& OutCellData) const;

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	UE2GridUnitComponent* GetCellOwner(int32 CellKey);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool CanPlaceUnit(const UE2GridUnitComponent* Unit, int32 CellKey);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool CommitUnitMove(UE2GridUnitComponent* Unit, int32 GoalCellKey);

	UFUNCTION(BlueprintCallable, Category = "E2Grid")
	bool FindPath(
		const UE2GridUnitComponent* RequestingUnit,
		int32 GoalCellKey,
		FE2GridPathResult& OutResult) const;

	bool IsUnitRegistered(const UE2GridUnitComponent* Unit) const;

private:
	void RetryPendingUnits();
	void CompactWeakState();

	TWeakObjectPtr<AE2GridManager> ActiveManager;
	TSet<TWeakObjectPtr<UE2GridUnitComponent>> RegisteredUnits;
	TSet<TWeakObjectPtr<UE2GridUnitComponent>> PendingUnits;
	TMap<int32, TWeakObjectPtr<UE2GridUnitComponent>> CellOwnersByKey;

	friend class FE2GridPathFinding;
};
