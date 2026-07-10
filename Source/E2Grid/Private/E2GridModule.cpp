// Copyright Epic Games, Inc. All Rights Reserved.

#include "E2GridModule.h"

#define LOCTEXT_NAMESPACE "FE2GridModule"

void FE2GridModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FE2GridModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FE2GridModule, E2Grid)