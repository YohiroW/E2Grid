#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "E2GridCombatUnit.h"
#include "E2GridGameModeBase.h"
#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "E2GridRuntimeData.h"
#include "E2GridUnitComponent.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"

namespace
{
	constexpr EAutomationTestFlags GameTestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	UE2GridMapAsset* CreateTwoCellGameAsset()
	{
		UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>(GetTransientPackage());
		FE2GridMapLayout Layout;
		Layout.CellSize = 50.0f;
		Layout.GridDimension = FIntPoint(2, 1);

		FE2GridCellData LeftCell;
		LeftCell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
		LeftCell.SetFlag(EE2GridCellFlags::CanStandOn, true);
		LeftCell.NeighborMask = 1u << 2;
		FE2GridCellData RightCell;
		RightCell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
		RightCell.SetFlag(EE2GridCellFlags::CanStandOn, true);
		RightCell.NeighborMask = 1u << 6;

		TMap<int32, FE2GridCellData> Cells;
		Cells.Add(0, LeftCell);
		Cells.Add(1, RightCell);
		Asset->ReplaceData(Layout, MoveTemp(Cells));
		return Asset;
	}

	UE2GridMapAsset* CreateThreeCellLineAsset(bool bOmitMiddleCell = false)
	{
		UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>(GetTransientPackage());
		FE2GridMapLayout Layout;
		Layout.CellSize = 50.0f;
		Layout.GridDimension = FIntPoint(3, 1);
		TMap<int32, FE2GridCellData> Cells;
		for (int32 CellKey = 0; CellKey < 3; ++CellKey)
		{
			if (bOmitMiddleCell && CellKey == 1)
			{
				continue;
			}
			FE2GridCellData Cell;
			Cell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
			Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
			if (!bOmitMiddleCell)
			{
				Cell.NeighborMask = static_cast<uint8>(
					(CellKey < 2 ? (1u << 2) : 0) |
					(CellKey > 0 ? (1u << 6) : 0));
			}
			Cells.Add(CellKey, Cell);
		}
		Asset->ReplaceData(Layout, MoveTemp(Cells));
		return Asset;
	}

	class FGameWorldFixture
	{
	public:
		FGameWorldFixture()
		{
			FWorldInitializationValues InitializationValues;
			InitializationValues
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false)
				.CreateFXSystem(false)
				.SetDefaultGameMode(AE2GridGameModeBase::StaticClass());
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				MakeUniqueObjectName(
					GetTransientPackage(),
					UWorld::StaticClass(),
					TEXT("E2GridGameTestWorld")),
				GetTransientPackage(),
				true,
				ERHIFeatureLevel::Num,
				&InitializationValues);
			check(World);
			WorldContext.SetCurrentWorld(World);
		}

		~FGameWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(true);
				GEngine->DestroyWorldContext(World);
				World->RemoveFromRoot();
			}
		}

		UWorld* World = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridGameExampleMapTest,
	"E2Grid.Game.ExampleMap.ContentAndAsset",
	GameTestFlags)

