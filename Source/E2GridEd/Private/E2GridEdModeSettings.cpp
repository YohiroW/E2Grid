#include "E2GridEdModeSettings.h"
#include "E2GridManager.h"
#include "E2GridMapAsset.h"

UE2GridEdModeSettings::UE2GridEdModeSettings()
{
	ResetToDefaults();
}

void UE2GridEdModeSettings::ResetToDefaults()
{
	GridManagerClass = AE2GridManager::StaticClass();
	GridDimension = FIntPoint(10, 10);
	GridSize = 50;
	Location = FVector::ZeroVector;
	Rotation = FRotator::ZeroRotator;
}

void UE2GridEdModeSettings::LoadFromGridManager(const AE2GridManager& InGridManager)
{
	Location = InGridManager.GetActorLocation();
	Rotation = InGridManager.GetActorRotation();
	if (const FE2GridMapLayout* Layout = InGridManager.GetLayout())
	{
		GridDimension = Layout->GridDimension;
		GridSize = FMath::RoundToInt(Layout->CellSize);
	}
	else
	{
		GridDimension = FIntPoint(10, 10);
		GridSize = 50;
	}
}

bool UE2GridEdModeSettings::MatchesGridManager(const AE2GridManager& InGridManager) const
{
	if (!Location.Equals(InGridManager.GetActorLocation()) ||
		!Rotation.Equals(InGridManager.GetActorRotation()))
	{
		return false;
	}

	return true;
}

bool UE2GridEdModeSettings::IsValid() const
{
	return GridDimension.X > 0 && GridDimension.Y > 0 && GridSize > 0;
}
