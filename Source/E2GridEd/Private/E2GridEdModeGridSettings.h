#pragma once

#include "CoreMinimal.h"
#include "E2GridRuntimeData.h"
#include "UObject/Object.h"
#include "E2GridEdModeGridSettings.generated.h"

UCLASS(Transient)
class UE2GridEdModeGridSettings : public UObject
{
	GENERATED_BODY()

public:
	void Reset();
	void LoadFrom(int32 InCellKey, const FE2GridCellData& InCellData);

	UPROPERTY(VisibleAnywhere, Category = "Selected Cell")
	int32 CellKey = INVALID_GRID_KEY;

	UPROPERTY(VisibleAnywhere, Category = "Selected Cell", meta = (ShowOnlyInnerProperties))
	FE2GridCellData CellData;
};