bool FE2GridGameExampleMapTest::RunTest(const FString& Parameters)
{
	UWorld* ExampleWorld = LoadObject<UWorld>(
		nullptr,
		TEXT("/E2Grid/Examples/Showcase_v01.Showcase_v01"));
	TestNotNull(TEXT("The v0.1 example map loads from disk"), ExampleWorld);
	if (!ExampleWorld || !ExampleWorld->PersistentLevel)
	{
		return false;
	}

	AE2GridManager* Manager = nullptr;
	int32 ManagerCount = 0;
	int32 StaticMeshActorCount = 0;
	int32 PlayerStartCount = 0;
	int32 PlayerCount = 0;
	int32 EnemyCount = 0;
	int32 NeutralCount = 0;
	TArray<AE2GridCombatUnit*> CombatUnits;
	for (AActor* Actor : ExampleWorld->PersistentLevel->Actors)
	{
		if (AE2GridManager* CandidateManager = Cast<AE2GridManager>(Actor))
		{
			Manager = CandidateManager;
			++ManagerCount;
		}
		else if (AE2GridCombatUnit* CombatUnit = Cast<AE2GridCombatUnit>(Actor))
		{
			CombatUnits.Add(CombatUnit);
			switch (CombatUnit->GetTeam())
			{
			case EE2GridTeam::Player:
				++PlayerCount;
				break;
			case EE2GridTeam::Enemy:
				++EnemyCount;
				break;
			case EE2GridTeam::Neutral:
				++NeutralCount;
				break;
			}
		}
		StaticMeshActorCount += Actor && Actor->IsA<AStaticMeshActor>() ? 1 : 0;
		PlayerStartCount += Actor && Actor->IsA<APlayerStart>() ? 1 : 0;
	}

	TestEqual(TEXT("The example contains one grid manager"), ManagerCount, 1);
	TestEqual(TEXT("The example contains four collision scene blocks"), StaticMeshActorCount, 4);
	TestEqual(TEXT("The example contains one camera start"), PlayerStartCount, 1);
	TestEqual(TEXT("The example contains one player unit"), PlayerCount, 1);
	TestEqual(TEXT("The example contains one enemy unit"), EnemyCount, 1);
	TestEqual(TEXT("The example contains one neutral blocker"), NeutralCount, 1);
	TestTrue(
		TEXT("The example uses E2GridGameModeBase"),
		ExampleWorld->GetWorldSettings()->DefaultGameMode == AE2GridGameModeBase::StaticClass());
	TestNotNull(TEXT("The manager references a collision-built asset"), Manager ? Manager->GridMapAsset.Get() : nullptr);
	if (!Manager || !Manager->GridMapAsset)
	{
		return false;
	}

	const UE2GridMapAsset& Asset = *Manager->GridMapAsset;
	TSet<int32> UnitCellKeys;
	for (const AE2GridCombatUnit* CombatUnit : CombatUnits)
	{
		int32 CellKey = INVALID_GRID_KEY;
		TestTrue(
			TEXT("Every example combat unit is placed on a grid cell"),
			Manager->WorldToCell(CombatUnit->GetActorLocation(), CellKey));
		const FE2GridCellData* Cell = Manager->FindCell(CellKey);
		TestTrue(
			TEXT("Every example combat unit starts on a standable cell"),
			Cell && Cell->CanStandOn());
		TestFalse(TEXT("Example combat units occupy distinct cells"), UnitCellKeys.Contains(CellKey));
		UnitCellKeys.Add(CellKey);
		const UCapsuleComponent* Capsule = CombatUnit->FindComponentByClass<UCapsuleComponent>();
		TestTrue(
			TEXT("Every example combat unit is clickable by the visibility trace"),
			Capsule && Capsule->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
	}
	FString ValidationError;
	TestTrue(TEXT("The example grid asset validates"), Asset.Validate(&ValidationError));
	if (!ValidationError.IsEmpty())
	{
		AddError(ValidationError);
	}
	TestTrue(TEXT("The example grid asset uses the current build version"), Asset.IsCurrentBuildVersion());
	TestEqual(TEXT("The collision build retained 139 sparse cells"), Asset.GetCells().Num(), 139);
	const FE2GridMapLayout& Layout = Asset.GetLayout();
	TestNotNull(
		TEXT("The main floor produces a standable cell"),
		Asset.FindCell(Layout.CoordToKey(FE2GridCoord(0, 0))));
	TestNull(
		TEXT("The vertical wall removes its occupied cell"),
		Asset.FindCell(Layout.CoordToKey(FE2GridCoord(5, 5))));
	TestNull(
		TEXT("The horizontal wall removes its occupied cell"),
		Asset.FindCell(Layout.CoordToKey(FE2GridCoord(6, 8))));
	TestNull(
		TEXT("The gap between the floor and island has no cell"),
		Asset.FindCell(Layout.CoordToKey(FE2GridCoord(11, 7))));
	TestNotNull(
		TEXT("The disconnected island produces standable cells"),
		Asset.FindCell(Layout.CoordToKey(FE2GridCoord(12, 7))));

	TSet<int32> Visited;
	int32 ConnectedComponents = 0;
	for (const TPair<int32, FE2GridCellData>& Seed : Asset.GetCells())
	{
		if (Visited.Contains(Seed.Key))
		{
			continue;
		}
		++ConnectedComponents;
		TArray<int32> Queue;
		Queue.Add(Seed.Key);
		Visited.Add(Seed.Key);
		for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
		{
			const int32 CurrentKey = Queue[QueueIndex];
			Manager->ForEachTraversableNeighbor(
				CurrentKey,
				[&Visited, &Queue](int32 NeighborKey, float)
				{
					if (!Visited.Contains(NeighborKey))
					{
						Visited.Add(NeighborKey);
						Queue.Add(NeighborKey);
					}
				});
		}
	}
	TestTrue(TEXT("The example retains disconnected walkable areas"), ConnectedComponents >= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridGameAdjacentAttackTurnTest,
	"E2Grid.Game.AdjacentAttackAndEnemyResponse",
	GameTestFlags)

bool FE2GridGameAdjacentAttackTurnTest::RunTest(const FString& Parameters)
{
	FGameWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.World->SpawnActor<AE2GridManager>();
	Manager->GridMapAsset = CreateTwoCellGameAsset();
	AE2GridCombatUnit* Player = Fixture.World->SpawnActor<AE2GridCombatUnit>(FVector::ZeroVector, FRotator::ZeroRotator);
	AE2GridCombatUnit* Enemy = Fixture.World->SpawnActor<AE2GridCombatUnit>(FVector(50.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	TestTrue(TEXT("Player team can be configured before play"), Player->SetTeamForSetup(EE2GridTeam::Player));
	TestTrue(TEXT("Enemy team can be configured before play"), Enemy->SetTeamForSetup(EE2GridTeam::Enemy));
	const UCapsuleComponent* PlayerCapsule = Player->FindComponentByClass<UCapsuleComponent>();
	TestNotNull(TEXT("Combat unit has a clickable collision capsule"), PlayerCapsule);
	if (PlayerCapsule)
	{
		TestEqual(
			TEXT("Mouse visibility traces hit combat units"),
			PlayerCapsule->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
	}

	Fixture.World->InitializeActorsForPlay(FURL());
	Player->DispatchBeginPlay();
	Enemy->DispatchBeginPlay();
	Manager->DispatchBeginPlay();
	AE2GridGameModeBase* GameMode = Fixture.World->SpawnActor<AE2GridGameModeBase>();
	GameMode->DispatchBeginPlay();
	TestNotNull(TEXT("E2Grid game mode is active"), GameMode);
	TestTrue(TEXT("Player registered to the grid"), Player->GetGridUnit()->IsRegistered());
	TestTrue(TEXT("Enemy registered to the grid"), Enemy->GetGridUnit()->IsRegistered());
	TestTrue(TEXT("Player can act at turn start"), GameMode && GameMode->CanPlayerIssueAction());

	TestTrue(TEXT("Adjacent player attack is accepted"), GameMode->SubmitPlayerAttack(Player, Enemy));
	TestEqual(TEXT("Player attack applies fixed damage"), Enemy->GetHealth(), 75);
	TestEqual(TEXT("Enemy immediately performs adjacent response"), Player->GetHealth(), 75);
	TestEqual(TEXT("Turn returns to player"), GameMode->GetTurnState(), EE2GridTurnState::PlayerTurn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridGameEnemyApproachTest,
	"E2Grid.Game.EnemyApproachesWhenNotAdjacent",
	GameTestFlags)

bool FE2GridGameEnemyApproachTest::RunTest(const FString& Parameters)
{
	FGameWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.World->SpawnActor<AE2GridManager>();
	Manager->GridMapAsset = CreateThreeCellLineAsset();
	AE2GridCombatUnit* Player = Fixture.World->SpawnActor<AE2GridCombatUnit>(
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	AE2GridCombatUnit* Enemy = Fixture.World->SpawnActor<AE2GridCombatUnit>(
		FVector(100.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator);
	Player->SetTeamForSetup(EE2GridTeam::Player);
	Enemy->SetTeamForSetup(EE2GridTeam::Enemy);

	Fixture.World->InitializeActorsForPlay(FURL());
	Player->DispatchBeginPlay();
	Enemy->DispatchBeginPlay();
	Manager->DispatchBeginPlay();
	AE2GridGameModeBase* GameMode = Fixture.World->SpawnActor<AE2GridGameModeBase>();
	GameMode->DispatchBeginPlay();

	TestTrue(TEXT("Player can skip to start the enemy turn"), GameMode->SkipPlayerAction());
	TestTrue(TEXT("Enemy starts moving toward the player"), Enemy->GetGridMovement()->IsMoving());
	Enemy->GetGridMovement()->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Enemy reaches the adjacent cell"), Enemy->GetGridUnit()->GetCurrentCellKey(), 1);
	TestFalse(TEXT("Enemy movement completes"), Enemy->GetGridMovement()->IsMoving());
	TestEqual(TEXT("Turn returns after enemy movement"), GameMode->GetTurnState(), EE2GridTurnState::PlayerTurn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridGameEnemyNoPathTest,
	"E2Grid.Game.EnemySkipsWhenNoPath",
	GameTestFlags)

bool FE2GridGameEnemyNoPathTest::RunTest(const FString& Parameters)
{
	FGameWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.World->SpawnActor<AE2GridManager>();
	Manager->GridMapAsset = CreateThreeCellLineAsset(true);
	AE2GridCombatUnit* Player = Fixture.World->SpawnActor<AE2GridCombatUnit>(
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	AE2GridCombatUnit* Enemy = Fixture.World->SpawnActor<AE2GridCombatUnit>(
		FVector(100.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator);
	Player->SetTeamForSetup(EE2GridTeam::Player);
	Enemy->SetTeamForSetup(EE2GridTeam::Enemy);

	Fixture.World->InitializeActorsForPlay(FURL());
	Player->DispatchBeginPlay();
	Enemy->DispatchBeginPlay();
	Manager->DispatchBeginPlay();
	AE2GridGameModeBase* GameMode = Fixture.World->SpawnActor<AE2GridGameModeBase>();
	GameMode->DispatchBeginPlay();

	TestTrue(TEXT("Player can skip to start the enemy turn"), GameMode->SkipPlayerAction());
	TestFalse(TEXT("Enemy does not move when no path exists"), Enemy->GetGridMovement()->IsMoving());
	TestEqual(TEXT("Enemy stays on its source cell"), Enemy->GetGridUnit()->GetCurrentCellKey(), 2);
	TestEqual(TEXT("No-path enemy turn returns to player"), GameMode->GetTurnState(), EE2GridTurnState::PlayerTurn);
	return true;
}

#endif
