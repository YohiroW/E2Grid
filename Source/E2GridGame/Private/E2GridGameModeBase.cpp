#include "E2GridGameModeBase.h"

#include "E2GridCameraPawn.h"
#include "E2GridCombatUnit.h"
#include "E2GridGamePlayerController.h"
#include "E2GridManager.h"
#include "E2GridMovementComponent.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "EngineUtils.h"

AE2GridGameModeBase::AE2GridGameModeBase()
{
	DefaultPawnClass = AE2GridCameraPawn::StaticClass();
	PlayerControllerClass = AE2GridGamePlayerController::StaticClass();
}

void AE2GridGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	DiscoverCombatants();
}

bool AE2GridGameModeBase::CanPlayerIssueAction() const
{
	return TurnState == EE2GridTurnState::PlayerTurn &&
		PlayerUnit.IsValid() && EnemyUnit.IsValid() && PlayerUnit->IsAlive() && EnemyUnit->IsAlive();
}

bool AE2GridGameModeBase::SubmitPlayerMove(AE2GridCombatUnit* Unit, int32 GoalCellKey)
{
	if (!CanPlayerIssueAction() || Unit != PlayerUnit.Get())
	{
		return false;
	}

	TurnState = EE2GridTurnState::ResolvingPlayerAction;
	if (!Unit->GetGridMovement()->MoveToCell(GoalCellKey))
	{
		TurnState = EE2GridTurnState::PlayerTurn;
		return false;
	}
	return true;
}

bool AE2GridGameModeBase::SubmitPlayerAttack(
	AE2GridCombatUnit* Attacker,
	AE2GridCombatUnit* Target)
{
	if (!CanPlayerIssueAction() || Attacker != PlayerUnit.Get() || Target != EnemyUnit.Get() ||
		!TryAttack(*Attacker, *Target))
	{
		return false;
	}

	TurnState = EE2GridTurnState::ResolvingPlayerAction;
	FinishPlayerAction();
	return true;
}

bool AE2GridGameModeBase::SkipPlayerAction()
{
	if (!CanPlayerIssueAction())
	{
		return false;
	}
	TurnState = EE2GridTurnState::ResolvingPlayerAction;
	FinishPlayerAction();
	return true;
}

bool AE2GridGameModeBase::IsTraversableAdjacent(
	const AE2GridCombatUnit& From,
	const AE2GridCombatUnit& To) const
{
	const UE2GridUnitComponent* FromGridUnit = From.GetGridUnit();
	const UE2GridUnitComponent* ToGridUnit = To.GetGridUnit();
	const UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	const AE2GridManager* Manager = GridSubsystem ? GridSubsystem->GetActiveManager() : nullptr;
	if (!Manager || !FromGridUnit || !ToGridUnit ||
		!FromGridUnit->IsRegistered() || !ToGridUnit->IsRegistered())
	{
		return false;
	}

	bool bAdjacent = false;
	const int32 TargetCellKey = ToGridUnit->GetCurrentCellKey();
	Manager->ForEachTraversableNeighbor(
		FromGridUnit->GetCurrentCellKey(),
		[TargetCellKey, &bAdjacent](int32 NeighborKey, float)
		{
			bAdjacent |= NeighborKey == TargetCellKey;
		});
	return bAdjacent;
}

bool AE2GridGameModeBase::TryAttack(AE2GridCombatUnit& Attacker, AE2GridCombatUnit& Target)
{
	if (!Attacker.IsAlive() || !Target.IsAlive() || Attacker.GetTeam() == Target.GetTeam() ||
		Attacker.GetTeam() == EE2GridTeam::Neutral || Target.GetTeam() == EE2GridTeam::Neutral ||
		!IsTraversableAdjacent(Attacker, Target))
	{
		return false;
	}

	Target.ApplyFixedDamage(AttackDamage);
	return true;
}

