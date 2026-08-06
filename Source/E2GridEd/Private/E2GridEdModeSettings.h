#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
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
	
	UPROPERTY(EditAnywhere, Category = "Transform", meta=(DisplayName="Location"))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Transform", meta=(DisplayName="Rotation"))
	FRotator Rotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, Category = "Transform", meta=(DisplayName="Transform"))
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "Grid")
	TSubclassOf<AE2GridManager> GridManagerClass;
	
	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint GridDimension = FIntPoint(10, 10);

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridSize = 50;
	
	UPROPERTY(EditAnywhere, Category = "Grid")
	bool bShowPreview = true;
};
