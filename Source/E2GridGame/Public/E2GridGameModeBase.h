#pragma once

#include "CoreMinimal.h"
#include "E2GridCombatTypes.h"
#include "GameFramework/GameModeBase.h"
#include "E2GridGameModeBase.generated.h"

class AE2GridCombatUnit;

UCLASS()
class E2GRIDGAME_API AE2GridGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AE2GridGameModeBase();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "E2Grid Game")
	EE2GridTurnState GetTurnState() const { return TurnState; }

	UFUNCTION(BlueprintPure, Category = "E2Grid Game")
	bool CanPlayerIssueAction() const;

	UFUNCTION(BlueprintCallable, Category = "E2Grid Game")
	bool SubmitPlayerMove(AE2GridCombatUnit* Unit, int32 GoalCellKey);

	UFUNCTION(BlueprintCallable, Category = "E2Grid Game")
	bool SubmitPlayerAttack(AE2GridCombatUnit* Attacker, AE2GridCombatUnit* Target);

	UFUNCTION(BlueprintCallable, Category = "E2Grid Game")
	bool SkipPlayerAction();

	UFUNCTION(BlueprintPure, Category = "E2Grid Game")
	AE2GridCombatUnit* GetPlayerUnit() const { return PlayerUnit.Get(); }

	UFUNCTION(BlueprintPure, Category = "E2Grid Game")
	AE2GridCombatUnit* GetEnemyUnit() const { return EnemyUnit.Get(); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (ClampMin = "1"))
	int32 AttackDamage = 25;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Turn")
	EE2GridTurnState TurnState = EE2GridTurnState::PlayerTurn;

private:
	bool IsTraversableAdjacent(const AE2GridCombatUnit& From, const AE2GridCombatUnit& To) const;
	bool TryAttack(AE2GridCombatUnit& Attacker, AE2GridCombatUnit& Target);
	void DiscoverCombatants();
	void FinishPlayerAction();
	void StartEnemyTurn();
	void ResolveEnemyTurn();
	void FinishEnemyAction();

	UFUNCTION()
	void HandlePlayerMovementFinished(bool bSucceeded, int32 GoalCellKey);

	UFUNCTION()
	void HandleEnemyMovementFinished(bool bSucceeded, int32 GoalCellKey);

	TWeakObjectPtr<AE2GridCombatUnit> PlayerUnit;
	TWeakObjectPtr<AE2GridCombatUnit> EnemyUnit;
};
