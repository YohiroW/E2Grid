// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "E2GridVisualizeComponent.generated.h"


UCLASS(Blueprintable, BlueprintType)
class E2GRID_API UGridVisualDataAsset: public UDataAsset
{
	GENERATED_BODY()
	
public:
	UGridVisualDataAsset() {}
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> VisualTags;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, UMaterialInterface*> VisualMap;
};


UCLASS()
class E2GRID_API UE2GridVisualizeComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UE2GridVisualizeComponent();
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	void BuildGridInstancedMeshes();
	
	UFUNCTION(BlueprintCallable)
	void ShowGrids(); 

protected:
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	
// ------------------------------------------------------------
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UGridVisualDataAsset* VisualData;
	
protected:
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<AE2GridManager> GridOwner;

	
};
