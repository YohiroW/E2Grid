#include "E2GridPlayerController.h"

#include "DrawDebugHelpers.h"
#include "Components/InputComponent.h"
#include "E2GridCombatUnit.h"
#include "E2GridGameModeBase.h"
#include "E2GridMovementComponent.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

AE2GridPlayerController::AE2GridPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AE2GridPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AE2GridPlayerController::HandlePrimaryClick);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AE2GridPlayerController::HandleSkipTurn);
}

void AE2GridPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	DrawPathPreview();
}

void AE2GridPlayerController::HandlePrimaryClick()
{
	FHitResult Hit;
	if (!TraceCursor(Hit))
	{
		return;
	}

	AE2GridGameModeBase* GridGameMode = GetWorld()->GetAuthGameMode<AE2GridGameModeBase>();
	if (!GridGameMode || GridGameMode->GetTurnState() != EE2GridTurnState::PlayerTurn)
	{
		return;
	}

	if (AE2GridCombatUnit* ClickedUnit = Cast<AE2GridCombatUnit>(Hit.GetActor()))
	{
		if (ClickedUnit->GetTeam() == EE2GridTeam::Player)
		{
			SelectedUnit = ClickedUnit;
			return;
		}
		if (SelectedUnit && ClickedUnit->GetTeam() == EE2GridTeam::Enemy)
		{
			GridGameMode->RequestPlayerAttack(SelectedUnit, ClickedUnit);
			return;
		}
	}

	if (!SelectedUnit)
	{
		return;
	}
	if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		int32 GoalCellKey = INVALID_GRID_KEY;
		if (GridSubsystem->WorldToCell(Hit.ImpactPoint, GoalCellKey))
		{
			GridGameMode->RequestPlayerMove(SelectedUnit, GoalCellKey);
		}
	}
}

void AE2GridPlayerController::HandleSkipTurn()
{
	if (AE2GridGameModeBase* GridGameMode = GetWorld()->GetAuthGameMode<AE2GridGameModeBase>())
	{
		GridGameMode->SkipPlayerAction();
	}
}

bool AE2GridPlayerController::TraceCursor(FHitResult& OutHit) const
{
	FVector RayOrigin;
	FVector RayDirection;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		return false;
	}
	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		RayOrigin,
		RayOrigin + RayDirection * 100000.0f,
		ECC_Visibility);
}

void AE2GridPlayerController::DrawPathPreview()
{
	if (!SelectedUnit || !SelectedUnit->IsAlive())
	{
		SelectedUnit = nullptr;
		return;
	}

	FHitResult Hit;
	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	int32 GoalCellKey = INVALID_GRID_KEY;
	if (!GridSubsystem || !TraceCursor(Hit) || !GridSubsystem->WorldToCell(Hit.ImpactPoint, GoalCellKey))
	{
		return;
	}

	FE2GridPathResult Path;
	if (!GridSubsystem->FindPath(SelectedUnit->GetGridUnitComponent(), GoalCellKey, Path))
	{
		return;
	}

	FVector PreviousPoint = SelectedUnit->GetActorLocation();
	for (int32 CellKey : Path.CellKeys)
	{
		FVector CellCenter;
		if (GridSubsystem->CellToWorld(CellKey, CellCenter))
		{
			DrawDebugLine(GetWorld(), PreviousPoint, CellCenter + FVector(0.0, 0.0, 4.0), FColor::Cyan, false, 0.0f, 0, 4.0f);
			PreviousPoint = CellCenter + FVector(0.0, 0.0, 4.0);
		}
	}
}
