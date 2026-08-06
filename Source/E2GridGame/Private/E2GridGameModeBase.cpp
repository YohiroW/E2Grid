#include "E2GridGameModeBase.h"

#include "E2GridCameraPawn.h"
#include "E2GridCombatUnit.h"
#include "E2GridManager.h"
#include "E2GridMovementComponent.h"
#include "E2GridPlayerController.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogE2GridGame, Log, All);

AE2GridGameModeBase::AE2GridGameModeBase()
{
	DefaultPawnClass = AE2GridCameraPawn::StaticClass();
	PlayerControllerClass = AE2GridPlayerController::StaticClass();
}

void AE2GridGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshCombatUnits();
	SetTurnState(EE2GridTurnState::PlayerTurn);
}

bool AE2GridGameModeBase::RequestPlayerMove(AE2GridCombatUnit* Unit, int32 GoalCellKey)
{
	if (TurnState != EE2GridTurnState::PlayerTurn || !IsValid(Unit) ||
		Unit != PlayerUnit || Unit->GetTeam() != EE2GridTeam::Player)
	{
		return false;
	}

	SetTurnState(EE2GridTurnState::ResolvingPlayerAction);
	ResolvingUnit = Unit;
	if (!Unit->GetGridMovementComponent()->MoveToCell(GoalCellKey))
	{
		ResolvingUnit.Reset();
		SetTurnState(EE2GridTurnState::PlayerTurn);
		return false;
	}
	return true;
}

bool AE2GridGameModeBase::RequestPlayerAttack(
	AE2GridCombatUnit* Attacker,
	AE2GridCombatUnit* Target)
{
	if (TurnState != EE2GridTurnState::PlayerTurn || !IsValid(Attacker) || !IsValid(Target) ||
		Attacker != PlayerUnit || Target != EnemyUnit || !AreAdjacent(Attacker, Target))
	{
		return false;
	}

	SetTurnState(EE2GridTurnState::ResolvingPlayerAction);
	Target->ApplyFixedDamage(FixedAttackDamage);
	FinishPlayerAction();
	return true;
}

void AE2GridGameModeBase::SkipPlayerAction()
{
	if (TurnState == EE2GridTurnState::PlayerTurn)
	{
		FinishPlayerAction();
	}
}

void AE2GridGameModeBase::SetTurnState(EE2GridTurnState NewState)
{
	if (TurnState != NewState)
	{
		TurnState = NewState;
		OnTurnStateChanged.Broadcast(TurnState);
	}
}

void AE2GridGameModeBase::RefreshCombatUnits()
{
	PlayerUnit = nullptr;
	EnemyUnit = nullptr;
	for (TActorIterator<AE2GridCombatUnit> It(GetWorld()); It; ++It)
	{
		AE2GridCombatUnit* Unit = *It;
		if (Unit->GetTeam() == EE2GridTeam::Player && !PlayerUnit)
		{
			PlayerUnit = Unit;
		}
		else if (Unit->GetTeam() == EE2GridTeam::Enemy && !EnemyUnit)
		{
			EnemyUnit = Unit;
		}
		Unit->GetGridMovementComponent()->OnMovementFinished.AddUniqueDynamic(
			this,
			&AE2GridGameModeBase::HandleMovementFinished);
	}

	if (!PlayerUnit || !EnemyUnit)
	{
		UE_LOG(
			LogE2GridGame,
			Warning,
			TEXT("The v0.1 game loop expects one Player combat unit and one Enemy combat unit."));
	}
}

