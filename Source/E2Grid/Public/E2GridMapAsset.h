#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "E2GridRuntimeData.h"
#include "E2GridMapAsset.generated.h"

UCLASS(BlueprintType)
class E2GRID_API UE2GridMapAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FE2GridMapLayout& GetLayout() const { return Layout; }
	const TMap<int32, FE2GridCellData>& GetCells() const { return CellsByKey; }
	const FE2GridCellData* FindCell(int32 CellKey) const { return CellsByKey.Find(CellKey); }
	bool IsValidMap() const { return Layout.IsValid() && !CellsByKey.IsEmpty(); }

	/** Used by the editor builder only after all temporary data has validated. */
	void ReplaceData(const FE2GridMapLayout& InLayout, TMap<int32, FE2GridCellData>&& InCells);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	FE2GridMapLayout Layout;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	TMap<int32, FE2GridCellData> CellsByKey;
};
