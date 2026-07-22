#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "E2GridCreateSettings.generated.h"

class UE2GridRuntimeData;

UCLASS(Transient)
class UE2GridCreateSettings : public UObject
{
	GENERATED_BODY()

public:
	UE2GridCreateSettings();

	void ResetToDefaults();

	UPROPERTY(EditAnywhere, Category = "Transform")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Transform")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint GridDimension = FIntPoint(10, 10);

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridSize = 100;

	UPROPERTY(EditAnywhere, Category = "Grid")
	TSubclassOf<UE2GridRuntimeData> GridDataClass;
};
