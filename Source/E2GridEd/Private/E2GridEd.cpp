// Copyright Epic Games, Inc. All Rights Reserved.

#include "E2GridEdModule.h"
#include "E2GridEdModeCommands.h"

#define LOCTEXT_NAMESPACE "FE2GridEdModule"

void FE2GridEdModule::StartupModule()
{
	FE2GridEdModeCommands::Register();
}

void FE2GridEdModule::ShutdownModule()
{
	FE2GridEdModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FE2GridEdModule, E2GridEd)
