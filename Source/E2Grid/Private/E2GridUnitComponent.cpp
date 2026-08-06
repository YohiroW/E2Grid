#include "E2GridUnitComponent.h"

#include "E2GridSubsystem.h"
#include "Engine/World.h"

UE2GridUnitComponent::UE2GridUnitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UE2GridUnitComponent::BeginPlay()
{
	Super::BeginPlay();
	RequestRegistration();
}

void UE2GridUnitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UE2GridSubsystem* GridSubsystem = World->GetSubsystem<UE2GridSubsystem>())
		{
			GridSubsystem->UnregisterUnit(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

EE2GridRegistrationStatus UE2GridUnitComponent::RequestRegistration()
{
	if (UE2GridSubsystem* GridSubsystem = GetWorld()->GetSubsystem<UE2GridSubsystem>())
	{
		return GridSubsystem->RegisterUnit(this);
	}
	return EE2GridRegistrationStatus::PendingManager;
}

void UE2GridUnitComponent::SetPlacementState(
	bool bInRegistered,
	int32 InCellKey,
	EE2GridRegistrationStatus Status)
{
	const bool bChanged = bRegistered != bInRegistered || CurrentCellKey != InCellKey;
	bRegistered = bInRegistered;
	CurrentCellKey = InCellKey;
	if (bChanged || Status != EE2GridRegistrationStatus::AlreadyRegistered)
	{
		OnRegistrationChanged.Broadcast(bRegistered, Status);
	}
}
