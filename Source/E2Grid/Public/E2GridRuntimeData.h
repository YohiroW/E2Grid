#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "E2GridRuntimeData.generated.h"

class UE2GridUnitComponent;

constexpr int32 INVALID_GRID_KEY = INDEX_NONE;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EE2GridCellFlags : uint8
{
	None = 0,
	CanWalkThrough = 1 << 0,
	CanStandOn = 1 << 1,
};
ENUM_CLASS_FLAGS(EE2GridCellFlags);

UENUM(BlueprintType)
enum class EE2GridPathStatus : uint8
{
	Success,
	InvalidUnit,
	InvalidGoal,
	GoalOccupied,
	NoPath,
};

/** Unified status for side-effect-free queries and versioned move commits. */
UENUM(BlueprintType)
enum class EE2GridQueryStatus : uint8
{
	Success,
	NoActiveGrid,
	InvalidUnit,
	InvalidCell,
	NotTraversable,
	NotStandable,
	Occupied,
	NoPath,
	StaleRevision,
	InvalidRequest,
};

UENUM(BlueprintType)
enum class EE2GridRangeMetric : uint8
{
	StepCount,
	TraversalCost,
};

UENUM(BlueprintType)
enum class EE2GridStateChangeKind : uint8
{
	None,
	ManagerChanged,
	UnitRegistered,
	UnitUnregistered,
	OccupancyMoved,
};

