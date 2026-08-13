#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "E2GridMovementComponent.h"
#include "E2GridSubsystem.h"
#include "E2GridUnitComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	TObjectPtr<UE2GridMapAsset> CreateMapAsset(
		const FIntPoint& Dimension,
		const TSet<int32>& OmittedKeys = {})
	{
		UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>(GetTransientPackage());
		FE2GridMapLayout Layout;
		Layout.CellSize = 50.0f;
		Layout.GridDimension = Dimension;
		Layout.LocalOrigin = FVector::ZeroVector;

		TMap<int32, FE2GridCellData> Cells;
		for (int32 Y = 0; Y < Dimension.Y; ++Y)
		{
			for (int32 X = 0; X < Dimension.X; ++X)
			{
				const int32 CellKey = Layout.CoordToKey(FE2GridCoord(X, Y));
				if (OmittedKeys.Contains(CellKey))
				{
					continue;
				}
				FE2GridCellData Cell;
				Cell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
				Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
				Cells.Add(CellKey, Cell);
			}
		}

		for (TPair<int32, FE2GridCellData>& Pair : Cells)
		{
			FE2GridCoord Coord;
			Layout.KeyToCoord(Pair.Key, Coord);
			for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
			{
				const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
				const int32 NeighborKey = Layout.CoordToKey(
					FE2GridCoord(Coord.X + Offset.X, Coord.Y + Offset.Y));
				if (!Cells.Contains(NeighborKey))
				{
					continue;
				}
				if (E2GridDirections::IsDiagonal(DirectionIndex))
				{
					const int32 OrthogonalA =
						(DirectionIndex + E2GridDirections::Count - 1) % E2GridDirections::Count;
					const int32 OrthogonalB = (DirectionIndex + 1) % E2GridDirections::Count;
					const FIntPoint& OffsetA = E2GridDirections::GetOffset(OrthogonalA);
					const FIntPoint& OffsetB = E2GridDirections::GetOffset(OrthogonalB);
					if (!Cells.Contains(Layout.CoordToKey(
							FE2GridCoord(Coord.X + OffsetA.X, Coord.Y + OffsetA.Y))) ||
						!Cells.Contains(Layout.CoordToKey(
							FE2GridCoord(Coord.X + OffsetB.X, Coord.Y + OffsetB.Y))))
					{
						continue;
					}
				}
				Pair.Value.NeighborMask |= static_cast<uint8>(1u << DirectionIndex);
			}
		}

		Asset->ReplaceData(Layout, MoveTemp(Cells));
		return Asset;
	}

	class FRuntimeWorldFixture
	{
	public:
		FRuntimeWorldFixture()
		{
			FWorldInitializationValues InitializationValues;
			InitializationValues
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false)
				.CreateFXSystem(false);
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				MakeUniqueObjectName(
					GetTransientPackage(),
					UWorld::StaticClass(),
					TEXT("E2GridTestWorld")),
				GetTransientPackage(),
				true,
				ERHIFeatureLevel::Num,
				&InitializationValues);
			check(World);
			WorldContext.SetCurrentWorld(World);
		}

		~FRuntimeWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(true);
				GEngine->DestroyWorldContext(World);
				World->RemoveFromRoot();
			}
		}

		AE2GridManager* AddManager(UE2GridMapAsset* Asset, const FVector& Location = FVector::ZeroVector)
		{
			AE2GridManager* Manager = World->SpawnActor<AE2GridManager>(Location, FRotator::ZeroRotator);
			Manager->GridMapAsset = Asset;
			return Manager;
		}

		UE2GridUnitComponent* AddUnit(
			const FVector& Location,
			FName ActorName = NAME_None)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Name = ActorName;
			AActor* Actor = World->SpawnActor<AActor>(
				AActor::StaticClass(),
				Location,
				FRotator::ZeroRotator,
				SpawnParameters);
			USceneComponent* RootComponent = NewObject<USceneComponent>(Actor, TEXT("Root"));
			Actor->AddInstanceComponent(RootComponent);
			RootComponent->RegisterComponent();
			Actor->SetRootComponent(RootComponent);
			Actor->SetActorLocation(Location);
			UE2GridUnitComponent* Unit = NewObject<UE2GridUnitComponent>(Actor, TEXT("GridUnit"));
			Actor->AddInstanceComponent(Unit);
			Unit->RegisterComponent();
			return Unit;
		}

		UWorld* World = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridCoordinateTest,
	"E2Grid.Runtime.Coordinates.KeyCoordAndBounds",
	TestFlags)

