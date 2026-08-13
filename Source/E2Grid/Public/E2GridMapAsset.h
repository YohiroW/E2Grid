#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "E2GridRuntimeData.h"
#include "E2GridMapAsset.generated.h"

constexpr int32 E2GRID_MAP_BUILD_VERSION = 1;

UCLASS(BlueprintType)
class E2GRID_API UE2GridMapAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FE2GridMapLayout& GetLayout() const { return Layout; }
	const TMap<int32, FE2GridCellData>& GetCells() const { return CellsByKey; }
	const FE2GridCellData* FindCell(int32 CellKey) const { return CellsByKey.Find(CellKey); }
	int32 GetBuildVersion() const { return BuildVersion; }

	bool IsCurrentBuildVersion() const { return BuildVersion == E2GRID_MAP_BUILD_VERSION; }
	bool Validate(FString* OutError = nullptr) const;
	bool IsValidMap() const { return Validate(); }

	/** Called by the editor builder only after all temporary data has validated. */
	void ReplaceData(const FE2GridMapLayout& InLayout, TMap<int32, FE2GridCellData>&& InCells);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	FE2GridMapLayout Layout;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	TMap<int32, FE2GridCellData> CellsByKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	int32 BuildVersion = 0;
};
