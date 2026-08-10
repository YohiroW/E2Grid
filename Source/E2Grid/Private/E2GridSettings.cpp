#include "E2GridSettings.h"

UE2GridSettings::UE2GridSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UE2GridSettings::GetCategoryName() const
{
	return TEXT("E2");
}

FName UE2GridSettings::GetSectionName() const
{
	return TEXT("E2Grid");
}
