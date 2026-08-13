#pragma once

#include "CoreMinimal.h"
#include "E2GridCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EE2GridTeam : uint8
{
	Player,
	Enemy,
	Neutral,
};

UENUM(BlueprintType)
enum class EE2GridTurnState : uint8
{
	PlayerTurn,
	ResolvingPlayerAction,
	EnemyTurn,
	ResolvingEnemyAction,
};