void AE2GridGameModeBase::DiscoverCombatants()
{
	PlayerUnit.Reset();
	EnemyUnit.Reset();
	for (TActorIterator<AE2GridCombatUnit> It(GetWorld()); It; ++It)
	{
		AE2GridCombatUnit* Combatant = *It;
		if (Combatant->GetTeam() == EE2GridTeam::Player && !PlayerUnit.IsValid())
		{
			PlayerUnit = Combatant;
			Combatant->GetGridMovement()->OnMovementFinished.AddDynamic(
				this,
				&AE2GridGameModeBase::HandlePlayerMovementFinished);
		}
		else if (Combatant->GetTeam() == EE2GridTeam::Enemy && !EnemyUnit.IsValid())
		{
			EnemyUnit = Combatant;
			Combatant->GetGridMovement()->OnMovementFinished.AddDynamic(
				this,
				&AE2GridGameModeBase::HandleEnemyMovementFinished);
		}
	}
}

void AE2GridGameModeBase::FinishPlayerAction()
{
	StartEnemyTurn();
}

void AE2GridGameModeBase::StartEnemyTurn()
{
	TurnState = EE2GridTurnState::EnemyTurn;
	ResolveEnemyTurn();
}

void AE2GridGameModeBase::ResolveEnemyTurn()
{
	AE2GridCombatUnit* Player = PlayerUnit.Get();
	AE2GridCombatUnit* Enemy = EnemyUnit.Get();
	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	AE2GridManager* Manager = GridSubsystem ? GridSubsystem->GetActiveManager() : nullptr;
	if (!Player || !Enemy || !Player->IsAlive() || !Enemy->IsAlive() || !GridSubsystem || !Manager)
	{
		FinishEnemyAction();
		return;
	}

	TurnState = EE2GridTurnState::ResolvingEnemyAction;
	if (TryAttack(*Enemy, *Player))
	{
		FinishEnemyAction();
		return;
	}

	const UE2GridUnitComponent* PlayerGridUnit = Player->GetGridUnit();
	if (!PlayerGridUnit || !PlayerGridUnit->IsRegistered())
	{
		FinishEnemyAction();
		return;
	}

	int32 BestGoal = INVALID_GRID_KEY;
	float BestCost = TNumericLimits<float>::Max();
	const int32 PlayerCellKey = PlayerGridUnit->GetCurrentCellKey();
	Manager->ForEachCell(
		[&](int32 CandidateKey, const FE2GridCellData& CandidateCell, const FVector&)
		{
			if (!CandidateCell.CanStandOn() ||
				!GridSubsystem->CanPlaceUnit(Enemy->GetGridUnit(), CandidateKey))
			{
				return;
			}

			bool bCanAttackPlayerFromCandidate = false;
			Manager->ForEachTraversableNeighbor(
				CandidateKey,
				[PlayerCellKey, &bCanAttackPlayerFromCandidate](int32 NeighborKey, float)
				{
					bCanAttackPlayerFromCandidate |= NeighborKey == PlayerCellKey;
				});
			if (!bCanAttackPlayerFromCandidate)
			{
				return;
			}

			FE2GridPathResult CandidatePath;
			if (GridSubsystem->FindPath(Enemy->GetGridUnit(), CandidateKey, CandidatePath) &&
				(CandidatePath.TotalCost < BestCost ||
					(FMath::IsNearlyEqual(CandidatePath.TotalCost, BestCost) &&
						(BestGoal == INVALID_GRID_KEY || CandidateKey < BestGoal))))
			{
				BestGoal = CandidateKey;
				BestCost = CandidatePath.TotalCost;
			}
		});

	if (BestGoal == INVALID_GRID_KEY || !Enemy->GetGridMovement()->MoveToCell(BestGoal))
	{
		FinishEnemyAction();
	}
}

void AE2GridGameModeBase::FinishEnemyAction()
{
	TurnState = EE2GridTurnState::PlayerTurn;
}

void AE2GridGameModeBase::HandlePlayerMovementFinished(bool bSucceeded, int32)
{
	if (TurnState != EE2GridTurnState::ResolvingPlayerAction)
	{
		return;
	}
	if (bSucceeded)
	{
		FinishPlayerAction();
	}
	else
	{
		TurnState = EE2GridTurnState::PlayerTurn;
	}
}

void AE2GridGameModeBase::HandleEnemyMovementFinished(bool, int32)
{
	if (TurnState == EE2GridTurnState::ResolvingEnemyAction)
	{
		FinishEnemyAction();
	}
}
