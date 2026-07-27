#include "E2GridEdModeGridSettings.h"

void UE2GridEdModeGridSettings::Reset()
{
	RuntimeData = FE2GridRuntimeData();
}

void UE2GridEdModeGridSettings::LoadFrom(const FE2GridRuntimeData& InGridData)
{
	RuntimeData = InGridData;
}
