#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "E2GridGamePlayerController.generated.h"

class AE2GridCombatUnit;

UCLASS(Blueprintable)
class E2GRIDGAME_API AE2GridGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AE2GridGamePlayerController();

	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "E2Grid Game")
	int32 GetHoveredCellKey() const { return HoveredCellKey; }

	UFUNCTION(BlueprintPure, Category = "E2Grid Game")
	const TArray<int32>& GetPreviewCellKeys() const { return PreviewCellKeys; }

	UFUNCTION(BlueprintCallable, Category = "E2Grid Game")
	void SelectUnit(AE2GridCombatUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "E2Grid Game")
	bool ConfirmHoveredAction();

	UFUNCTION(BlueprintCallable, Category = "E2Grid Game")
	bool SkipAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "E2Grid Game|Preview")
	FColor PreviewColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "E2Grid Game|Preview", meta = (ClampMin = "0.0"))
	float PreviewHeightOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "E2Grid Game|Preview", meta = (ClampMin = "0.1"))
	float PreviewThickness = 5.0f;

private:
	void UpdateHoverAndPreview();
	void DrawPathPreview() const;
	void HandleConfirmInput();
	void HandleSkipInput();
	AE2GridCombatUnit* ResolveCombatUnitAtCell(int32 CellKey) const;

	UPROPERTY(Transient)
	TObjectPtr<AE2GridCombatUnit> SelectedUnit;

	UPROPERTY(VisibleInstanceOnly, Category = "E2Grid Game")
	int32 HoveredCellKey = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "E2Grid Game")
	TArray<int32> PreviewCellKeys;
};
