#include "E2GridCombatUnit.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "E2GridMovementComponent.h"
#include "E2GridUnitComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AE2GridCombatUnit::AE2GridCombatUnit()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	CollisionComponent->SetupAttachment(SceneRoot);
	CollisionComponent->SetCapsuleSize(20.0f, 40.0f);
	CollisionComponent->SetRelativeLocation(FVector(0.0, 0.0, 40.0));
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetRelativeLocation(FVector(0.0, 0.0, 40.0));
	VisualMesh->SetRelativeScale3D(FVector(0.4, 0.4, 0.8));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	VisualMesh->SetStaticMesh(DefaultMesh.Object);

	GridUnitComponent = CreateDefaultSubobject<UE2GridUnitComponent>(TEXT("GridUnit"));
	GridMovementComponent = CreateDefaultSubobject<UE2GridMovementComponent>(TEXT("GridMovement"));
}

void AE2GridCombatUnit::BeginPlay()
{
	Health = MaxHealth;
	Super::BeginPlay();
}

void AE2GridCombatUnit::ApplyFixedDamage(int32 Damage)
{
	if (Damage <= 0 || !IsAlive())
	{
		return;
	}

	const int32 PreviousHealth = Health;
	Health = FMath::Clamp(Health - Damage, 0, MaxHealth);
	OnHealthChanged.Broadcast(Health, Health - PreviousHealth);
	if (Health == 0)
	{
		Destroy();
	}
}
