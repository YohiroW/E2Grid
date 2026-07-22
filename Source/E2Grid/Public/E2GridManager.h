// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "E2GridRuntimeData.h"
#include "E2GridMapAsset.h"
#include "E2GridManager.generated.h"

class UE2GridVisualizeComponent;

// ---------------------------------------------------------

UCLASS()
class E2GRID_API AE2GridManager : public AActor
{
	GENERATED_BODY()

public:
	AE2GridManager();
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	void Generate();
	
	UFUNCTION(BlueprintCallable)
	void Clear();
	
	UFUNCTION(BlueprintCallable)
	bool IsValidGridKey(const int32 InGridKey) const;
	
	UFUNCTION(BlueprintCallable)
	bool IsValidGridCoord(const FE2GridCoord& InCoord) const;
	
	UFUNCTION(BlueprintCallable)
	bool IsGridMapEmpty();
	
	UFUNCTION(BlueprintCallable)
	const UE2GridRuntimeData* GetGridData(const int32 InGridKey);
	
	// TODO: Move to grid coordinates utils
	UFUNCTION(BlueprintCallable)
	FE2GridCoord GetCoord(const int32 InGridKey);
	
	// UFUNCTION(BlueprintCallable)
	// FE2GridCoord GetCoordByWorldPosition(const FVector& InWorldPos);
	//
	// UFUNCTION(BlueprintCallable)
	// FE2GridCoord GetCoordByScreenPosition(const FVector& InScreenPos);
	
	UFUNCTION(BlueprintCallable)
	FVector GetWorldPosition(const FVector& InOrigin, const FE2GridCoord& InCoord);

	/** Returns the center of a grid cell in the grid manager's local space. */
	UFUNCTION(BlueprintPure)
	FVector GetGridLocalPosition(const FE2GridCoord& InCoord) const;

	/** Returns the center of a grid cell in world space, including the actor transform. */
	UFUNCTION(BlueprintPure)
	FVector GetGridWorldPosition(const FE2GridCoord& InCoord) const;
	
	UFUNCTION(BlueprintCallable)
	bool GetGridCoord(const FVector& InWorldPos, FE2GridCoord& OutCoord);
	
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable)
	void DrawGridMap(bool bClearOnly = false);
#endif

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// TODO: change grid map to independent data assets.
	void ForEachGridData(TFunctionRef<bool(TObjectPtr<UE2GridRuntimeData> InGridData, const FVector& InWorldPosition)> InFunc);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif
	
// ---------------------------------------------------------
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UE2GridRuntimeData> GridDataClass;
	
	// Indicate grid dimension (width, height)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FIntPoint GridDimension;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 GridSize = 100;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay)
	FVector BaseOffset = FVector::ZeroVector;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay)
	bool bDrawGridMap = false;
#endif

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UE2GridMapAsset> GridDataAsset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowVisualizedGrid = false;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UE2GridVisualizeComponent> GridVisualizeComponent; 
	
private:
	TMap<int32, TObjectPtr<UE2GridRuntimeData>> GridMap;
};
