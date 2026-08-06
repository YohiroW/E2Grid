#include "E2GridCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

AE2GridCameraPawn::AE2GridCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SceneRoot);
	CameraComponent->SetRelativeLocation(FVector(0.0, 0.0, 900.0));
	CameraComponent->SetRelativeRotation(FRotator(-60.0, 0.0, 0.0));
}

void AE2GridCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	FVector Direction = FVector::ZeroVector;
	Direction.X += PlayerController->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f;
	Direction.X -= PlayerController->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f;
	Direction.Y += PlayerController->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f;
	Direction.Y -= PlayerController->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f;
	AddActorWorldOffset(Direction.GetClampedToMaxSize(1.0f) * CameraMoveSpeed * DeltaSeconds);
}
