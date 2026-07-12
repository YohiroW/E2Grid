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
	const int32 HalfGridSize = GridSize * 0.5f;
	
	BaseOffset = FVector(Width* HalfGridSize - HalfGridSize, Height* HalfGridSize- HalfGridSize, 0.0f);
	BaseOffset *= -1.0f; 
	
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
	
	return FE2GridCoord(INVALID_GRID_KEY, INVALID_GRID_KEY);
}

FVector AE2GridManager::GetWorldPosition(const FVector& InOrigin, const FE2GridCoord& InCoord)
{
	FVector Position = InOrigin + FVector(InCoord.X * GridSize, InCoord.Y * GridSize, InOrigin.Z);
	return Position;
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
	const FVector& Origin = GetActorLocation();
	for (const auto& Elem: GridMap)
	{
		TObjectPtr<UE2GridRuntimeData> GridData = Elem.Value;
		FVector WorldPos = GetWorldPosition(Origin, GridData->Coord);
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
		FVector Origin = GetActorLocation();
		Origin += BaseOffset;
		for (const auto& Elem: GridMap)
		{
			const UE2GridRuntimeData* GridData = Elem.Value;
			const FE2GridCoord& Coord = GridData->Coord;
		
			FVector WorldPosition = GetWorldPosition(Origin, Coord);
			DrawDebugCrosshairs(World, WorldPosition, FRotator::ZeroRotator, 10.0f, FColor::Green, true);
			DrawDebugBox(World, WorldPosition, FVector(GridSize * 0.5f, GridSize * 0.5f, 1.0f), FColor::Green, true);
		}
	}
}


#endif 
