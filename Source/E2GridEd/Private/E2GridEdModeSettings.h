#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "E2GridEdModeSettings.generated.h"

class AE2GridManager;

UCLASS(Transient)
class UE2GridEdModeSettings : public UObject
{
	GENERATED_BODY()

public:
	UE2GridEdModeSettings();

	void ResetToDefaults();
	void LoadFromGridManager(const AE2GridManager& InGridManager);
	bool MatchesGridManager(const AE2GridManager& InGridManager) const;
	bool IsValid() const;

	UPROPERTY(EditAnywhere, Category = "Transform")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Transform")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint GridDimension = FIntPoint(10, 10);

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridSize = 100;

};
