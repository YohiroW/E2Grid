#include "E2GridRuntimeData.h"

const FE2GridCoord FE2GridCoord::INVALID_COORD(INVALID_GRID_KEY, INVALID_GRID_KEY, INVALID_GRID_KEY);

namespace
{
	const FIntPoint DirectionOffsets[E2GridDirections::Count] =
	{
		FIntPoint(0, 1),
		FIntPoint(1, 1),
		FIntPoint(1, 0),
		FIntPoint(1, -1),
		FIntPoint(0, -1),
		FIntPoint(-1, -1),
		FIntPoint(-1, 0),
		FIntPoint(-1, 1),
	};
}

bool FE2GridMapLayout::IsValid() const
{
	return FMath::IsFinite(CellSize) && CellSize > UE_SMALL_NUMBER &&
		GridDimension.X > 0 && GridDimension.Y > 0;
}

bool FE2GridMapLayout::IsValidCoord(const FE2GridCoord& Coord) const
{
	return Coord.Layer == 0 && Coord.X >= 0 && Coord.X < GridDimension.X &&
		Coord.Y >= 0 && Coord.Y < GridDimension.Y;
}

int32 FE2GridMapLayout::CoordToKey(const FE2GridCoord& Coord) const
{
	return IsValidCoord(Coord) ? Coord.X + Coord.Y * GridDimension.X : INVALID_GRID_KEY;
}

bool FE2GridMapLayout::KeyToCoord(int32 CellKey, FE2GridCoord& OutCoord) const
{
	const int64 CellCount = static_cast<int64>(GridDimension.X) * GridDimension.Y;
	if (!IsValid() || CellKey < 0 || static_cast<int64>(CellKey) >= CellCount)
	{
		OutCoord = FE2GridCoord::INVALID_COORD;
		return false;
	}

	OutCoord = FE2GridCoord(CellKey % GridDimension.X, CellKey / GridDimension.X);
	return true;
}

FVector FE2GridMapLayout::GetCellLocalCenter(const FE2GridCoord& Coord, float LocalHeight) const
{
	return LocalOrigin + FVector(Coord.X * CellSize, Coord.Y * CellSize, LocalHeight);
}

bool FE2GridCellData::HasFlag(EE2GridCellFlags Flag) const
{
	return (Flags & static_cast<int32>(Flag)) != 0;
}

void FE2GridCellData::SetFlag(EE2GridCellFlags Flag, bool bEnabled)
{
	if (bEnabled)
	{
		Flags |= static_cast<int32>(Flag);
	}
	else
	{
		Flags &= ~static_cast<int32>(Flag);
	}
}

bool FE2GridBuildSettings::IsValid() const
{
	return AgentRadius >= 0.0f && AgentHeight > 0.0f && MaxSlope >= 0.0f && MaxSlope < 90.0f &&
		MaxStepHeight >= 0.0f && TraceStartHeight >= 0.0f && TraceDepth > 0.0f &&
		GroundChannel >= ECC_WorldStatic && GroundChannel < ECC_MAX &&
		ObstacleChannel >= ECC_WorldStatic && ObstacleChannel < ECC_MAX;
}

const FIntPoint& E2GridDirections::GetOffset(int32 DirectionIndex)
{
	check(DirectionIndex >= 0 && DirectionIndex < Count);
	return DirectionOffsets[DirectionIndex];
}

bool E2GridDirections::IsDiagonal(int32 DirectionIndex)
{
	return (DirectionIndex & 1) != 0;
}

int32 E2GridDirections::GetOpposite(int32 DirectionIndex)
{
	check(DirectionIndex >= 0 && DirectionIndex < Count);
	return (DirectionIndex + Count / 2) % Count;
}
