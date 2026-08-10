// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "E2GridSettings.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI, defaultconfig, config=game)
class UE2GridSettings: public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
	
public:
	// -------------- Builder ------------------
	UPROPERTY(EditAnywhere, Config, Category = "Builder")
	float DefaultAgentHeight = 10.0f;
	
	// -------------- Preview ------------------
	UPROPERTY(EditAnywhere, Config, Category = "Preview")
	FLinearColor PreviewColor = FLinearColor::Green;
	
	UPROPERTY(EditAnywhere, Config, Category = "Preview")
	FLinearColor HoverColor = FLinearColor(1.00f, 0.05f, 0.55f);
	
	UPROPERTY(EditAnywhere, Config, Category = "Preview")
	FLinearColor SelectedColor = FLinearColor(0.00f, 0.45f, 1.00f);
};
