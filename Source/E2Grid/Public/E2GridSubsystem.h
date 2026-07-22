// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "E2GridSubsystem.generated.h"

class UE2GridMapAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGridMapLoaded);

UCLASS()
class E2GRID_API UE2GridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UE2GridSubsystem();
	
	// UWorldSubsystem interfaces
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize();
	virtual void PreDeinitialize();
	
	// --------------------------------------------------------------------------
	
	UFUNCTION(BlueprintCallable)
	bool LoadGridMap(UE2GridMapAsset* InGridMapAsset);
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnGridMapLoaded OnGridMapLoaded;
};
