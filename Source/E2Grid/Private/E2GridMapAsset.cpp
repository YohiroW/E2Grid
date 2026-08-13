#include "E2GridMapAsset.h"

namespace
{
	bool FailValidation(FString* OutError, const FString& Error)
	{
		if (OutError)
		{
			*OutError = Error;
		}
		return false;
	}
}

bool UE2GridMapAsset::Validate(FString* OutError) const
{
	if (!IsCurrentBuildVersion())
	{
		return FailValidation(
			OutError,
			FString::Printf(
				TEXT("Grid map build version %d does not match current version %d."),
				BuildVersion,
				E2GRID_MAP_BUILD_VERSION));
	}
	if (!Layout.IsValid())
	{
		return FailValidation(OutError, TEXT("Grid map layout is invalid."));
	}
	if (CellsByKey.IsEmpty())
	{
		return FailValidation(OutError, TEXT("Grid map contains no cells."));
	}

	for (const TPair<int32, FE2GridCellData>& Pair : CellsByKey)
	{
		constexpr int32 ValidCellFlags =
			static_cast<int32>(EE2GridCellFlags::CanWalkThrough) |
			static_cast<int32>(EE2GridCellFlags::CanStandOn);
		FE2GridCoord Coord;
		if (!Layout.KeyToCoord(Pair.Key, Coord) || !FMath::IsFinite(Pair.Value.LocalHeight))
		{
			return FailValidation(
				OutError,
				FString::Printf(TEXT("Cell %d has an invalid key or height."), Pair.Key));
		}
		if ((Pair.Value.Flags & ~ValidCellFlags) != 0)
		{
			return FailValidation(
				OutError,
				FString::Printf(TEXT("Cell %d contains unknown flags."), Pair.Key));
		}
		if (Pair.Value.NeighborMask != 0 && !Pair.Value.CanWalkThrough())
		{
			return FailValidation(
				OutError,
				FString::Printf(TEXT("Cell %d has traversal bits but cannot be walked through."), Pair.Key));
		}

		for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
		{
			if ((Pair.Value.NeighborMask & (1u << DirectionIndex)) == 0)
			{
				continue;
			}

			const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
			const int32 NeighborKey = Layout.CoordToKey(
				FE2GridCoord(Coord.X + Offset.X, Coord.Y + Offset.Y));
			const FE2GridCellData* NeighborCell = CellsByKey.Find(NeighborKey);
			if (NeighborKey == INVALID_GRID_KEY || !NeighborCell)
			{
				return FailValidation(
					OutError,
					FString::Printf(
						TEXT("Cell %d has a traversal bit that points to a missing cell."),
						Pair.Key));
			}
			if (!NeighborCell->CanWalkThrough())
			{
				return FailValidation(
					OutError,
					FString::Printf(
						TEXT("Cell %d traverses to cell %d, which cannot be walked through."),
						Pair.Key,
						NeighborKey));
			}
			if (E2GridDirections::IsDiagonal(DirectionIndex))
			{
				const int32 OrthogonalA =
					(DirectionIndex + E2GridDirections::Count - 1) % E2GridDirections::Count;
				const int32 OrthogonalB = (DirectionIndex + 1) % E2GridDirections::Count;
				if ((Pair.Value.NeighborMask & (1u << OrthogonalA)) == 0 ||
					(Pair.Value.NeighborMask & (1u << OrthogonalB)) == 0)
				{
					return FailValidation(
						OutError,
						FString::Printf(
							TEXT("Cell %d has a diagonal traversal that cuts a blocked corner."),
							Pair.Key));
				}
			}
		}
	}

	if (OutError)
	{
		OutError->Reset();
	}
	return true;
}

void UE2GridMapAsset::ReplaceData(
	const FE2GridMapLayout& InLayout,
	TMap<int32, FE2GridCellData>&& InCells)
{
	Layout = InLayout;
	CellsByKey = MoveTemp(InCells);
	BuildVersion = E2GRID_MAP_BUILD_VERSION;
}
