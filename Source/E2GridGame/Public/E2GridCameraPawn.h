#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "E2GridCameraPawn.generated.h"

class UCameraComponent;

UCLASS()
class E2GRIDGAME_API AE2GridCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AE2GridCameraPawn();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0"))
	float CameraMoveSpeed = 800.0f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
};
