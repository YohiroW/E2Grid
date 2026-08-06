#include "E2GridEdModeGridSettings.h"

void UE2GridEdModeGridSettings::Reset()
{
	CellKey = INVALID_GRID_KEY;
	CellData = FE2GridCellData();
}

void UE2GridEdModeGridSettings::LoadFrom(int32 InCellKey, const FE2GridCellData& InCellData)
{
	CellKey = InCellKey;
	CellData = InCellData;
}
