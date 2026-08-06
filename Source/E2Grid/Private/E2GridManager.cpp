#include "E2GridManager.h"

#include "E2GridMapAsset.h"
#include "E2GridSubsystem.h"
#include "E2GridVisualizeComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AE2GridManager::AE2GridManager()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	GridVisualizeComponent = CreateDefaultSubobject<UE2GridVisualizeComponent>(TEXT("GridVisualization"));
	GridVisualizeComponent->SetupAttachment(SceneRoot);
}

void AE2GridManager::BeginPlay()
{
	Super::BeginPlay();

	if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		GridSubsystem->RegisterManager(this);
	}
}

void AE2GridManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UE2GridSubsystem* GridSubsystem = World->GetSubsystem<UE2GridSubsystem>())
		{
			GridSubsystem->UnregisterManager(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

const FE2GridMapLayout* AE2GridManager::GetLayout() const
{
	return GridMapAsset && GridMapAsset->IsValidMap() ? &GridMapAsset->GetLayout() : nullptr;
}

const TMap<int32, FE2GridCellData>* AE2GridManager::GetCells() const
{
	return GridMapAsset && GridMapAsset->IsValidMap() ? &GridMapAsset->GetCells() : nullptr;
}

bool AE2GridManager::HasValidGrid() const
{
	return GetLayout() != nullptr;
}

bool AE2GridManager::IsValidGridKey(int32 CellKey) const
{
	return FindCell(CellKey) != nullptr;
}

bool AE2GridManager::IsValidGridCoord(const FE2GridCoord& Coord) const
{
	const FE2GridMapLayout* Layout = GetLayout();
	return Layout && Layout->IsValidCoord(Coord) && FindCell(Layout->CoordToKey(Coord));
}

int32 AE2GridManager::GetGridKey(const FE2GridCoord& Coord) const
{
	const FE2GridMapLayout* Layout = GetLayout();
	return Layout ? Layout->CoordToKey(Coord) : INVALID_GRID_KEY;
}

bool AE2GridManager::GetCoordByKey(int32 CellKey, FE2GridCoord& OutCoord) const
{
	const FE2GridMapLayout* Layout = GetLayout();
	if (!Layout || !FindCell(CellKey))
	{
		OutCoord = FE2GridCoord::INVALID_COORD;
		return false;
	}
	return Layout->KeyToCoord(CellKey, OutCoord);
}

bool AE2GridManager::TryGetCellData(int32 CellKey, FE2GridCellData& OutCellData) const
{
	if (const FE2GridCellData* Cell = FindCell(CellKey))
	{
		OutCellData = *Cell;
		return true;
	}
	OutCellData = FE2GridCellData();
	return false;
}

const FE2GridCellData* AE2GridManager::FindCell(int32 CellKey) const
{
	return GridMapAsset ? GridMapAsset->FindCell(CellKey) : nullptr;
}

bool AE2GridManager::WorldToCell(const FVector& WorldPosition, int32& OutCellKey) const
{
	const FE2GridMapLayout* Layout = GetLayout();
	if (!Layout)
	{
		OutCellKey = INVALID_GRID_KEY;
		return false;
	}

	const FVector LocalPosition = GetActorTransform().InverseTransformPosition(WorldPosition) - Layout->LocalOrigin;
	const FE2GridCoord Coord(
		FMath::FloorToInt((LocalPosition.X + Layout->CellSize * 0.5f) / Layout->CellSize),
		FMath::FloorToInt((LocalPosition.Y + Layout->CellSize * 0.5f) / Layout->CellSize));
	const int32 CandidateKey = Layout->CoordToKey(Coord);
	if (!FindCell(CandidateKey))
	{
		OutCellKey = INVALID_GRID_KEY;
		return false;
	}

	OutCellKey = CandidateKey;
	return true;
}

bool AE2GridManager::CellToWorld(int32 CellKey, FVector& OutWorldPosition) const
{
	const FE2GridMapLayout* Layout = GetLayout();
	const FE2GridCellData* Cell = FindCell(CellKey);
	FE2GridCoord Coord;
	if (!Layout || !Cell || !Layout->KeyToCoord(CellKey, Coord))
	{
		OutWorldPosition = FVector::ZeroVector;
		return false;
	}

	OutWorldPosition = GetActorTransform().TransformPosition(
		Layout->GetCellLocalCenter(Coord, Cell->LocalHeight));
	return true;
}

FVector AE2GridManager::GetCellWorldCenterChecked(int32 CellKey) const
{
	FVector Result;
	check(CellToWorld(CellKey, Result));
	return Result;
}

void AE2GridManager::ForEachCell(
	TFunctionRef<void(int32, const FE2GridCellData&, const FVector&)> Visitor) const
{
	const TMap<int32, FE2GridCellData>* Cells = GetCells();
	if (!Cells)
	{
		return;
	}

	for (const TPair<int32, FE2GridCellData>& Pair : *Cells)
	{
		FVector WorldCenter;
		if (CellToWorld(Pair.Key, WorldCenter))
		{
			Visitor(Pair.Key, Pair.Value, WorldCenter);
		}
	}
}

#if WITH_EDITOR
void AE2GridManager::RefreshVisualization()
{
	if (GridVisualizeComponent)
	{
		GridVisualizeComponent->SetVisibility(bShowVisualizedGrid);
		GridVisualizeComponent->BuildGridInstancedMeshes();
	}
}
#endif
