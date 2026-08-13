#include "E2GridBuilder.h"

#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "E2GridRuntimeData.h"
#include "E2GridUnitComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopedSlowTask.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

DEFINE_LOG_CATEGORY_STATIC(LogE2GridBuild, Log, All);

namespace
{
	struct FGroundCandidate
	{
		int32 CellKey = INVALID_GRID_KEY;
		FVector ImpactPoint = FVector::ZeroVector;
		float LocalHeight = 0.0f;
	};

	bool ValidateTemporaryData(
		const FE2GridMapLayout& Layout,
		const TMap<int32, FE2GridCellData>& Cells,
		FString& OutError)
	{
		if (!Layout.IsValid() || Cells.IsEmpty())
		{
			OutError = TEXT("The temporary grid layout or cells are invalid.");
			return false;
		}

		for (const TPair<int32, FE2GridCellData>& Pair : Cells)
		{
			FE2GridCoord Coord;
			if (!Layout.KeyToCoord(Pair.Key, Coord) || !FMath::IsFinite(Pair.Value.LocalHeight))
			{
				OutError = FString::Printf(TEXT("Cell %d has an invalid key or height."), Pair.Key);
				return false;
			}

			for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; ++DirectionIndex)
			{
				if ((Pair.Value.NeighborMask & (1u << DirectionIndex)) == 0)
				{
					continue;
				}

				const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
				const int32 NeighborKey = Layout.CoordToKey(
					FE2GridCoord(Coord.X + Offset.X, Coord.Y + Offset.Y));
				if (NeighborKey == INVALID_GRID_KEY || !Cells.Contains(NeighborKey))
				{
					OutError = FString::Printf(
						TEXT("Cell %d has a traversal bit pointing to a missing cell."),
						Pair.Key);
					return false;
				}

				if (E2GridDirections::IsDiagonal(DirectionIndex))
				{
					const int32 OrthogonalA = (DirectionIndex + E2GridDirections::Count - 1) % E2GridDirections::Count;
					const int32 OrthogonalB = (DirectionIndex + 1) % E2GridDirections::Count;
					if ((Pair.Value.NeighborMask & (1u << OrthogonalA)) == 0 ||
						(Pair.Value.NeighborMask & (1u << OrthogonalB)) == 0)
					{
						OutError = FString::Printf(
							TEXT("Cell %d has a diagonal traversal that cuts a blocked corner."),
							Pair.Key);
						return false;
					}
				}
			}
		}

