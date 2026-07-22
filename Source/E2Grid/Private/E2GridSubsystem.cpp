#include "E2GridSubsystem.h"

UE2GridSubsystem::UE2GridSubsystem()
{
}

bool UE2GridSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UE2GridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UE2GridSubsystem::PostInitialize()
{
	Super::PostInitialize();
	UE_LOG(LogTemp, Warning, TEXT("E2GridSubsystem::PostInitialize"));
}

void UE2GridSubsystem::PreDeinitialize()
{
	Super::PreDeinitialize();
	UE_LOG(LogTemp, Warning, TEXT("E2GridSubsystem::PreDeinitialize"));
}

bool UE2GridSubsystem::LoadGridMap(UE2GridMapAsset* InGridMapAsset)
{
	
	OnGridMapLoaded.Broadcast();
	return true;
}
