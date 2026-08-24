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
		const TSet<int32>& OmittedKeys = {},
		const TSet<int32>& NonStandableKeys = {})
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
				Cell.SetFlag(EE2GridCellFlags::CanStandOn, !NonStandableKeys.Contains(CellKey));
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
	TestEqual(TEXT("Unified path status reports success"), Path.QueryStatus, EE2GridQueryStatus::Success);
	TestEqual(TEXT("Path records its start"), Path.StartCellKey, 0);
	TestEqual(TEXT("Path records its goal"), Path.GoalCellKey, 8);
	TestEqual(TEXT("Path records its runtime snapshot"), Path.RuntimeRevision, Subsystem->GetRuntimeRevision());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridPlacementRevisionTest,
	"E2Grid.Runtime.Query.PlacementReasonsAndRevision",
	TestFlags)

bool FE2GridPlacementRevisionTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	UE2GridUnitComponent* Unit = Fixture.AddUnit(FVector::ZeroVector, TEXT("PlacementUnit"));
	TestEqual(TEXT("Initial revision is zero"), Subsystem->GetRuntimeRevision(), int64(0));
	TestEqual(
		TEXT("Placement reports a missing active grid"),
		Subsystem->QueryPlacement(Unit, 0).Status,
		EE2GridQueryStatus::NoActiveGrid);
	TestEqual(TEXT("Pure query does not advance revision"), Subsystem->GetRuntimeRevision(), int64(0));

	AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(4, 1), {}, {1}));
	TestTrue(TEXT("Manager registers"), Subsystem->RegisterManager(Manager));
	TestEqual(TEXT("Manager change advances revision"), Subsystem->GetRuntimeRevision(), int64(1));
	TestEqual(
		TEXT("Null unit is explicit"),
		Subsystem->QueryPlacement(nullptr, 0).Status,
		EE2GridQueryStatus::InvalidUnit);
	TestEqual(
		TEXT("Missing cell is explicit"),
		Subsystem->QueryPlacement(Unit, 99).Status,
		EE2GridQueryStatus::InvalidCell);
	TestEqual(
		TEXT("Non-standing cell is explicit"),
		Subsystem->QueryPlacement(Unit, 1).Status,
		EE2GridQueryStatus::NotStandable);

	TestEqual(
		TEXT("Unit registration succeeds"),
		Subsystem->RegisterUnit(Unit),
		EE2GridRegistrationStatus::Registered);
	TestEqual(TEXT("Unit registration advances revision"), Subsystem->GetRuntimeRevision(), int64(2));
	TestEqual(
		TEXT("A unit may query its own occupied cell"),
		Subsystem->QueryPlacement(Unit, 0).Status,
		EE2GridQueryStatus::Success);

	UE2GridUnitComponent* Blocker = Fixture.AddUnit(FVector(100.0f, 0.0f, 0.0f), TEXT("PlacementBlocker"));
	Subsystem->RegisterUnit(Blocker);
	TestEqual(TEXT("Second registration advances revision"), Subsystem->GetRuntimeRevision(), int64(3));
	TestEqual(
		TEXT("Other unit occupancy is explicit"),
		Subsystem->QueryPlacement(Unit, 2).Status,
		EE2GridQueryStatus::Occupied);
	TestEqual(
		TEXT("Free standing cell succeeds"),
		Subsystem->QueryPlacement(Unit, 3).Status,
		EE2GridQueryStatus::Success);
	TestEqual(TEXT("Placement queries remain side-effect free"), Subsystem->GetRuntimeRevision(), int64(3));

	Subsystem->UnregisterUnit(Blocker);
	TestEqual(TEXT("Unit unregister advances revision"), Subsystem->GetRuntimeRevision(), int64(4));
	Subsystem->UnregisterUnit(Blocker);
	TestEqual(TEXT("Repeated unregister is a no-op"), Subsystem->GetRuntimeRevision(), int64(4));
	Subsystem->UnregisterManager(Manager);
	TestEqual(TEXT("Manager removal advances revision"), Subsystem->GetRuntimeRevision(), int64(5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridStalePathCommitTest,
	"E2Grid.Runtime.Pathfinding.StaleRevisionCommit",
	TestFlags)

bool FE2GridStalePathCommitTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(3, 2)));
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	Subsystem->RegisterManager(Manager);
	UE2GridUnitComponent* MovingUnit = Fixture.AddUnit(FVector::ZeroVector, TEXT("VersionedMovingUnit"));
	Subsystem->RegisterUnit(MovingUnit);

	FE2GridPathResult OldPath;
	TestTrue(TEXT("Versioned path is found"), Subsystem->FindPath(MovingUnit, 2, OldPath));
	const int64 PathRevision = OldPath.RuntimeRevision;
	UE2GridUnitComponent* UnrelatedUnit = Fixture.AddUnit(FVector(100.0f, 50.0f, 0.0f), TEXT("UnrelatedUnit"));
	Subsystem->RegisterUnit(UnrelatedUnit);
	TestTrue(TEXT("Runtime changed after preview"), Subsystem->GetRuntimeRevision() > PathRevision);

	FE2GridMoveCommitResult StaleCommit;
	TestFalse(
		TEXT("Old path cannot commit"),
		Subsystem->CommitUnitMoveFromPath(MovingUnit, OldPath, StaleCommit));
	TestEqual(TEXT("Stale failure is structured"), StaleCommit.Status, EE2GridQueryStatus::StaleRevision);
	TestEqual(TEXT("Stale commit preserves unit cell"), MovingUnit->GetCurrentCellKey(), 0);
	TestEqual(TEXT("Stale commit preserves source occupancy"), Subsystem->GetCellOwner(0), MovingUnit);

	FE2GridPathResult FreshPath;
	TestTrue(TEXT("Fresh path is found"), Subsystem->FindPath(MovingUnit, 2, FreshPath));
	FE2GridMoveCommitResult FreshCommit;
	TestTrue(
		TEXT("Fresh path commits"),
		Subsystem->CommitUnitMoveFromPath(MovingUnit, FreshPath, FreshCommit));
	TestEqual(TEXT("Commit result reports success"), FreshCommit.Status, EE2GridQueryStatus::Success);
	TestEqual(TEXT("Commit updates destination"), MovingUnit->GetCurrentCellKey(), 2);
	TestEqual(TEXT("Commit result reports new revision"), FreshCommit.RuntimeRevision, Subsystem->GetRuntimeRevision());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridReachableTest,
	"E2Grid.Runtime.Query.ReachableBudgetOccupancyAndPath",
	TestFlags)

