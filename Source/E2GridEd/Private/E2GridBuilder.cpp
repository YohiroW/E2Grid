#include "E2GridBuilder.h"

#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "E2GridRuntimeData.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopedSlowTask.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

DEFINE_LOG_CATEGORY_STATIC(LogE2GridBuild, Log, All);

FString FE2GridBuildReport::ToSummary() const
{
	if (!bSucceeded)
	{
		return FString::Printf(TEXT("Build failed: %s"), *Message);
	}
	return FString::Printf(
		TEXT("Build succeeded: %d sampled, %d retained, %d filtered, %d component(s), %.2f ms total%s"),
		SampledCells,
		RetainedCells,
		FilteredCells,
		ConnectedComponents,
		TotalMs,
		ConnectedComponents > 1 ? TEXT(" (disconnected areas retained)") : TEXT(""));
}

FE2GridBuildReport FE2GridBuilder::Build(
	AE2GridManager& Manager,
	const FIntPoint& GridDimension,
	float CellSize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_EditorBuild);
	FE2GridBuildReport Report;
	const double TotalStart = FPlatformTime::Seconds();
	UE2GridMapAsset* TargetAsset = Manager.GridMapAsset;
	UWorld* World = Manager.GetWorld();
	const FE2GridBuildSettings& Settings = Manager.BuildSettings;

	if (!World || !TargetAsset || GridDimension.X <= 0 || GridDimension.Y <= 0 ||
		!FMath::IsFinite(CellSize) || CellSize <= UE_SMALL_NUMBER || !Settings.IsValid())
	{
		Report.Message = TEXT("Manager, target asset, layout, or build settings are invalid.");
		return Report;
	}

	FE2GridMapLayout NewLayout;
	NewLayout.CellSize = CellSize;
	NewLayout.GridDimension = GridDimension;
	NewLayout.LocalOrigin = FVector(
		-static_cast<double>(GridDimension.X - 1) * CellSize * 0.5,
		-static_cast<double>(GridDimension.Y - 1) * CellSize * 0.5,
		0.0);
	if (!NewLayout.IsValid())
	{
		Report.Message = TEXT("Draft layout is invalid.");
		return Report;
	}

	const int64 SampleCount64 = static_cast<int64>(GridDimension.X) * GridDimension.Y;
	if (SampleCount64 > MAX_int32)
	{
		Report.Message = TEXT("Grid dimensions exceed the supported v0.1 cell count.");
		return Report;
	}
	Report.SampledCells = static_cast<int32>(SampleCount64);

	FScopedSlowTask SlowTask(3.0f, NSLOCTEXT("E2GridBuild", "Building", "Building E2Grid map..."));
	SlowTask.MakeDialogDelayed(0.5f);
	TMap<int32, FE2GridCellData> NewCells;
	NewCells.Reserve(Report.SampledCells);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(E2GridBuild), false, &Manager);
	const FVector LocalUp = Manager.GetActorTransform().TransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	const double SamplingStart = FPlatformTime::Seconds();

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_GroundSampling);
		for (int32 Y = 0; Y < GridDimension.Y; ++Y)
		{
			for (int32 X = 0; X < GridDimension.X; ++X)
			{
				const FE2GridCoord Coord(X, Y);
				const FVector LocalCenter = NewLayout.GetCellLocalCenter(Coord);
				const FVector CandidateWorld = Manager.GetActorTransform().TransformPosition(LocalCenter);
				const FVector TraceStart = CandidateWorld + LocalUp * Settings.TraceStartHeight;
				const FVector TraceEnd = CandidateWorld - LocalUp * Settings.TraceDepth;
				FHitResult GroundHit;
				if (!World->LineTraceSingleByChannel(
					GroundHit,
					TraceStart,
					TraceEnd,
					Settings.GroundChannel,
					QueryParams))
				{
					continue;
				}

				const float UpDot = FVector::DotProduct(GroundHit.ImpactNormal.GetSafeNormal(), LocalUp);
				const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(UpDot, -1.0f, 1.0f)));
				if (SlopeDegrees > Settings.MaxSlope)
				{
					continue;
				}

				const FVector GroundLocal = Manager.GetActorTransform().InverseTransformPosition(GroundHit.ImpactPoint);
				const FVector AgentCenter = GroundHit.ImpactPoint + LocalUp * (Settings.AgentHeight * 0.5f + 0.5f);
				const FCollisionShape AgentShape = FCollisionShape::MakeCapsule(
					Settings.AgentRadius,
					FMath::Max(Settings.AgentHeight * 0.5f, Settings.AgentRadius));
				if (World->OverlapBlockingTestByChannel(
					AgentCenter,
					Manager.GetActorQuat(),
					Settings.ObstacleChannel,
					AgentShape,
					QueryParams))
				{
					continue;
				}

				FE2GridCellData Cell;
				Cell.LocalHeight = static_cast<float>(GroundLocal.Z - NewLayout.LocalOrigin.Z);
				Cell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
				Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
				NewCells.Add(NewLayout.CoordToKey(Coord), Cell);
			}
		}
	}
	Report.GroundSamplingMs = (FPlatformTime::Seconds() - SamplingStart) * 1000.0;
	Report.GeometryFilteringMs = Report.GroundSamplingMs;
	Report.RetainedCells = NewCells.Num();
	Report.FilteredCells = Report.SampledCells - Report.RetainedCells;
	SlowTask.EnterProgressFrame(1.0f);

	if (NewCells.IsEmpty())
	{
		Report.Message = TEXT("Collision sampling produced no valid cells; the old asset data was preserved.");
		return Report;
	}

	const double NeighborStart = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_NeighborBuild);
		for (TPair<int32, FE2GridCellData>& Pair : NewCells)
		{
			FE2GridCoord Coord;
			NewLayout.KeyToCoord(Pair.Key, Coord);
			for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
			{
				const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
				const FE2GridCoord NeighborCoord(Coord.X + Offset.X, Coord.Y + Offset.Y);
				const int32 NeighborKey = NewLayout.CoordToKey(NeighborCoord);
				const FE2GridCellData* Neighbor = NewCells.Find(NeighborKey);
				if (!Neighbor || FMath::Abs(Pair.Value.LocalHeight - Neighbor->LocalHeight) > Settings.MaxStepHeight)
				{
					continue;
				}

				if (E2GridDirections::IsDiagonal(DirectionIndex))
				{
					const int32 OrthogonalA = NewLayout.CoordToKey(FE2GridCoord(Coord.X + Offset.X, Coord.Y));
					const int32 OrthogonalB = NewLayout.CoordToKey(FE2GridCoord(Coord.X, Coord.Y + Offset.Y));
					const FE2GridCellData* CellA = NewCells.Find(OrthogonalA);
					const FE2GridCellData* CellB = NewCells.Find(OrthogonalB);
					if (!CellA || !CellB ||
						FMath::Abs(Pair.Value.LocalHeight - CellA->LocalHeight) > Settings.MaxStepHeight ||
						FMath::Abs(Pair.Value.LocalHeight - CellB->LocalHeight) > Settings.MaxStepHeight)
					{
						continue;
					}
				}

				Pair.Value.NeighborMask |= static_cast<uint8>(1u << DirectionIndex);
			}
		}
	}
	Report.NeighborBuildMs = (FPlatformTime::Seconds() - NeighborStart) * 1000.0;
	SlowTask.EnterProgressFrame(1.0f);

	const double ValidationStart = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_ConnectivityValidation);
		for (const TPair<int32, FE2GridCellData>& Pair : NewCells)
		{
			FE2GridCoord Coord;
			NewLayout.KeyToCoord(Pair.Key, Coord);
			for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
			{
				if ((Pair.Value.NeighborMask & (1u << DirectionIndex)) == 0)
				{
					continue;
				}
				const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
				const int32 NeighborKey = NewLayout.CoordToKey(
					FE2GridCoord(Coord.X + Offset.X, Coord.Y + Offset.Y));
				if (!NewCells.Contains(NeighborKey))
				{
					Report.Message = TEXT("Neighbor validation failed; the old asset data was preserved.");
					return Report;
				}
			}
		}

		TSet<int32> Visited;
		for (const TPair<int32, FE2GridCellData>& Seed : NewCells)
		{
			if (Visited.Contains(Seed.Key))
			{
				continue;
			}
			++Report.ConnectedComponents;
			TArray<int32> Queue;
			Queue.Add(Seed.Key);
			Visited.Add(Seed.Key);
			for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
			{
				const int32 CurrentKey = Queue[QueueIndex];
				const FE2GridCellData& CurrentCell = NewCells.FindChecked(CurrentKey);
				FE2GridCoord CurrentCoord;
				NewLayout.KeyToCoord(CurrentKey, CurrentCoord);
				for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
				{
					if ((CurrentCell.NeighborMask & (1u << DirectionIndex)) == 0)
					{
						continue;
					}
					const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
					const int32 NeighborKey = NewLayout.CoordToKey(
						FE2GridCoord(CurrentCoord.X + Offset.X, CurrentCoord.Y + Offset.Y));
					if (!Visited.Contains(NeighborKey))
					{
						Visited.Add(NeighborKey);
						Queue.Add(NeighborKey);
					}
				}
			}
		}
	}
	Report.ConnectivityValidationMs = (FPlatformTime::Seconds() - ValidationStart) * 1000.0;
	SlowTask.EnterProgressFrame(1.0f);

	const double CommitStart = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_AssetCommit);
		TargetAsset->Modify();
		TargetAsset->ReplaceData(NewLayout, MoveTemp(NewCells));
		TargetAsset->MarkPackageDirty();
		Manager.RefreshVisualization();
	}
	Report.AssetCommitMs = (FPlatformTime::Seconds() - CommitStart) * 1000.0;
	Report.TotalMs = (FPlatformTime::Seconds() - TotalStart) * 1000.0;
	Report.bSucceeded = true;
	Report.Message = Report.ConnectedComponents > 1
		? TEXT("Build completed with disconnected areas; all valid areas were retained.")
		: TEXT("Build completed.");

	UE_LOG(
		LogE2GridBuild,
		Display,
		TEXT("%s | sampling %.2f ms, filtering %.2f ms, neighbors %.2f ms, validation %.2f ms, commit %.2f ms"),
		*Report.ToSummary(),
		Report.GroundSamplingMs,
		Report.GeometryFilteringMs,
		Report.NeighborBuildMs,
		Report.ConnectivityValidationMs,
		Report.AssetCommitMs);
	return Report;
}
