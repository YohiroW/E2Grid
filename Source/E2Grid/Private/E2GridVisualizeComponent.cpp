#include "E2GridVisualizeComponent.h"
#include "E2GridManager.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

UE2GridVisualizeComponent::UE2GridVisualizeComponent()
{
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
		return;
	}
	
	ClearInstances();
	
	const FRotator Rot = GridOwner->GetActorRotation();
	const FVector MeshSize = VisualMesh->GetBounds().BoxExtent * 2.0;
	const FVector GridScale(
		MeshSize.X > UE_SMALL_NUMBER ? GridOwner->GridSize / MeshSize.X : 1.0,
		MeshSize.Y > UE_SMALL_NUMBER ? GridOwner->GridSize / MeshSize.Y : 1.0,
		1.0);
	const FVector Scale = GridOwner->GetActorScale3D() * GridScale;
	GridOwner->ForEachGridData([&](const FE2GridRuntimeData&, const FVector& InWorldPos) -> bool
	{
		FTransform InstanceTransform;
		InstanceTransform.SetLocation(InWorldPos);
		InstanceTransform.SetRotation(Rot.Quaternion());
		InstanceTransform.SetScale3D(Scale);
		
		AddInstance(InstanceTransform, true);
		
		return true;
	});
}

void UE2GridVisualizeComponent::BeginPlay()
{
	Super::BeginPlay();

	
}

void UE2GridVisualizeComponent::OnRegister()
{
	Super::OnRegister();
	
	AE2GridManager* GridManager = Cast<AE2GridManager>(GetOwner());
	if (!ensureMsgf(GridManager, TEXT("%s must owned by a AGridManager"), *GetName()))
	{
		return;
	}
	
	GridOwner = GridManager; 
}

void UE2GridVisualizeComponent::PostLoad()
{
	Super::PostLoad();
		
	AE2GridManager* GridManager = Cast<AE2GridManager>(GetOwner());
	if (!ensureMsgf(GridManager, TEXT("%s must owned by a AGridManager"), *GetName()))
	{
		return;
	}
	
	GridOwner = GridManager; 
}

void UE2GridVisualizeComponent::ShowGrids()
{
}

void UE2GridVisualizeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

