#include "E2GridCreateSettings.h"

#include "E2GridManager.h"

UE2GridCreateSettings::UE2GridCreateSettings()
{
	ResetToDefaults();
}

void UE2GridCreateSettings::ResetToDefaults()
{
	const AE2GridManager* DefaultManager = GetDefault<AE2GridManager>();
	GridDimension = DefaultManager->GridDimension;
	GridSize = DefaultManager->GridSize;
	GridDataClass = DefaultManager->GridDataClass;
	Location = FVector::ZeroVector;
	Rotation = FRotator::ZeroRotator;
}
