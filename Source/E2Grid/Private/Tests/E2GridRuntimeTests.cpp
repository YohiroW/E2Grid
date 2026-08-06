#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace
{
	TMap<int32, FE2GridCellData> MakeOpenCells(const FE2GridMapLayout& Layout)
	{
		TMap<int32, FE2GridCellData> Cells;
		for (int32 Y = 0; Y < Layout.GridDimension.Y; ++Y)
		{
			for (int32 X = 0; X < Layout.GridDimension.X; ++X)
			{
				const FE2GridCoord Coord(X, Y);
				FE2GridCellData Cell;
				Cell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
				Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
				for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
				{
					const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
					if (Layout.IsValidCoord(FE2GridCoord(X + Offset.X, Y + Offset.Y)))
					{
						Cell.NeighborMask |= static_cast<uint8>(1u << DirectionIndex);
					}
				}
				Cells.Add(Layout.CoordToKey(Coord), Cell);
			}
		}
		return Cells;
	}

	UWorld* CreateTestWorld()
	{
		const FName WorldName = MakeUniqueObjectName(
			nullptr,
			UWorld::StaticClass(),
			NAME_None,
			EUniqueObjectNameOptions::GloballyUnique);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyTestWorld(UWorld* World)
	{
		if (World->AreActorsInitialized())
		{
			for (AActor* Actor : FActorRange(World))
			{
				if (Actor)
				{
					Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
				}
			}
		}
		GEngine->ShutdownWorldNetDriver(World);
		World->DestroyWorld(true);
		World->SetPhysicsScene(nullptr);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	}

	UE2GridUnitComponent* AddGridUnit(AActor& Owner)
	{
		return NewObject<UE2GridUnitComponent>(&Owner);
	}

	void AddSceneRoot(AActor& Owner)
	{
		USceneComponent* Root = NewObject<USceneComponent>(&Owner);
		Owner.SetRootComponent(Root);
		Root->RegisterComponent();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridCoordinateAndSparseMapTest,
	"E2Grid.Runtime.CoordinatesAndSparseMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FE2GridCoordinateAndSparseMapTest::RunTest(const FString& Parameters)
{
	FE2GridMapLayout Layout;
	Layout.CellSize = 50.0f;
	Layout.GridDimension = FIntPoint(4, 3);
	Layout.LocalOrigin = FVector(-75.0, -50.0, 10.0);

	for (int32 Y = 0; Y < Layout.GridDimension.Y; ++Y)
	{
		for (int32 X = 0; X < Layout.GridDimension.X; ++X)
		{
			const FE2GridCoord Coord(X, Y);
			FE2GridCoord RoundTrip;
			const int32 Key = Layout.CoordToKey(Coord);
			TestTrue(TEXT("Valid key converts back to a coordinate"), Layout.KeyToCoord(Key, RoundTrip));
			TestTrue(TEXT("Cell key and coordinate conversion round trip"), RoundTrip == Coord);
		}
	}

	UWorld* World = CreateTestWorld();
	UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>();
	TMap<int32, FE2GridCellData> SparseCells;
	FE2GridCellData Cell;
	Cell.LocalHeight = 12.0f;
	Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
	const int32 PresentKey = Layout.CoordToKey(FE2GridCoord(2, 1));
	SparseCells.Add(PresentKey, Cell);
	Asset->ReplaceData(Layout, MoveTemp(SparseCells));

	AE2GridManager* Manager = World->SpawnActor<AE2GridManager>();
	Manager->GridMapAsset = Asset;
	Manager->SetActorTransform(FTransform(FRotator(0.0, 30.0, 0.0), FVector(100.0, 200.0, 300.0)));
	FVector CellWorld;
	TestTrue(TEXT("Sparse cell resolves through manager"), Manager->CellToWorld(PresentKey, CellWorld));
	int32 RoundTripKey = INVALID_GRID_KEY;
	TestTrue(TEXT("World center converts through manager"), Manager->WorldToCell(CellWorld, RoundTripKey));
	TestEqual(TEXT("World/cell conversion round trip"), RoundTripKey, PresentKey);
	TestFalse(TEXT("Missing sparse key is not a valid cell"), Manager->IsValidGridKey(Layout.CoordToKey(FE2GridCoord(0, 0))));

	DestroyTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridOccupancyAndPathTest,
	"E2Grid.Runtime.OccupancyAndPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FE2GridOccupancyAndPathTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateTestWorld();
	FE2GridMapLayout Layout;
	Layout.CellSize = 50.0f;
	Layout.GridDimension = FIntPoint(3, 3);
	Layout.LocalOrigin = FVector::ZeroVector;
	UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>();
	Asset->ReplaceData(Layout, MakeOpenCells(Layout));

	AE2GridManager* Manager = World->SpawnActor<AE2GridManager>();
	Manager->GridMapAsset = Asset;
	UE2GridSubsystem* GridSubsystem = World->GetSubsystem<UE2GridSubsystem>();
	TestTrue(TEXT("A valid manager becomes active"), GridSubsystem->RegisterManager(Manager));

	AActor* UnitActorA = World->SpawnActor<AActor>();
	AActor* UnitActorB = World->SpawnActor<AActor>();
	AActor* ConflictActor = World->SpawnActor<AActor>();
	AddSceneRoot(*UnitActorA);
	AddSceneRoot(*UnitActorB);
	AddSceneRoot(*ConflictActor);
	UnitActorA->SetActorLocation(Manager->GetCellWorldCenterChecked(Layout.CoordToKey(FE2GridCoord(0, 0))));
	UnitActorB->SetActorLocation(Manager->GetCellWorldCenterChecked(Layout.CoordToKey(FE2GridCoord(1, 0))));
	ConflictActor->SetActorLocation(UnitActorB->GetActorLocation());
	UE2GridUnitComponent* UnitA = AddGridUnit(*UnitActorA);
	UE2GridUnitComponent* UnitB = AddGridUnit(*UnitActorB);
	UE2GridUnitComponent* ConflictUnit = AddGridUnit(*ConflictActor);

	TestEqual(
		TEXT("First unit registers"),
		GridSubsystem->RegisterUnit(UnitA),
		EE2GridRegistrationStatus::Registered);
	TestEqual(
		TEXT("Second unit registers"),
		GridSubsystem->RegisterUnit(UnitB),
		EE2GridRegistrationStatus::Registered);
	TestEqual(
		TEXT("A second owner cannot register to an occupied cell"),
		GridSubsystem->RegisterUnit(ConflictUnit),
		EE2GridRegistrationStatus::CellOccupied);

	FE2GridPathResult OccupiedGoalPath;
	TestFalse(
		TEXT("Occupied goal is rejected"),
		GridSubsystem->FindPath(UnitA, UnitB->GetCurrentCellKey(), OccupiedGoalPath));
	TestEqual(TEXT("Occupied goal has an explicit status"), OccupiedGoalPath.Status, EE2GridPathStatus::GoalOccupied);

	const int32 GoalKey = Layout.CoordToKey(FE2GridCoord(2, 0));
	FE2GridPathResult DetourPath;
	TestTrue(TEXT("A* routes around dynamic occupancy"), GridSubsystem->FindPath(UnitA, GoalKey, DetourPath));
	TestEqual(TEXT("A* returns success"), DetourPath.Status, EE2GridPathStatus::Success);
	TestTrue(TEXT("Detour contains movement cells"), DetourPath.CellKeys.Num() >= 2);

	const int32 SourceKey = UnitA->GetCurrentCellKey();
	const int32 CommitKey = Layout.CoordToKey(FE2GridCoord(0, 1));
	TestTrue(TEXT("Move commits atomically to an empty standable cell"), GridSubsystem->CommitUnitMove(UnitA, CommitKey));
	TestNull(TEXT("Source occupancy is cleared"), GridSubsystem->GetCellOwner(SourceKey));
	TestEqual(TEXT("Goal occupancy points to the unit"), GridSubsystem->GetCellOwner(CommitKey), UnitA);
	TestEqual(TEXT("Unit current cell matches occupancy"), UnitA->GetCurrentCellKey(), CommitKey);

	GridSubsystem->UnregisterUnit(UnitA);
	TestNull(TEXT("Unregister clears occupancy"), GridSubsystem->GetCellOwner(CommitKey));
	DestroyTestWorld(World);
	return true;
}

#endif
