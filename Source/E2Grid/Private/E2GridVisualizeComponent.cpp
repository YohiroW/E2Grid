#include "E2GridVisualizeComponent.h"

#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

UE2GridVisualizeComponent::UE2GridVisualizeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);

#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet() && !GetStaticMesh())
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/E2Grid/Meshes/Plane"));
		SetStaticMesh(DefaultMesh.Object);
	}
#endif
}

void UE2GridVisualizeComponent::BuildGridInstancedMeshes()
{
	const UStaticMesh* VisualMesh = GetStaticMesh();
	if (!GridOwner || !VisualMesh)
	{
		ClearInstances();
		return;
	}

	ClearInstances();
	const FE2GridMapLayout* Layout = GridOwner->GetLayout();
	if (!Layout)
	{
		return;
	}

	const FVector MeshSize = VisualMesh->GetBounds().BoxExtent * 2.0;
	const FVector LocalScale(
		MeshSize.X > UE_SMALL_NUMBER ? Layout->CellSize / MeshSize.X : 1.0,
		MeshSize.Y > UE_SMALL_NUMBER ? Layout->CellSize / MeshSize.Y : 1.0,
		1.0);

	GridOwner->ForEachCell([this, &LocalScale](int32, const FE2GridCellData&, const FVector& WorldPosition)
	{
		FTransform InstanceTransform(FQuat::Identity, GetComponentTransform().InverseTransformPosition(WorldPosition), LocalScale);
		AddInstance(InstanceTransform);
	});
}

void UE2GridVisualizeComponent::BeginPlay()
{
	Super::BeginPlay();
	SetVisibility(GridOwner && GridOwner->bShowVisualizedGrid);
	BuildGridInstancedMeshes();
}

void UE2GridVisualizeComponent::OnRegister()
{
	Super::OnRegister();
	GridOwner = Cast<AE2GridManager>(GetOwner());
}

void UE2GridVisualizeComponent::PostLoad()
{
	Super::PostLoad();
	GridOwner = Cast<AE2GridManager>(GetOwner());
}

void UE2GridVisualizeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
