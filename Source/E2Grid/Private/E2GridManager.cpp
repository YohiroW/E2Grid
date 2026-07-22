#include "E2GridManager.h"
#include "E2GridVisualizeComponent.h"
#include "DrawDebugHelpers.h"

AE2GridManager::AE2GridManager()
{
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = SceneComponent;
	
	{
		GridVisualizeComponent = CreateDefaultSubobject<UE2GridVisualizeComponent>(TEXT("GridVisualizeComponent"));
		GridVisualizeComponent->SetupAttachment(RootComponent);
		// GridVisualizeComponent->SetVisibility(bShowVisualizedGrid);
	}
	
	GridDataClass = UE2GridRuntimeData::StaticClass();
	GridDimension = FIntPoint(10, 10);
}

void AE2GridManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	Generate();
}

void AE2GridManager::Generate()
{
	Clear();
	
	// Fallback data class
    UClass* DataClass = GridDataClass ? GridDataClass.Get() : UE2GridRuntimeData::StaticClass();

	const int32 Width = GridDimension.X;
	const int32 Height = GridDimension.Y;
	const double HalfGridSize = static_cast<double>(GridSize) * 0.5;

	// Grid coordinates identify cell centers. Offset (0, 0) by half of the
	// center-to-center span so that the actor origin remains at the grid center.
	BaseOffset = FVector(
		-static_cast<double>(Width - 1) * HalfGridSize,
		-static_cast<double>(Height - 1) * HalfGridSize,
		0.0);
	
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			FE2GridCoord Coord(X , Y);
			int32 GridKey = X + Y * Width;
			
			UE2GridRuntimeData* GridData = NewObject<UE2GridRuntimeData>(this, DataClass);
			GridData->Coord = Coord;
			GridData->GridKey= GridKey;
			
			GridMap.Add(GridKey, GridData);
		}
	}
	
	bool bNotifyVisualChanged = true;
	if (bNotifyVisualChanged && GridVisualizeComponent)
	{
		GridVisualizeComponent->BuildGridInstancedMeshes();
	}
}

void AE2GridManager::Clear()
{
	GridMap.Empty();
}

bool AE2GridManager::IsValidGridKey(const int32 InGridKey) const
{
	return InGridKey != INVALID_GRID_KEY;
}

bool AE2GridManager::IsValidGridCoord(const FE2GridCoord& InCoord) const
{
	return InCoord.X >= 0 && InCoord.X < GridDimension.X && InCoord.Y >= 0 && InCoord.Y < GridDimension.Y;
}

bool AE2GridManager::IsGridMapEmpty()
{
	return GridMap.IsEmpty();
}

const UE2GridRuntimeData* AE2GridManager::GetGridData(const int32 InGridKey)
{
	return GridMap[InGridKey];
}

FE2GridCoord AE2GridManager::GetCoord(const int32 InGridKey)
{
	if (GridMap.Contains(InGridKey))
	{
		return GetGridData(InGridKey)->Coord;
	}
	
	return FE2GridCoord::INVALID_COORD;
}

FVector AE2GridManager::GetWorldPosition(const FVector& InOrigin, const FE2GridCoord& InCoord)
{
	FVector Position = InOrigin + FVector(InCoord.X * GridSize, InCoord.Y * GridSize, 0.0f);
	return Position;
}

FVector AE2GridManager::GetGridLocalPosition(const FE2GridCoord& InCoord) const
{
	return BaseOffset + FVector(InCoord.X * GridSize, InCoord.Y * GridSize, 0.0);
}

FVector AE2GridManager::GetGridWorldPosition(const FE2GridCoord& InCoord) const
{
	return GetActorTransform().TransformPosition(GetGridLocalPosition(InCoord));
}

bool AE2GridManager::GetGridCoord(const FVector& InWorldPos, FE2GridCoord& OutCoord)
{
	if (GridSize <= 0)
	{
		OutCoord = FE2GridCoord::INVALID_COORD;
		return false;
	}

	const FVector LocalPos = GetActorTransform().InverseTransformPosition(InWorldPos) - BaseOffset;
	const int32 CoordX = FMath::FloorToInt((LocalPos.X + GridSize * 0.5f) / GridSize);
	const int32 CoordY = FMath::FloorToInt((LocalPos.Y + GridSize * 0.5f) / GridSize);
	
	OutCoord.X = CoordX;
	OutCoord.Y = CoordY;
	OutCoord.Layer = 0;
	
	return IsValidGridCoord(OutCoord);
}

// Called when the game starts or when spawned
void AE2GridManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AE2GridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AE2GridManager::ForEachGridData(
	TFunctionRef<bool(TObjectPtr<UE2GridRuntimeData> InGridData, const FVector& InWorldPosition)> InFunc)
{
	for (const auto& Elem: GridMap)
	{
		TObjectPtr<UE2GridRuntimeData> GridData = Elem.Value;
		const FVector WorldPos = GetGridWorldPosition(GridData->Coord);
		InFunc(Elem.Value, WorldPos);
	}
}

// ----------------------------------------------------------------------

#if WITH_EDITOR
void AE2GridManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AE2GridManager, GridDimension) ||
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AE2GridManager, GridSize) || 
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AE2GridManager, GridDataClass) || 
			PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AE2GridManager, BaseOffset))
		{
			Generate();
		}
		
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AE2GridManager, bDrawGridMap))
		{
			if (IsGridMapEmpty())
			{
				Generate();
			}
			DrawGridMap(!bDrawGridMap);
		}
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AE2GridManager::PostLoad()
{
	Super::PostLoad();
	// Generate();
}

void AE2GridManager::DrawGridMap(bool bClearOnly /*= false*/)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	FlushPersistentDebugLines(World);
	
	if (!bClearOnly)
	{
		const FQuat GridRotation = GetActorQuat();
		const FVector GridScale = GetActorScale3D().GetAbs();
		const FVector BoxExtent(
			GridSize * 0.5 * GridScale.X,
			GridSize * 0.5 * GridScale.Y,
			GridScale.Z);

		for (const auto& Elem: GridMap)
		{
			const UE2GridRuntimeData* GridData = Elem.Value;
			const FE2GridCoord& Coord = GridData->Coord;
		
			const FVector WorldPosition = GetGridWorldPosition(Coord);
			DrawDebugCrosshairs(World, WorldPosition, GridRotation.Rotator(), 10.0f, FColor::Green, true);
			DrawDebugBox(World, WorldPosition, BoxExtent, GridRotation, FColor::Green, true);
		}
	}
}


#endif 
