#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "E2GridRuntimeData.generated.h"

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

UENUM(BlueprintType)
enum class EE2GridRegistrationStatus : uint8
{
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
	FE2GridCoord(int32 InX, int32 InY, int32 InLayer = 0) : X(InX), Y(InY), Layer(InLayer) {}

	static const FE2GridCoord INVALID_COORD;
	bool operator==(const FE2GridCoord& Other) const { return X == Other.X && Y == Other.Y && Layer == Other.Layer; }
	bool operator!=(const FE2GridCoord& Other) const { return !(*this == Other); }

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Coordinates")
	int32 X = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Coordinates")
	int32 Y = 0;

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

	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	float LocalHeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Cell", meta = (Bitmask, BitmaskEnum = "/Script/E2Grid.EE2GridCellFlags"))
	int32 Flags = 0;

	/** N, NE, E, SE, S, SW, W, NW. */
	UPROPERTY(BlueprintReadOnly, Category = "Cell")
	uint8 NeighborMask = 0;

	bool HasFlag(EE2GridCellFlags Flag) const;
	void SetFlag(EE2GridCellFlags Flag, bool bEnabled);
	bool CanWalkThrough() const { return HasFlag(EE2GridCellFlags::CanWalkThrough); }
	bool CanStandOn() const { return HasFlag(EE2GridCellFlags::CanStandOn); }
};

USTRUCT(BlueprintType)
struct E2GRID_API FE2GridPathResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	EE2GridPathStatus Status = EE2GridPathStatus::NoPath;

	/** Ordered movement cells; the unit's current cell is omitted. */
	UPROPERTY(BlueprintReadOnly, Category = "Path")
	TArray<int32> CellKeys;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	float TotalCost = 0.0f;

	void Reset(EE2GridPathStatus InStatus = EE2GridPathStatus::NoPath)
	{
		Status = InStatus;
		CellKeys.Reset();
		TotalCost = 0.0f;
	}
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
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_WorldDynamic;

	UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "0.0"))
	float TraceStartHeight = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "1.0"))
	float TraceDepth = 500.0f;

	bool IsValid() const;
};

namespace E2GridDirections
{
	E2GRID_API const FIntPoint& GetOffset(int32 DirectionIndex);
	E2GRID_API bool IsDiagonal(int32 DirectionIndex);
	constexpr int32 Count = 8;
}