bool FE2GridReachableTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(3, 3)));
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	Subsystem->RegisterManager(Manager);
	UE2GridUnitComponent* Unit = Fixture.AddUnit(FVector(50.0f, 50.0f, 0.0f), TEXT("ReachableUnit"));
	Subsystem->RegisterUnit(Unit);

	FE2GridReachableResult OrthogonalRange;
	TestTrue(TEXT("One-point Reachable query succeeds"), Subsystem->FindReachableCells(Unit, 1.0f, OrthogonalRange));
	TestEqual(TEXT("One point reaches start plus four orthogonal cells"), OrthogonalRange.Cells.Num(), 5);
	TestEqual(TEXT("Reachable set includes start first"), OrthogonalRange.Cells[0].CellKey, 4);
	TestTrue(TEXT("Start cost is zero"), FMath::IsNearlyZero(OrthogonalRange.Cells[0].Cost));

	UE2GridUnitComponent* Blocker = Fixture.AddUnit(FVector(50.0f, 0.0f, 0.0f), TEXT("ReachableBlocker"));
	Subsystem->RegisterUnit(Blocker);
	FE2GridReachableResult BlockedRange;
	TestTrue(TEXT("Occupied Reachable query succeeds"), Subsystem->FindReachableCells(Unit, 1.0f, BlockedRange));
	TestEqual(TEXT("Occupied neighbor is excluded"), BlockedRange.Cells.Num(), 4);
	TestFalse(
		TEXT("Blocked cell is absent"),
		BlockedRange.Cells.ContainsByPredicate([](const FE2GridReachableCell& Cell)
		{
			return Cell.CellKey == 1;
		}));
	Subsystem->UnregisterUnit(Blocker);

	FE2GridReachableResult DiagonalRange;
	TestTrue(
		TEXT("Diagonal-budget Reachable query succeeds"),
		Subsystem->FindReachableCells(Unit, UE_SQRT_2, DiagonalRange));
	TestEqual(TEXT("Diagonal budget reaches the full 3x3 grid"), DiagonalRange.Cells.Num(), 9);
	const TArray<int32> ExpectedOrder({4, 1, 3, 5, 7, 0, 2, 6, 8});
	TArray<int32> ActualOrder;
	for (const FE2GridReachableCell& Cell : DiagonalRange.Cells)
	{
		ActualOrder.Add(Cell.CellKey);
	}
	TestEqual(TEXT("Reachable cells have deterministic cost/key order"), ActualOrder, ExpectedOrder);

	FE2GridPathResult RebuiltPath;
	TestTrue(
		TEXT("Reachable parent tree rebuilds a path"),
		Subsystem->BuildPathFromReachableResult(DiagonalRange, 0, RebuiltPath));
	TestEqual(TEXT("Rebuilt path has one diagonal step"), RebuiltPath.Steps.Num(), 1);
	FE2GridPathResult AStarPath;
	TestTrue(TEXT("A* reaches the same target"), Subsystem->FindPath(Unit, 0, AStarPath));
	TestTrue(
		TEXT("Reachable and A* costs agree"),
		FMath::IsNearlyEqual(RebuiltPath.TotalCost, AStarPath.TotalCost));

	FE2GridReachableResult InvalidBudget;
	TestFalse(TEXT("Negative budget is rejected"), Subsystem->FindReachableCells(Unit, -1.0f, InvalidBudget));
	TestEqual(TEXT("Invalid budget has a reason"), InvalidBudget.Status, EE2GridQueryStatus::InvalidRequest);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridRangeTest,
	"E2Grid.Runtime.Query.TopologyRangeMetricsAndDisconnectedAreas",
	TestFlags)

