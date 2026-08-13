#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "E2GridBuilder.h"
#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

namespace
{
	constexpr EAutomationTestFlags BuilderTestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	class FEditorBuilderWorldFixture
	{
	public:
		FEditorBuilderWorldFixture()
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
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Editor);
			World = UWorld::CreateWorld(
				EWorldType::Editor,
				false,
				MakeUniqueObjectName(
					GetTransientPackage(),
					UWorld::StaticClass(),
					TEXT("E2GridBuilderTestWorld")),
				GetTransientPackage(),
				true,
				ERHIFeatureLevel::Num,
				&InitializationValues);
			check(World);
			WorldContext.SetCurrentWorld(World);
		}

		~FEditorBuilderWorldFixture()
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

	UE2GridMapAsset* CreateSentinelAsset()
	{
		UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>(GetTransientPackage());
		FE2GridMapLayout Layout;
		Layout.GridDimension = FIntPoint(1, 1);
		FE2GridCellData Cell;
		Cell.LocalHeight = 17.0f;
		Cell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
		Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
		TMap<int32, FE2GridCellData> Cells;
		Cells.Add(0, Cell);
		Asset->ReplaceData(Layout, MoveTemp(Cells));
		return Asset;
	}

	AStaticMeshActor* SpawnCollisionBlock(
		UWorld& World,
		UStaticMesh& Mesh,
		const FVector& Location,
		const FVector& Scale,
		ECollisionResponse VisibilityResponse)
	{
		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Location, Scale));
		if (!Actor)
		{
			return nullptr;
		}
		UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetStaticMesh(&Mesh);
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, VisibilityResponse);
		return Actor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridBuilderAtomicFailureTest,
	"E2Grid.Editor.Builder.FailurePreservesAsset",
	BuilderTestFlags)

bool FE2GridBuilderAtomicFailureTest::RunTest(const FString& Parameters)
{
	FEditorBuilderWorldFixture Fixture;
	AE2GridManager* Manager = Fixture.World->SpawnActor<AE2GridManager>();
	UE2GridMapAsset* Asset = CreateSentinelAsset();
	Manager->GridMapAsset = Asset;

	AddExpectedError(
		TEXT("Build failed: Collision sampling produced no valid cells"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	const FE2GridBuildReport Report = FE2GridBuilder::Build(
		*Manager,
		*Asset,
		FIntPoint(2, 2),
		50.0f);
	TestFalse(TEXT("A world without ground fails to build"), Report.bSucceeded);
	TestEqual(TEXT("The old layout is preserved"), Asset->GetLayout().GridDimension, FIntPoint(1, 1));
	TestEqual(TEXT("The old cell count is preserved"), Asset->GetCells().Num(), 1);
	const FE2GridCellData* PreservedCell = Asset->FindCell(0);
	TestNotNull(TEXT("The old cell is preserved"), PreservedCell);
	if (PreservedCell)
	{
		TestEqual(TEXT("The old cell data is preserved"), PreservedCell->LocalHeight, 17.0f);
	}
	TestTrue(TEXT("The preserved asset still validates"), Asset->Validate());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FE2GridBuilderCollisionChannelsTest,
	"E2Grid.Editor.Builder.GroundAndObstacleChannels",
	BuilderTestFlags)

bool FE2GridBuilderCollisionChannelsTest::RunTest(const FString& Parameters)
{
	FEditorBuilderWorldFixture Fixture;
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	TestNotNull(TEXT("The engine cube mesh loads"), CubeMesh);
	if (!CubeMesh)
	{
		return false;
	}

	TestNotNull(
		TEXT("The test ground is created"),
		SpawnCollisionBlock(
			*Fixture.World,
			*CubeMesh,
			FVector(0.0f, 0.0f, -25.0f),
			FVector(1.5f, 0.5f, 0.5f),
			ECR_Block));
	TestNotNull(
		TEXT("The test wall is created"),
		SpawnCollisionBlock(
			*Fixture.World,
			*CubeMesh,
			FVector(0.0f, 0.0f, 50.0f),
			FVector(0.5f, 0.5f, 1.0f),
			ECR_Ignore));
	TestNotNull(
		TEXT("The test roof is created"),
		SpawnCollisionBlock(
			*Fixture.World,
			*CubeMesh,
			FVector(-50.0f, 0.0f, 130.0f),
			FVector(0.4f, 0.45f, 0.2f),
			ECR_Ignore));
	AE2GridManager* Manager = Fixture.World->SpawnActor<AE2GridManager>();
	Manager->BuildSettings.GroundChannel = ECC_Visibility;
	Manager->BuildSettings.ObstacleChannel = ECC_WorldStatic;
	Fixture.World->UpdateWorldComponents(true, false);

	UE2GridMapAsset* Asset = NewObject<UE2GridMapAsset>(GetTransientPackage());
	const FE2GridBuildReport Report = FE2GridBuilder::Build(
		*Manager,
		*Asset,
		FIntPoint(3, 1),
		50.0f);
	TestTrue(TEXT("Collision build succeeds"), Report.bSucceeded);
	TestEqual(TEXT("The wall filters its cell"), Report.RetainedCells, 2);
	TestEqual(TEXT("The wall separates the two ground cells"), Report.ConnectedComponents, 2);
	TestNotNull(TEXT("The cell beneath a sufficiently high roof is retained"), Asset->FindCell(0));
	TestNull(TEXT("The wall cell is omitted"), Asset->FindCell(1));
	TestNotNull(TEXT("The right ground cell is retained"), Asset->FindCell(2));
	TestTrue(TEXT("The collision-built asset validates"), Asset->Validate());
	return true;
}

#endif
