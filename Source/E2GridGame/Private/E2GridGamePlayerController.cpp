#include "E2GridGamePlayerController.h"

#include "Components/InputComponent.h"
#include "E2GridCombatUnit.h"
#include "E2GridGameModeBase.h"
#include "E2GridRuntimeData.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

AE2GridGamePlayerController::AE2GridGamePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AE2GridGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(
		EKeys::LeftMouseButton,
		IE_Pressed,
		this,
		&AE2GridGamePlayerController::HandleConfirmInput);
	InputComponent->BindKey(
		EKeys::SpaceBar,
		IE_Pressed,
		this,
		&AE2GridGamePlayerController::HandleSkipInput);
}

void AE2GridGamePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateHoverAndPreview();
	DrawPathPreview();
}

void AE2GridGamePlayerController::DrawPathPreview() const
{
	if (!SelectedUnit || PreviewCellKeys.IsEmpty())
	{
		return;
	}

	const UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	FVector PreviousPosition;
	if (!GridSubsystem || !GridSubsystem->CellToWorld(
		SelectedUnit->GetGridUnit()->GetCurrentCellKey(),
		PreviousPosition))
	{
		return;
	}
	PreviousPosition.Z += PreviewHeightOffset;

	for (const int32 CellKey : PreviewCellKeys)
	{
		FVector CellPosition;
		if (!GridSubsystem->CellToWorld(CellKey, CellPosition))
		{
			return;
		}
		CellPosition.Z += PreviewHeightOffset;
		DrawDebugLine(
			GetWorld(),
			PreviousPosition,
			CellPosition,
			PreviewColor,
			false,
			0.0f,
			0,
			PreviewThickness);
		DrawDebugPoint(GetWorld(), CellPosition, PreviewThickness * 2.0f, PreviewColor, false, 0.0f);
		PreviousPosition = CellPosition;
	}
}

void AE2GridGamePlayerController::SelectUnit(AE2GridCombatUnit* Unit)
{
	SelectedUnit = Unit && Unit->GetTeam() == EE2GridTeam::Player ? Unit : nullptr;
}

bool AE2GridGamePlayerController::ConfirmHoveredAction()
{
	AE2GridGameModeBase* GridGameMode = GetWorld()->GetAuthGameMode<AE2GridGameModeBase>();
	if (!GridGameMode || !GridGameMode->CanPlayerIssueAction())
	{
		return false;
	}
	if (!SelectedUnit)
	{
		SelectUnit(GridGameMode->GetPlayerUnit());
	}
	if (!SelectedUnit || HoveredCellKey == INVALID_GRID_KEY)
	{
		return false;
	}

	if (AE2GridCombatUnit* Target = ResolveCombatUnitAtCell(HoveredCellKey))
	{
		return GridGameMode->SubmitPlayerAttack(SelectedUnit, Target);
	}
	return GridGameMode->SubmitPlayerMove(SelectedUnit, HoveredCellKey);
}

bool AE2GridGamePlayerController::SkipAction()
{
	if (AE2GridGameModeBase* GridGameMode = GetWorld()->GetAuthGameMode<AE2GridGameModeBase>())
	{
		return GridGameMode->SkipPlayerAction();
	}
	return false;
}

void AE2GridGamePlayerController::UpdateHoverAndPreview()
{
	HoveredCellKey = INVALID_GRID_KEY;
	PreviewCellKeys.Reset();
	FHitResult CursorHit;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, CursorHit))
	{
		return;
	}

	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	AE2GridCombatUnit* HitCombatUnit = Cast<AE2GridCombatUnit>(CursorHit.GetActor());
	if (HitCombatUnit && HitCombatUnit->GetGridUnit()->IsRegistered())
	{
		HoveredCellKey = HitCombatUnit->GetGridUnit()->GetCurrentCellKey();
	}
	else if (!GridSubsystem || !GridSubsystem->WorldToCell(CursorHit.ImpactPoint, HoveredCellKey))
	{
		HoveredCellKey = INVALID_GRID_KEY;
		return;
	}

	AE2GridGameModeBase* GridGameMode = GetWorld()->GetAuthGameMode<AE2GridGameModeBase>();
	if (!SelectedUnit && GridGameMode)
	{
		SelectUnit(GridGameMode->GetPlayerUnit());
	}
	if (!SelectedUnit || ResolveCombatUnitAtCell(HoveredCellKey))
	{
		return;
	}

	FE2GridPathResult PreviewPath;
	if (GridSubsystem->FindPath(SelectedUnit->GetGridUnit(), HoveredCellKey, PreviewPath))
	{
		for (const FE2GridPathStep& Step : PreviewPath.Steps)
		{
			PreviewCellKeys.Add(Step.ToCellKey);
		}
	}
}

void AE2GridGamePlayerController::HandleConfirmInput()
{
	ConfirmHoveredAction();
}

void AE2GridGamePlayerController::HandleSkipInput()
{
	SkipAction();
}

AE2GridCombatUnit* AE2GridGamePlayerController::ResolveCombatUnitAtCell(int32 CellKey) const
{
	UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>();
	UE2GridUnitComponent* GridUnit = GridSubsystem ? GridSubsystem->GetCellOwner(CellKey) : nullptr;
	return GridUnit ? Cast<AE2GridCombatUnit>(GridUnit->GetOwner()) : nullptr;
}
