#include "E2GridCombatUnit.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "E2GridMovementComponent.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AE2GridCombatUnit::AE2GridCombatUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	CollisionComponent->SetupAttachment(SceneRoot);
	CollisionComponent->InitCapsuleSize(20.0f, 40.0f);
	CollisionComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 40.5f));
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 40.5f));
	VisualMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.8f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMaterial.Succeeded())
	{
		BaseVisualMaterial = BaseMaterial.Object;
	}

	TeamLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TeamLabel"));
	TeamLabel->SetupAttachment(SceneRoot);
	TeamLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	TeamLabel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	TeamLabel->SetHorizontalAlignment(EHTA_Center);
	TeamLabel->SetWorldSize(24.0f);
	TeamLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GridUnit = CreateDefaultSubobject<UE2GridUnitComponent>(TEXT("GridUnit"));
	GridMovement = CreateDefaultSubobject<UE2GridMovementComponent>(TEXT("GridMovement"));
}

void AE2GridCombatUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTeamVisuals();
}

void AE2GridCombatUnit::BeginPlay()
{
	Health = FMath::Max(1, MaxHealth);
	Super::BeginPlay();
	ApplyTeamVisuals();
}

bool AE2GridCombatUnit::SetTeamForSetup(EE2GridTeam InTeam)
{
	if (HasActorBegunPlay())
	{
		return false;
	}
	Team = InTeam;
	ApplyTeamVisuals();
	return true;
}

void AE2GridCombatUnit::ApplyTeamVisuals()
{
	FLinearColor TeamColor = FLinearColor(0.55f, 0.55f, 0.55f);
	FText Label = NSLOCTEXT("E2GridGame", "NeutralTeam", "Neutral");
	switch (Team)
	{
	case EE2GridTeam::Player:
		TeamColor = FLinearColor(0.08f, 0.45f, 1.0f);
		Label = NSLOCTEXT("E2GridGame", "PlayerTeam", "Player");
		break;
	case EE2GridTeam::Enemy:
		TeamColor = FLinearColor(1.0f, 0.08f, 0.04f);
		Label = NSLOCTEXT("E2GridGame", "EnemyTeam", "Enemy");
		break;
	case EE2GridTeam::Neutral:
	default:
		break;
	}

	if (TeamLabel)
	{
		TeamLabel->SetText(Label);
		TeamLabel->SetTextRenderColor(TeamColor.ToFColor(true));
	}

	if (VisualMesh && VisualMesh->GetStaticMesh())
	{
		if (BaseVisualMaterial)
		{
			if (!TeamMaterial)
			{
				TeamMaterial = UMaterialInstanceDynamic::Create(BaseVisualMaterial, this);
			}
			TeamMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
			VisualMesh->SetMaterial(0, TeamMaterial);
		}
	}
}

void AE2GridCombatUnit::ApplyFixedDamage(int32 Damage)
{
	if (Damage <= 0 || !IsAlive())
	{
		return;
	}

	Health = FMath::Max(0, Health - Damage);
	if (Health == 0)
	{
		GridMovement->CancelMove();
		if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
		{
			GridSubsystem->UnregisterUnit(GridUnit);
		}
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetActorHiddenInGame(true);
		OnDefeated.Broadcast(this);
	}
}
