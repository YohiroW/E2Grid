#include "E2GridEdModeGridSettings.h"

void UE2GridEdModeGridSettings::Reset()
{
	CellKey = INVALID_GRID_KEY;
	Coord = FE2GridCoord::INVALID_COORD;
	CellData = FE2GridCellData();
}

void UE2GridEdModeGridSettings::LoadFrom(
	int32 InCellKey,
	const FE2GridCoord& InCoord,
	const FE2GridCellData& InCellData)
{
	CellKey = InCellKey;
	Coord = InCoord;
	CellData = InCellData;
}
