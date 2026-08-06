#include "E2GridMapAsset.h"

void UE2GridMapAsset::ReplaceData(const FE2GridMapLayout& InLayout, TMap<int32, FE2GridCellData>&& InCells)
{
	Layout = InLayout;
	CellsByKey = MoveTemp(InCells);
}
