#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "E2GridCombatUnit.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UE2GridMovementComponent;
class UE2GridUnitComponent;

UENUM(BlueprintType)
enum class EE2GridTeam : uint8
{
	Player,
	Enemy,
	Neutral,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FE2GridHealthChanged, int32, NewHealth, int32, Delta);

UCLASS()
class E2GRIDGAME_API AE2GridCombatUnit : public APawn
{
	GENERATED_BODY()

public:
	AE2GridCombatUnit();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyFixedDamage(int32 Damage);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAlive() const { return Health > 0; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	EE2GridTeam GetTeam() const { return Team; }

	UE2GridUnitComponent* GetGridUnitComponent() const { return GridUnitComponent; }
	UE2GridMovementComponent* GetGridMovementComponent() const { return GridMovementComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	EE2GridTeam Team = EE2GridTeam::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 MaxHealth = 100;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat")
	int32 Health = 100;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FE2GridHealthChanged OnHealthChanged;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UE2GridUnitComponent> GridUnitComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UE2GridMovementComponent> GridMovementComponent;
};
