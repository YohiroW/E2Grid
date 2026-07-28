#include "E2GridEdModeSettings.h"
#include "E2GridManager.h"

UE2GridEdModeSettings::UE2GridEdModeSettings()
{
	ResetToDefaults();
}

void UE2GridEdModeSettings::ResetToDefaults()
{
	GridManagerClass = AE2GridManager::StaticClass();
	const AE2GridManager* DefaultManager = GetDefault<AE2GridManager>();
	GridDimension = DefaultManager->GridDimension;
	GridSize = DefaultManager->GridSize;
	Location = FVector::ZeroVector;
	Rotation = FRotator::ZeroRotator;
}

void UE2GridEdModeSettings::LoadFromGridManager(const AE2GridManager& InGridManager)
{
	Location = InGridManager.GetActorLocation();
	Rotation = InGridManager.GetActorRotation();
	GridDimension = InGridManager.GridDimension;
	GridSize = InGridManager.GridSize;
}

bool UE2GridEdModeSettings::MatchesGridManager(const AE2GridManager& InGridManager) const
{
	return Location.Equals(InGridManager.GetActorLocation()) &&
		Rotation.Equals(InGridManager.GetActorRotation()) &&
		GridDimension == InGridManager.GridDimension &&
		GridSize == InGridManager.GridSize;
}

bool UE2GridEdModeSettings::IsValid() const
{
	return GridDimension.X > 0 && GridDimension.Y > 0 && GridSize > 0;
}