bool FE2GridCoordinateTest::RunTest(const FString& Parameters)
{
	FE2GridMapLayout Layout;
	Layout.CellSize = 50.0f;
	Layout.GridDimension = FIntPoint(4, 3);
	Layout.LocalOrigin = FVector(-75.0f, -50.0f, 10.0f);

	TestEqual(TEXT("Coord converts to row-major key"), Layout.CoordToKey(FE2GridCoord(2, 1)), 6);
	FE2GridCoord Coord;
	TestTrue(TEXT("Key converts to coord"), Layout.KeyToCoord(6, Coord));
	TestEqual(TEXT("Round-trip X"), Coord.X, 2);
	TestEqual(TEXT("Round-trip Y"), Coord.Y, 1);
	TestEqual(TEXT("v0.1 layer remains zero"), Coord.Layer, 0);
	TestEqual(TEXT("Out-of-bounds coord is invalid"), Layout.CoordToKey(FE2GridCoord(4, 1)), INVALID_GRID_KEY);
	TestFalse(TEXT("Out-of-bounds key is rejected"), Layout.KeyToCoord(12, Coord));
	TestTrue(
		TEXT("Local center uses origin, spacing and local height"),
		Layout.GetCellLocalCenter(FE2GridCoord(2, 1), 7.0f).Equals(FVector(25.0f, 0.0f, 17.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridAssetValidationTest,
	"E2Grid.Runtime.Asset.ValidationAndSparseTopology",
	TestFlags)

bool FE2GridAssetValidationTest::RunTest(const FString& Parameters)
{
	UE2GridMapAsset* UnbuiltAsset = NewObject<UE2GridMapAsset>(GetTransientPackage());
	FString ValidationError;
	TestFalse(TEXT("Unbuilt asset is rejected by build version"), UnbuiltAsset->Validate(&ValidationError));
	TestTrue(TEXT("Version failure is explicit"), ValidationError.Contains(TEXT("build version")));

	UE2GridMapAsset* SparseAsset = CreateMapAsset(FIntPoint(2, 2), {3});
	TestTrue(TEXT("Valid sparse asset passes"), SparseAsset->Validate(&ValidationError));
	TestNull(TEXT("Omitted cell is absent"), SparseAsset->FindCell(3));
	TestEqual(
		TEXT("Missing corner prevents diagonal traversal"),
		SparseAsset->FindCell(0)->NeighborMask & (1u << 1),
		0);

	FE2GridMapLayout Layout;
	Layout.GridDimension = FIntPoint(2, 2);
	TMap<int32, FE2GridCellData> InvalidCells;
	FE2GridCellData InvalidCell;
	InvalidCell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
	InvalidCell.SetFlag(EE2GridCellFlags::CanStandOn, true);
	InvalidCell.NeighborMask = 1u << 1;
	InvalidCells.Add(0, InvalidCell);
	InvalidCells.Add(3, InvalidCell);
	UE2GridMapAsset* InvalidAsset = NewObject<UE2GridMapAsset>(GetTransientPackage());
	InvalidAsset->ReplaceData(Layout, MoveTemp(InvalidCells));
	TestFalse(TEXT("Diagonal corner cutting is rejected"), InvalidAsset->Validate(&ValidationError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridManagerQueryTest,
	"E2Grid.Runtime.Manager.TransformSparseAndNeighbors",
	TestFlags)

bool FE2GridManagerQueryTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridMapAsset* Asset = CreateMapAsset(FIntPoint(2, 2), {3});
	AE2GridManager* Manager = Fixture.AddManager(Asset, FVector(100.0f, 200.0f, 300.0f));

	FVector CellWorld;
	TestTrue(TEXT("CellToWorld succeeds"), Manager->CellToWorld(1, CellWorld));
	TestTrue(TEXT("Manager transform is applied"), CellWorld.Equals(FVector(150.0f, 200.0f, 300.0f)));
	int32 CellKey = INVALID_GRID_KEY;
	TestTrue(TEXT("WorldToCell round trips"), Manager->WorldToCell(CellWorld, CellKey));
	TestEqual(TEXT("WorldToCell returns original cell"), CellKey, 1);
	TestFalse(
		TEXT("WorldToCell rejects a position far above the ground surface"),
		Manager->WorldToCell(CellWorld + FVector(0.0f, 0.0f, 100.0f), CellKey));
	TestFalse(TEXT("Sparse hole cannot be resolved"), Manager->CellToWorld(3, CellWorld));

	TArray<int32> Neighbors;
	Manager->ForEachTraversableNeighbor(
		0,
		[&Neighbors](int32 NeighborKey, float)
		{
			Neighbors.Add(NeighborKey);
		});
	Neighbors.Sort();
	TestEqual(TEXT("Only existing orthogonal neighbors are returned"), Neighbors, TArray<int32>({1, 2}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridRegistrationTest,
	"E2Grid.Runtime.Subsystem.RegistrationOccupancyAndManagerUniqueness",
	TestFlags)

bool FE2GridRegistrationTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridMapAsset* Asset = CreateMapAsset(FIntPoint(2, 1));
	AE2GridManager* Manager = Fixture.AddManager(Asset);
	AE2GridManager* SecondManager = Fixture.AddManager(Asset);
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	TestNotNull(TEXT("World subsystem is created"), Subsystem);
	TestTrue(TEXT("First valid manager registers"), Subsystem->RegisterManager(Manager));
	AddExpectedError(TEXT("Only one active E2Grid manager"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Second manager is rejected"), Subsystem->RegisterManager(SecondManager));
	TestEqual(TEXT("First manager remains active"), Subsystem->GetActiveManager(), Manager);

	UE2GridUnitComponent* FirstUnit = Fixture.AddUnit(FVector::ZeroVector, TEXT("FirstUnit"));
	UE2GridUnitComponent* ConflictUnit = Fixture.AddUnit(FVector::ZeroVector, TEXT("ConflictUnit"));
	TestEqual(
		TEXT("First unit registers"),
		Subsystem->RegisterUnit(FirstUnit),
		EE2GridRegistrationStatus::Registered);
	TestEqual(
		TEXT("Conflicting unit is rejected"),
		Subsystem->RegisterUnit(ConflictUnit),
		EE2GridRegistrationStatus::CellOccupied);
	TestEqual(TEXT("Cell owner is first unit"), Subsystem->GetCellOwner(0), FirstUnit);
	Subsystem->UnregisterUnit(FirstUnit);
	TestNull(TEXT("Unregister clears occupancy"), Subsystem->GetCellOwner(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridPendingRegistrationTest,
	"E2Grid.Runtime.Subsystem.PendingRegistrationOrder",
	TestFlags)

bool FE2GridPendingRegistrationTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	UE2GridUnitComponent* UnitB = Fixture.AddUnit(FVector::ZeroVector, TEXT("B_Unit"));
	UE2GridUnitComponent* UnitA = Fixture.AddUnit(FVector::ZeroVector, TEXT("A_Unit"));
	TestEqual(
		TEXT("Unit B waits for manager"),
		Subsystem->RegisterUnit(UnitB),
		EE2GridRegistrationStatus::PendingManager);
	TestEqual(
		TEXT("Unit A waits for manager"),
		Subsystem->RegisterUnit(UnitA),
		EE2GridRegistrationStatus::PendingManager);

	AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(1, 1)));
	TestTrue(TEXT("Manager registration retries pending units"), Subsystem->RegisterManager(Manager));
	TestTrue(TEXT("Path-name-sorted unit A wins the shared cell"), UnitA->IsRegistered());
	TestFalse(TEXT("Unit B reports the conflict"), UnitB->IsRegistered());
	TestEqual(TEXT("Occupancy belongs to unit A"), Subsystem->GetCellOwner(0), UnitA);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridPathAndCommitTest,
	"E2Grid.Runtime.Pathfinding.ShortestFailuresOccupancyAndCommit",
	TestFlags)

bool FE2GridPathAndCommitTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridMapAsset* Asset = CreateMapAsset(FIntPoint(3, 3));
	AE2GridManager* Manager = Fixture.AddManager(Asset);
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	Subsystem->RegisterManager(Manager);
	UE2GridUnitComponent* MovingUnit = Fixture.AddUnit(FVector::ZeroVector, TEXT("MovingUnit"));
	TestEqual(
		TEXT("Moving unit registers at cell zero"),
		Subsystem->RegisterUnit(MovingUnit),
		EE2GridRegistrationStatus::Registered);

	FE2GridPathResult Path;
	TestTrue(TEXT("A* finds a path"), Subsystem->FindPath(MovingUnit, 8, Path));
	TestEqual(TEXT("Diagonal shortest path contains two steps"), Path.Steps.Num(), 2);
	TestTrue(TEXT("Octile path cost is correct"), FMath::IsNearlyEqual(Path.TotalCost, 2.0f * UE_SQRT_2));
	TestEqual(TEXT("Path reaches requested goal"), Path.Steps.Last().ToCellKey, 8);

	FE2GridPathResult InvalidPath;
	TestFalse(TEXT("Invalid goal is rejected"), Subsystem->FindPath(MovingUnit, 99, InvalidPath));
	TestEqual(TEXT("Invalid goal status is explicit"), InvalidPath.Status, EE2GridPathStatus::InvalidGoal);

	UE2GridUnitComponent* BlockingUnit = Fixture.AddUnit(FVector(50.0f, 50.0f, 0.0f), TEXT("BlockingUnit"));
	Subsystem->RegisterUnit(BlockingUnit);
	FE2GridPathResult OccupiedPath;
	TestFalse(TEXT("Occupied goal is rejected"), Subsystem->FindPath(MovingUnit, 4, OccupiedPath));
	TestEqual(TEXT("Occupied goal status is explicit"), OccupiedPath.Status, EE2GridPathStatus::GoalOccupied);

	TArray<FE2GridPathStep> InvalidCommit;
	FE2GridPathStep InvalidStep;
	InvalidStep.ToCellKey = 8;
	InvalidCommit.Add(InvalidStep);
	TestFalse(TEXT("Commit rejects non-neighbor jump"), Subsystem->CommitUnitMove(MovingUnit, InvalidCommit));
	TestEqual(TEXT("Rejected commit preserves source"), MovingUnit->GetCurrentCellKey(), 0);
	TestEqual(TEXT("Rejected commit preserves occupancy"), Subsystem->GetCellOwner(0), MovingUnit);

	Subsystem->UnregisterUnit(BlockingUnit);
	TestTrue(TEXT("Validated path commits atomically"), Subsystem->CommitUnitMove(MovingUnit, Path.Steps));
	TestNull(TEXT("Atomic commit clears source"), Subsystem->GetCellOwner(0));
	TestEqual(TEXT("Atomic commit sets destination"), Subsystem->GetCellOwner(8), MovingUnit);
	TestEqual(TEXT("Atomic commit updates unit state"), MovingUnit->GetCurrentCellKey(), 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridNoPathTest,
	"E2Grid.Runtime.Pathfinding.NoPath",
	TestFlags)

bool FE2GridNoPathTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridMapAsset* Asset = CreateMapAsset(FIntPoint(3, 1), {1});
	AE2GridManager* Manager = Fixture.AddManager(Asset);
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	Subsystem->RegisterManager(Manager);
	UE2GridUnitComponent* Unit = Fixture.AddUnit(FVector::ZeroVector);
	Subsystem->RegisterUnit(Unit);

	FE2GridPathResult Path;
	TestFalse(TEXT("Disconnected goal has no path"), Subsystem->FindPath(Unit, 2, Path));
	TestEqual(TEXT("No path status is explicit"), Path.Status, EE2GridPathStatus::NoPath);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridMovementCancelTest,
	"E2Grid.Runtime.Movement.CancelRestoresOccupiedCell",
	TestFlags)

bool FE2GridMovementCancelTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridMapAsset* Asset = CreateMapAsset(FIntPoint(2, 1));
	AE2GridManager* Manager = Fixture.AddManager(Asset);
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	Subsystem->RegisterManager(Manager);
	UE2GridUnitComponent* Unit = Fixture.AddUnit(FVector::ZeroVector, TEXT("MovingUnit"));
	Subsystem->RegisterUnit(Unit);
	UE2GridMovementComponent* Movement = NewObject<UE2GridMovementComponent>(Unit->GetOwner(), TEXT("Movement"));
	Unit->GetOwner()->AddInstanceComponent(Movement);
	Movement->RegisterComponent();
	Movement->bSweepDuringMovement = false;

	TestTrue(TEXT("Movement starts for a valid path"), Movement->MoveToCell(1));
	TestTrue(TEXT("Movement reports active state"), Movement->IsMoving());
	Unit->GetOwner()->SetActorLocation(FVector(25.0f, 0.0f, 0.0f));
	Movement->CancelMove();
	TestFalse(TEXT("Cancel disables movement"), Movement->IsMoving());
	TestTrue(
		TEXT("Cancel restores actor to occupied source center"),
		Unit->GetOwner()->GetActorLocation().Equals(FVector::ZeroVector));
	TestEqual(TEXT("Cancel preserves current cell"), Unit->GetCurrentCellKey(), 0);
	TestEqual(TEXT("Cancel preserves source occupancy"), Subsystem->GetCellOwner(0), Unit);
	TestNull(TEXT("Cancel does not occupy the goal"), Subsystem->GetCellOwner(1));
	return true;
}

#endif