bool FE2GridRangeTest::RunTest(const FString& Parameters)
{
	{
		FRuntimeWorldFixture Fixture;
		AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(3, 3)));
		UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
		Subsystem->RegisterManager(Manager);
		FE2GridRangeResult ZeroRange;
		TestTrue(TEXT("Zero range succeeds"), Subsystem->FindCellsInRange(4, 0.0f, EE2GridRangeMetric::StepCount, ZeroRange));
		TestEqual(TEXT("Zero range contains only start"), ZeroRange.Cells.Num(), 1);

		FE2GridRangeResult StepRange;
		TestTrue(TEXT("Step range succeeds"), Subsystem->FindCellsInRange(4, 1.0f, EE2GridRangeMetric::StepCount, StepRange));
		TestEqual(TEXT("One topological step includes diagonals"), StepRange.Cells.Num(), 9);
		FE2GridRangeResult CostRange;
		TestTrue(TEXT("Cost range succeeds"), Subsystem->FindCellsInRange(4, 1.0f, EE2GridRangeMetric::TraversalCost, CostRange));
		TestEqual(TEXT("One traversal cost excludes diagonals"), CostRange.Cells.Num(), 5);
	}

	{
		FRuntimeWorldFixture Fixture;
		AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(3, 1), {1}));
		UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
		Subsystem->RegisterManager(Manager);
		FE2GridRangeResult DisconnectedRange;
		TestTrue(
			TEXT("Disconnected range query succeeds"),
			Subsystem->FindCellsInRange(0, 10.0f, EE2GridRangeMetric::StepCount, DisconnectedRange));
		TestEqual(TEXT("Range does not cross a disconnected area"), DisconnectedRange.Cells.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridMovementStaleRollbackTest,
	"E2Grid.Runtime.Movement.StateChangeDuringMoveRollsBack",
	TestFlags)

bool FE2GridMovementStaleRollbackTest::RunTest(const FString& Parameters)
{
	FRuntimeWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.AddManager(CreateMapAsset(FIntPoint(2, 2)));
	UE2GridSubsystem* Subsystem = Fixture.World->GetSubsystem<UE2GridSubsystem>();
	Subsystem->RegisterManager(Manager);
	UE2GridUnitComponent* Unit = Fixture.AddUnit(FVector::ZeroVector, TEXT("StaleMovingUnit"));
	Subsystem->RegisterUnit(Unit);
	UE2GridMovementComponent* Movement = NewObject<UE2GridMovementComponent>(Unit->GetOwner(), TEXT("StaleMovement"));
	Unit->GetOwner()->AddInstanceComponent(Movement);
	Movement->RegisterComponent();
	Movement->bSweepDuringMovement = false;
	TestTrue(TEXT("Movement starts from a versioned path"), Movement->MoveToCell(1));

	UE2GridUnitComponent* UnrelatedUnit = Fixture.AddUnit(FVector(50.0f, 50.0f, 0.0f), TEXT("MidMoveUnit"));
	Subsystem->RegisterUnit(UnrelatedUnit);
	Movement->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Stale movement finishes as failure"), Movement->IsMoving());
	TestEqual(TEXT("Failed movement keeps source cell"), Unit->GetCurrentCellKey(), 0);
	TestEqual(TEXT("Failed movement keeps source occupancy"), Subsystem->GetCellOwner(0), Unit);
	TestNull(TEXT("Failed movement does not occupy goal"), Subsystem->GetCellOwner(1));
	TestTrue(
		TEXT("Failed movement restores source center"),
		Unit->GetOwner()->GetActorLocation().Equals(FVector::ZeroVector));
	return true;
}

#endif
