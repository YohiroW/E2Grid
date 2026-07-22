// Fill out your copyright notice in the Description page of Project Settings.


#include "E2GridUtils.h"

#include "E2GridManager.h"

bool UE2GridUtils::MouseToWorld(APlayerController* InPC, FVector& OutWorldLocation, FVector& OutWorldDirection)
{
	ensure(InPC);
	return InPC->DeprojectMousePositionToWorld(OutWorldLocation, OutWorldDirection);
}

FE2GridCoord UE2GridUtils::WorldToGrid(AE2GridManager* InGridManager, const FVector& InWorldPos)
{
	ensure(InGridManager);
	
	FE2GridCoord OutGridCoord;
	if(InGridManager->GetGridCoord(InWorldPos, OutGridCoord))
	{
		return OutGridCoord;
	}
	
	return FE2GridCoord::INVALID_COORD;
}

