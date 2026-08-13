#include "E2GridCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"

AE2GridCameraPawn::AE2GridCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 1000.0f;
	SpringArm->SetRelativeRotation(FRotator(-55.0f, -45.0f, 0.0f));
	SpringArm->bDoCollisionTest = false;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AE2GridCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxisKey(EKeys::W, this, &AE2GridCameraPawn::MoveForward);
	PlayerInputComponent->BindAxisKey(EKeys::S, this, &AE2GridCameraPawn::MoveBackward);
	PlayerInputComponent->BindAxisKey(EKeys::D, this, &AE2GridCameraPawn::MoveRight);
	PlayerInputComponent->BindAxisKey(EKeys::A, this, &AE2GridCameraPawn::MoveLeft);
	PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AE2GridCameraPawn::Zoom);
}

void AE2GridCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector Offset = (FVector::ForwardVector * PendingPan.Y + FVector::RightVector * PendingPan.X)
		.GetClampedToMaxSize(1.0f) * PanSpeed * DeltaSeconds;
	AddActorWorldOffset(Offset);
	SpringArm->TargetArmLength = FMath::Clamp(
		SpringArm->TargetArmLength - PendingZoom * ZoomSpeed * DeltaSeconds,
		FMath::Min(ZoomRange.X, ZoomRange.Y),
		FMath::Max(ZoomRange.X, ZoomRange.Y));
	PendingPan = FVector2D::ZeroVector;
	PendingZoom = 0.0f;
}

void AE2GridCameraPawn::MoveForward(float Value) { PendingPan.Y += Value; }
void AE2GridCameraPawn::MoveBackward(float Value) { PendingPan.Y -= Value; }
void AE2GridCameraPawn::MoveRight(float Value) { PendingPan.X += Value; }
void AE2GridCameraPawn::MoveLeft(float Value) { PendingPan.X -= Value; }
void AE2GridCameraPawn::Zoom(float Value) { PendingZoom += Value; }
