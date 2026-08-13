#pragma once

#include "CoreMinimal.h"
#include "E2GridCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "E2GridCombatUnit.generated.h"

class AE2GridCombatUnit;
class UCapsuleComponent;
class UE2GridMovementComponent;
class UE2GridUnitComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FE2GridCombatUnitDefeated, AE2GridCombatUnit*, Unit);

UCLASS(Blueprintable)
class E2GRIDGAME_API AE2GridCombatUnit : public APawn
{
	GENERATED_BODY()

public:
	AE2GridCombatUnit();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Combat")
	EE2GridTeam GetTeam() const { return Team; }

	/** Intended for spawn/setup before BeginPlay; team is immutable during a match. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool SetTeamForSetup(EE2GridTeam InTeam);

	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAlive() const { return Health > 0; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyFixedDamage(int32 Damage);

	UE2GridUnitComponent* GetGridUnit() const { return GridUnit; }
	UE2GridMovementComponent* GetGridMovement() const { return GridMovement; }

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FE2GridCombatUnitDefeated OnDefeated;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> TeamLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UE2GridUnitComponent> GridUnit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UE2GridMovementComponent> GridMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	EE2GridTeam Team = EE2GridTeam::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 MaxHealth = 100;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	int32 Health = 100;

private:
	void ApplyTeamVisuals();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TeamMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseVisualMaterial;
};
