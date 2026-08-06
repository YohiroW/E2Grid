#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "E2GridGameModeBase.generated.h"

class AE2GridCombatUnit;

UENUM(BlueprintType)
enum class EE2GridTurnState : uint8
{
	PlayerTurn,
	ResolvingPlayerAction,
	EnemyTurn,
	ResolvingEnemyAction,
	PlayerVictory,
	PlayerDefeat,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FE2GridTurnStateChanged, EE2GridTurnState, NewState);

UCLASS()
class E2GRIDGAME_API AE2GridGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AE2GridGameModeBase();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Turn")
	EE2GridTurnState GetTurnState() const { return TurnState; }

	UFUNCTION(BlueprintCallable, Category = "Turn")
	bool RequestPlayerMove(AE2GridCombatUnit* Unit, int32 GoalCellKey);

	UFUNCTION(BlueprintCallable, Category = "Turn")
	bool RequestPlayerAttack(AE2GridCombatUnit* Attacker, AE2GridCombatUnit* Target);

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void SkipPlayerAction();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 FixedAttackDamage = 25;

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FE2GridTurnStateChanged OnTurnStateChanged;

private:
	void SetTurnState(EE2GridTurnState NewState);
	void RefreshCombatUnits();
	void BeginEnemyAction();
	void FinishPlayerAction();
	void FinishEnemyAction();
	bool AreAdjacent(const AE2GridCombatUnit* A, const AE2GridCombatUnit* B) const;

	UFUNCTION()
	void HandleMovementFinished(bool bSucceeded, int32 GoalCellKey);

	UPROPERTY(Transient)
	TObjectPtr<AE2GridCombatUnit> PlayerUnit;

	UPROPERTY(Transient)
	TObjectPtr<AE2GridCombatUnit> EnemyUnit;

	TWeakObjectPtr<AE2GridCombatUnit> ResolvingUnit;
	EE2GridTurnState TurnState = EE2GridTurnState::PlayerTurn;
};
