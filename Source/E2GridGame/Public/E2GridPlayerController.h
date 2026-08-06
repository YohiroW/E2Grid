#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "E2GridPlayerController.generated.h"

class AE2GridCombatUnit;

UCLASS()
class E2GRIDGAME_API AE2GridPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AE2GridPlayerController();
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void HandlePrimaryClick();
	void HandleSkipTurn();
	bool TraceCursor(FHitResult& OutHit) const;
	void DrawPathPreview();

	UPROPERTY(Transient)
	TObjectPtr<AE2GridCombatUnit> SelectedUnit;
};
