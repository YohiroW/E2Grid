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
	void LoadFrom(const FE2GridRuntimeData& InGridData);

	UPROPERTY(VisibleAnywhere, Category = "Selected Grid", meta = (ShowOnlyInnerProperties))
	FE2GridRuntimeData RuntimeData;
};
