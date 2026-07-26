#include "E2GridRuntimeData.h"

const FE2GridCoord FE2GridCoord::INVALID_COORD(INVALID_GRID_KEY, INVALID_GRID_KEY,INVALID_GRID_KEY);

bool FE2GridRuntimeData::HasGridFlag(EE2GridFlags Flag) const
{
	return (GridFlags & static_cast<int32>(Flag)) != 0;
}

void FE2GridRuntimeData::SetGridFlag(EE2GridFlags Flag, const bool bEnabled)
{
	if (bEnabled)
	{
		GridFlags |= static_cast<int32>(Flag);
	}
	else
	{
		GridFlags &= ~static_cast<int32>(Flag);
	}
}

void FE2GridRuntimeData::SetWalkable(const bool bInIsWalkable)
{
	SetGridFlag(EE2GridFlags::IsWalkable, bInIsWalkable);
}

bool FE2GridRuntimeData::IsWalkable() const
{
	return HasGridFlag(EE2GridFlags::IsWalkable);
}

void FE2GridRuntimeData::SetCanWalkThrough(const bool bInCanWalkThrough)
{
	SetGridFlag(EE2GridFlags::CanWalkThrough, bInCanWalkThrough);
}

bool FE2GridRuntimeData::CanWalkThrough() const
{
	return HasGridFlag(EE2GridFlags::CanWalkThrough);
}

