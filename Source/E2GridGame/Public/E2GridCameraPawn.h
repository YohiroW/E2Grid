#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "E2GridCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class E2GRIDGAME_API AE2GridCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AE2GridCameraPawn();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0"))
	float PanSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0"))
	float ZoomSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector2D ZoomRange = FVector2D(400.0f, 2000.0f);

private:
	void MoveForward(float Value);
	void MoveBackward(float Value);
	void MoveRight(float Value);
	void MoveLeft(float Value);
	void Zoom(float Value);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	FVector2D PendingPan = FVector2D::ZeroVector;
	float PendingZoom = 0.0f;
};
