#pragma once

#include "CoreMinimal.h"

class AE2GridManager;
class UE2GridMapAsset;

struct FE2GridBuildReport
{
	bool bSucceeded = false;
	int32 SampledCells = 0;
	int32 RetainedCells = 0;
	int32 FilteredCells = 0;
	int32 ConnectedComponents = 0;
	double GroundSamplingMs = 0.0;
	double GeometryFilteringMs = 0.0;
	double NeighborBuildMs = 0.0;
	double ConnectivityValidationMs = 0.0;
	double AssetCommitMs = 0.0;
	double TotalMs = 0.0;
	FString Message;

	FString ToSummary() const;
};

class FE2GridBuilder
{
public:
	static FE2GridBuildReport Build(
		AE2GridManager& Manager,
		UE2GridMapAsset& TargetAsset,
		const FIntPoint& GridDimension,
		float CellSize);
};
