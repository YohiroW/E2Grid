// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "E2GridRuntimeData.h"
#include "E2GridUtils.generated.h"


UCLASS()
class E2GRID_API UE2GridUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Convert current mouse position to world position
	static bool MouseToWorld(APlayerController* InPC, FVector& OutWorldLocation, FVector& OutWorldDirection);
	 
	// 
	static FE2GridCoord WorldToGrid(AE2GridManager* InGridManager, const FVector& InWorldPos);
};
