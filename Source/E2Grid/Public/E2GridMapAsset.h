// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "E2GridRuntimeData.h"
#include "E2GridMapAsset.generated.h"

/**
 * 
 */
UCLASS()
class E2GRID_API UE2GridMapAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadonly)
	TMap<int32, TObjectPtr<UE2GridRuntimeData>> GridDataMap; 
};