UENUM(BlueprintType)
enum class EE2GridRegistrationStatus : uint8
{
	Unregistered,
	Registered,
	PendingManager,
	InvalidLocation,
	CellNotStandable,
	CellOccupied,
	AlreadyRegistered,
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridCoord
{
	GENERATED_BODY()

	FE2GridCoord() = default;
	FE2GridCoord(int32 InX, int32 InY, int32 InLayer = 0)
		: X(InX), Y(InY), Layer(InLayer)
	{
	}

	static const FE2GridCoord INVALID_COORD;

	bool operator==(const FE2GridCoord& Other) const
	{
		return X == Other.X && Y == Other.Y && Layer == Other.Layer;
	}

	bool operator!=(const FE2GridCoord& Other) const { return !(*this == Other); }

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Coordinates")
	int32 X = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Coordinates")
	int32 Y = 0;

	/** Reserved for a future multi-surface design. v0.1 only accepts zero. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Coordinates")
	int32 Layer = 0;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridMapLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1.0"))
	float CellSize = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint GridDimension = FIntPoint(10, 10);

	/** Center of cell (0, 0), relative to the manager. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FVector LocalOrigin = FVector::ZeroVector;

	bool IsValid() const;
	bool IsValidCoord(const FE2GridCoord& Coord) const;
	int32 CoordToKey(const FE2GridCoord& Coord) const;
	bool KeyToCoord(int32 CellKey, FE2GridCoord& OutCoord) const;
	FVector GetCellLocalCenter(const FE2GridCoord& Coord, float LocalHeight = 0.0f) const;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridCellData
{
	GENERATED_BODY()

	/** Local ground height relative to Layout.LocalOrigin.Z. */
	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	float LocalHeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cell", meta = (Bitmask, BitmaskEnum = "/Script/E2Grid.EE2GridCellFlags"))
	int32 Flags = 0;

	/**
	 * Build-validated directed traversal to N, NE, E, SE, S, SW, W and NW.
	 * This is not a mask of merely existing adjacent cells.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	uint8 NeighborMask = 0;

	bool HasFlag(EE2GridCellFlags Flag) const;
	void SetFlag(EE2GridCellFlags Flag, bool bEnabled);
	bool CanWalkThrough() const { return HasFlag(EE2GridCellFlags::CanWalkThrough); }
	bool CanStandOn() const { return HasFlag(EE2GridCellFlags::CanStandOn); }
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridPathStep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	int32 ToCellKey = INVALID_GRID_KEY;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridPathResult
{
	GENERATED_BODY()

	/** Legacy path-only status. QueryStatus is authoritative for new callers. */
	UPROPERTY(BlueprintReadOnly, Category = "Path")
	EE2GridPathStatus Status = EE2GridPathStatus::NoPath;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	EE2GridQueryStatus QueryStatus = EE2GridQueryStatus::NoPath;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	int32 StartCellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	int32 GoalCellKey = INVALID_GRID_KEY;

	/** Runtime state snapshot used to produce this path. */
	UPROPERTY(BlueprintReadOnly, Category = "Path")
	int64 RuntimeRevision = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	TArray<FE2GridPathStep> Steps;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	float TotalCost = 0.0f;

	void Reset(EE2GridPathStatus InStatus = EE2GridPathStatus::NoPath);
	void Reset(EE2GridQueryStatus InStatus);
	void SetQueryStatus(EE2GridQueryStatus InStatus);
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridPlacementResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	EE2GridQueryStatus Status = EE2GridQueryStatus::InvalidRequest;

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	int32 CellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Placement")
	int64 RuntimeRevision = 0;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridMoveCommitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EE2GridQueryStatus Status = EE2GridQueryStatus::InvalidRequest;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	int32 FromCellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	int32 ToCellKey = INVALID_GRID_KEY;

	/** Current runtime revision after success, or at the time of failure. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	int64 RuntimeRevision = 0;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridReachableCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	int32 CellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	float Cost = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	int32 ParentCellKey = INVALID_GRID_KEY;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridReachableResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	EE2GridQueryStatus Status = EE2GridQueryStatus::InvalidRequest;

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	int32 StartCellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	float Budget = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	int64 RuntimeRevision = 0;

	/** Legal standing destinations, sorted by Cost then CellKey. Includes the start cell. */
	UPROPERTY(BlueprintReadOnly, Category = "Reachable")
	TArray<FE2GridReachableCell> Cells;

	void Reset(EE2GridQueryStatus InStatus = EE2GridQueryStatus::InvalidRequest)
	{
		Status = InStatus;
		StartCellKey = INVALID_GRID_KEY;
		Budget = 0.0f;
		RuntimeRevision = 0;
		Cells.Reset();
		TraversalTree.Reset();
	}

private:
	/** Includes traversal-only cells so every ParentCellKey chain can be reconstructed. */
	UPROPERTY()
	TArray<FE2GridReachableCell> TraversalTree;

	friend class FE2GridPathFinding;
	friend class UE2GridSubsystem;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridRangeCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 CellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	float Distance = 0.0f;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridRangeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	EE2GridQueryStatus Status = EE2GridQueryStatus::InvalidRequest;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 StartCellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	float MaxRange = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	EE2GridRangeMetric Metric = EE2GridRangeMetric::StepCount;

	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int64 RuntimeRevision = 0;

	/** Cells sorted by Distance then CellKey. Includes the start cell. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	TArray<FE2GridRangeCell> Cells;

	void Reset(EE2GridQueryStatus InStatus = EE2GridQueryStatus::InvalidRequest)
	{
		Status = InStatus;
		StartCellKey = INVALID_GRID_KEY;
		MaxRange = 0.0f;
		Metric = EE2GridRangeMetric::StepCount;
		RuntimeRevision = 0;
		Cells.Reset();
	}
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridStateDelta
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EE2GridStateChangeKind ChangeKind = EE2GridStateChangeKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<UE2GridUnitComponent> Unit = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 FromCellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 ToCellKey = INVALID_GRID_KEY;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int64 RuntimeRevision = 0;
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridBuildSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Agent", meta = (ClampMin = "0.0"))
	float AgentRadius = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Agent", meta = (ClampMin = "1.0"))
	float AgentHeight = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Agent", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float MaxSlope = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Agent", meta = (ClampMin = "0.0"))
	float MaxStepHeight = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Collision")
	TEnumAsByte<ECollisionChannel> GroundChannel = ECC_WorldStatic;

	UPROPERTY(EditAnywhere, Category = "Collision")
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_WorldStatic;

	UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "0.0"))
	float TraceStartHeight = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "1.0"))
	float TraceDepth = 500.0f;

	bool IsValid() const;
};

namespace E2GridDirections
{
	constexpr int32 Count = 8;

	E2GRID_API const FIntPoint& GetOffset(int32 DirectionIndex);
	E2GRID_API bool IsDiagonal(int32 DirectionIndex);
	E2GRID_API int32 GetOpposite(int32 DirectionIndex);
}