		return true;
	}
}

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
	UE2GridMapAsset& TargetAsset,
	const FIntPoint& GridDimension,
	float CellSize)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_EditorBuild);
	FE2GridBuildReport Report;
	const auto FailBuild = [&Report](const FString& Message)
	{
		Report.Message = Message;
		UE_LOG(LogE2GridBuild, Error, TEXT("%s"), *Report.ToSummary());
		return Report;
	};
	const double TotalStart = FPlatformTime::Seconds();
	UWorld* World = Manager.GetWorld();
	const FE2GridBuildSettings& Settings = Manager.BuildSettings;

	if (!World || GridDimension.X <= 0 || GridDimension.Y <= 0 ||
		!FMath::IsFinite(CellSize) || CellSize <= UE_SMALL_NUMBER || !Settings.IsValid())
	{
		return FailBuild(TEXT("Manager world, draft layout, or build settings are invalid."));
	}

	if (!Manager.GetActorScale3D().Equals(FVector::OneVector))
	{
		return FailBuild(TEXT("Grid Manager scale must be (1,1,1); use Cell Size to control spacing."));
	}

	FE2GridMapLayout NewLayout;
	NewLayout.CellSize = CellSize;
	NewLayout.GridDimension = GridDimension;
	NewLayout.LocalOrigin = FVector(
		-static_cast<double>(GridDimension.X - 1) * CellSize * 0.5,
		-static_cast<double>(GridDimension.Y - 1) * CellSize * 0.5,
		0.0);

	const int64 SampleCount64 = static_cast<int64>(GridDimension.X) * GridDimension.Y;
	if (!NewLayout.IsValid() || SampleCount64 > MAX_int32)
	{
		return FailBuild(TEXT("Draft layout exceeds the supported v0.1 cell count."));
	}
	Report.SampledCells = static_cast<int32>(SampleCount64);

	FScopedSlowTask SlowTask(4.0f, NSLOCTEXT("E2GridBuild", "Building", "Building E2Grid map..."));
	SlowTask.MakeDialogDelayed(0.5f);
	TArray<FGroundCandidate> GroundCandidates;
	GroundCandidates.Reserve(Report.SampledCells);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(E2GridBuild), false, &Manager);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->FindComponentByClass<UE2GridUnitComponent>())
		{
			QueryParams.AddIgnoredActor(*It);
		}
	}
	const FTransform ManagerTransform = Manager.GetActorTransform();
	const FVector LocalUp = ManagerTransform.TransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	const float CapsuleHalfHeight = FMath::Max(Settings.AgentHeight * 0.5f, Settings.AgentRadius);
	const FCollisionShape AgentShape = FCollisionShape::MakeCapsule(Settings.AgentRadius, CapsuleHalfHeight);
	const double SamplingStart = FPlatformTime::Seconds();

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_GroundSampling);
		for (int32 Y = 0; Y < GridDimension.Y; ++Y)
		{
			for (int32 X = 0; X < GridDimension.X; ++X)
			{
				const FE2GridCoord Coord(X, Y);
				const FVector CandidateWorld = ManagerTransform.TransformPosition(
					NewLayout.GetCellLocalCenter(Coord));
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
				const float SlopeDegrees = FMath::RadiansToDegrees(
					FMath::Acos(FMath::Clamp(UpDot, -1.0f, 1.0f)));
				if (SlopeDegrees > Settings.MaxSlope)
				{
					continue;
				}

				const FVector GroundLocal = ManagerTransform.InverseTransformPosition(GroundHit.ImpactPoint);
				FGroundCandidate& Candidate = GroundCandidates.AddDefaulted_GetRef();
				Candidate.CellKey = NewLayout.CoordToKey(Coord);
				Candidate.ImpactPoint = GroundHit.ImpactPoint;
				Candidate.LocalHeight = static_cast<float>(GroundLocal.Z - NewLayout.LocalOrigin.Z);
			}
		}
	}

	Report.GroundSamplingMs = (FPlatformTime::Seconds() - SamplingStart) * 1000.0;
	SlowTask.EnterProgressFrame(1.0f);

	TMap<int32, FE2GridCellData> NewCells;
	NewCells.Reserve(GroundCandidates.Num());
	const double GeometryFilteringStart = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_GeometryFiltering);
		for (const FGroundCandidate& Candidate : GroundCandidates)
		{
			const FVector AgentCenter = Candidate.ImpactPoint + LocalUp * (CapsuleHalfHeight + 0.5f);
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
			Cell.LocalHeight = Candidate.LocalHeight;
			Cell.SetFlag(EE2GridCellFlags::CanWalkThrough, true);
			Cell.SetFlag(EE2GridCellFlags::CanStandOn, true);
			NewCells.Add(Candidate.CellKey, Cell);
		}
	}
	Report.GeometryFilteringMs = (FPlatformTime::Seconds() - GeometryFilteringStart) * 1000.0;
	Report.RetainedCells = NewCells.Num();
	Report.FilteredCells = Report.SampledCells - Report.RetainedCells;
	SlowTask.EnterProgressFrame(1.0f);
	if (NewCells.IsEmpty())
	{
		return FailBuild(TEXT("Collision sampling produced no valid cells; the old asset data was preserved."));
	}

	const auto CanTraverse = [
		World,
		&Manager,
		&NewLayout,
		&NewCells,
		&Settings,
		&QueryParams,
		&AgentShape,
		&LocalUp,
		CapsuleHalfHeight](int32 FromKey, int32 ToKey)
	{
		const FE2GridCellData* FromCell = NewCells.Find(FromKey);
		const FE2GridCellData* ToCell = NewCells.Find(ToKey);
		FE2GridCoord FromCoord;
		FE2GridCoord ToCoord;
		if (!FromCell || !ToCell ||
			!NewLayout.KeyToCoord(FromKey, FromCoord) || !NewLayout.KeyToCoord(ToKey, ToCoord) ||
			FMath::Abs(FromCell->LocalHeight - ToCell->LocalHeight) > Settings.MaxStepHeight)
		{
			return false;
		}

		const FTransform& Transform = Manager.GetActorTransform();
		const FVector StartGround = Transform.TransformPosition(
			NewLayout.GetCellLocalCenter(FromCoord, FromCell->LocalHeight));
		const FVector EndGround = Transform.TransformPosition(
			NewLayout.GetCellLocalCenter(ToCoord, ToCell->LocalHeight));
		const FVector StartCenter = StartGround + LocalUp * (CapsuleHalfHeight + 0.5f);
		const FVector EndCenter = EndGround + LocalUp * (CapsuleHalfHeight + 0.5f);
		FHitResult SweepHit;
		return !World->SweepSingleByChannel(
			SweepHit,
			StartCenter,
			EndCenter,
			Manager.GetActorQuat(),
			Settings.ObstacleChannel,
			AgentShape,
			QueryParams);
	};

	const double NeighborStart = FPlatformTime::Seconds();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(E2Grid_NeighborBuild);
		// Orthogonal traversal is established first so diagonal traversal can require both sides.
		for (TPair<int32, FE2GridCellData>& Pair : NewCells)
		{
			FE2GridCoord Coord;
			NewLayout.KeyToCoord(Pair.Key, Coord);
			for (int32 DirectionIndex = 0; DirectionIndex < E2GridDirections::Count; DirectionIndex += 2)
			{
				const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
				const int32 NeighborKey = NewLayout.CoordToKey(
					FE2GridCoord(Coord.X + Offset.X, Coord.Y + Offset.Y));
				if (CanTraverse(Pair.Key, NeighborKey))
				{
					Pair.Value.NeighborMask |= static_cast<uint8>(1u << DirectionIndex);
				}
			}
		}

		for (TPair<int32, FE2GridCellData>& Pair : NewCells)
		{
			FE2GridCoord Coord;
			NewLayout.KeyToCoord(Pair.Key, Coord);
			for (int32 DirectionIndex = 1; DirectionIndex < E2GridDirections::Count; DirectionIndex += 2)
			{
				const int32 OrthogonalA = (DirectionIndex + E2GridDirections::Count - 1) % E2GridDirections::Count;
				const int32 OrthogonalB = (DirectionIndex + 1) % E2GridDirections::Count;
				if ((Pair.Value.NeighborMask & (1u << OrthogonalA)) == 0 ||
					(Pair.Value.NeighborMask & (1u << OrthogonalB)) == 0)
				{
					continue;
				}

				const FIntPoint& Offset = E2GridDirections::GetOffset(DirectionIndex);
				const int32 NeighborKey = NewLayout.CoordToKey(
					FE2GridCoord(Coord.X + Offset.X, Coord.Y + Offset.Y));
				if (CanTraverse(Pair.Key, NeighborKey))
				{
					Pair.Value.NeighborMask |= static_cast<uint8>(1u << DirectionIndex);
				}
			}
		}
	}
	Report.NeighborBuildMs = (FPlatformTime::Seconds() - NeighborStart) * 1000.0;
	SlowTask.EnterProgressFrame(1.0f);

	const double ValidationStart = FPlatformTime::Seconds();
	if (!ValidateTemporaryData(NewLayout, NewCells, Report.Message))
	{
		return FailBuild(Report.Message);
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
	Report.ConnectivityValidationMs = (FPlatformTime::Seconds() - ValidationStart) * 1000.0;
	SlowTask.EnterProgressFrame(1.0f);

	const double CommitStart = FPlatformTime::Seconds();
	TargetAsset.Modify();
	TargetAsset.ReplaceData(NewLayout, MoveTemp(NewCells));
	TargetAsset.MarkPackageDirty();
	Manager.Modify();
	Manager.GridMapAsset = &TargetAsset;
	Manager.MarkPackageDirty();
	Manager.RefreshVisualization();
	Report.AssetCommitMs = (FPlatformTime::Seconds() - CommitStart) * 1000.0;
	Report.TotalMs = (FPlatformTime::Seconds() - TotalStart) * 1000.0;
	Report.bSucceeded = true;
	Report.Message = Report.ConnectedComponents > 1
		? TEXT("Build completed with disconnected areas; all valid areas were retained.")
		: TEXT("Build completed.");

	UE_LOG(
		LogE2GridBuild,
		Display,
		TEXT("%s | sampling %.2f ms, filtering %.2f ms, neighbors %.2f ms, connectivity/validation %.2f ms, commit %.2f ms"),
		*Report.ToSummary(),
		Report.GroundSamplingMs,
		Report.GeometryFilteringMs,
		Report.NeighborBuildMs,
		Report.ConnectivityValidationMs,
		Report.AssetCommitMs);
	return Report;
}