void AE2GridGameModeBase::BeginEnemyAction()
{
	if (!IsValid(PlayerUnit) || !PlayerUnit->IsAlive())
	{
		SetTurnState(EE2GridTurnState::PlayerDefeat);
		return;
	}
	if (!IsValid(EnemyUnit) || !EnemyUnit->IsAlive())
	{
		SetTurnState(EE2GridTurnState::PlayerVictory);
		return;
	}

	SetTurnState(EE2GridTurnState::EnemyTurn);
	if (AreAdjacent(EnemyUnit, PlayerUnit))
	{
		SetTurnState(EE2GridTurnState::ResolvingEnemyAction);
		PlayerUnit->ApplyFixedDamage(FixedAttackDamage);
		FinishEnemyAction();
		return;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	AE2GridManager* Manager = GridSubsystem ? GridSubsystem->GetActiveManager() : nullptr;
	const FE2GridMapLayout* Layout = Manager ? Manager->GetLayout() : nullptr;
	UE2GridUnitComponent* PlayerGridUnit = PlayerUnit->GetGridUnitComponent();
	UE2GridUnitComponent* EnemyGridUnit = EnemyUnit->GetGridUnitComponent();
	FE2GridCoord PlayerCoord;
	if (!GridSubsystem || !Layout || !PlayerGridUnit->IsRegistered() || !EnemyGridUnit->IsRegistered() ||
		!Layout->KeyToCoord(PlayerGridUnit->GetCurrentCellKey(), PlayerCoord))
	{
		FinishEnemyAction();
		return;
	}

	int32 BestGoal = INVALID_GRID_KEY;
	float BestCost = TNumericLimits<float>::Max();
	for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
	{
		const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
		const int32 CandidateKey = Layout->CoordToKey(
			FE2GridCoord(PlayerCoord.X + Offset.X, PlayerCoord.Y + Offset.Y));
		if (CandidateKey == INVALID_GRID_KEY || !GridSubsystem->CanPlaceUnit(EnemyGridUnit, CandidateKey))
		{
			continue;
		}

		FE2GridPathResult CandidatePath;
		if (GridSubsystem->FindPath(EnemyGridUnit, CandidateKey, CandidatePath) &&
			(CandidatePath.TotalCost < BestCost ||
				(FMath::IsNearlyEqual(CandidatePath.TotalCost, BestCost) &&
					(BestGoal == INVALID_GRID_KEY || CandidateKey < BestGoal))))
		{
			BestCost = CandidatePath.TotalCost;
			BestGoal = CandidateKey;
		}
	}

	if (BestGoal == INVALID_GRID_KEY)
	{
		FinishEnemyAction();
		return;
	}

	SetTurnState(EE2GridTurnState::ResolvingEnemyAction);
	ResolvingUnit = EnemyUnit;
	if (!EnemyUnit->GetGridMovementComponent()->MoveToCell(BestGoal))
	{
		ResolvingUnit.Reset();
		FinishEnemyAction();
	}
}

void AE2GridGameModeBase::FinishPlayerAction()
{
	ResolvingUnit.Reset();
	if (!IsValid(EnemyUnit) || !EnemyUnit->IsAlive())
	{
		SetTurnState(EE2GridTurnState::PlayerVictory);
		return;
	}
	SetTurnState(EE2GridTurnState::EnemyTurn);
	GetWorldTimerManager().SetTimerForNextTick(this, &AE2GridGameModeBase::BeginEnemyAction);
}

void AE2GridGameModeBase::FinishEnemyAction()
{
	ResolvingUnit.Reset();
	if (!IsValid(PlayerUnit) || !PlayerUnit->IsAlive())
	{
		SetTurnState(EE2GridTurnState::PlayerDefeat);
	}
	else
	{
		SetTurnState(EE2GridTurnState::PlayerTurn);
	}
}

bool AE2GridGameModeBase::AreAdjacent(
	const AE2GridCombatUnit* A,
	const AE2GridCombatUnit* B) const
{
	if (!A || !B)
	{
		return false;
	}
	const UE2GridUnitComponent* UnitA = A->GetGridUnitComponent();
	const UE2GridUnitComponent* UnitB = B->GetGridUnitComponent();
	const UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	const AE2GridManager* Manager = GridSubsystem ? GridSubsystem->GetActiveManager() : nullptr;
	const FE2GridMapLayout* Layout = Manager ? Manager->GetLayout() : nullptr;
	FE2GridCoord CoordA;
	FE2GridCoord CoordB;
	if (!Layout || !UnitA->IsRegistered() || !UnitB->IsRegistered() ||
		!Layout->KeyToCoord(UnitA->GetCurrentCellKey(), CoordA) ||
		!Layout->KeyToCoord(UnitB->GetCurrentCellKey(), CoordB))
	{
		return false;
	}
	return FMath::Max(FMath::Abs(CoordA.X - CoordB.X), FMath::Abs(CoordA.Y - CoordB.Y)) == 1;
}

void AE2GridGameModeBase::HandleMovementFinished(bool bSucceeded, int32 GoalCellKey)
{
	if (!ResolvingUnit.IsValid())
	{
		return;
	}
	if (TurnState == EE2GridTurnState::ResolvingPlayerAction)
	{
		FinishPlayerAction();
	}
	else if (TurnState == EE2GridTurnState::ResolvingEnemyAction)
	{
		FinishEnemyAction();
	}
}
