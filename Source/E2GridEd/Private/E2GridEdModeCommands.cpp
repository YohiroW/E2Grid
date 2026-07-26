#include "E2GridEdModeCommands.h"

#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "E2GridEdModeCommands"

FE2GridEdModeCommands::FE2GridEdModeCommands()
	: TCommands<FE2GridEdModeCommands>(
		TEXT("E2GridEdMode"),
		LOCTEXT("ContextDescription", "E2 Grid Editor Mode"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FE2GridEdModeCommands::RegisterCommands()
{
	UI_COMMAND(
		NewTool,
		"New",
		"Create a new grid manager",
		EUserInterfaceActionType::RadioButton,
		FInputChord());
	UI_COMMAND(
		EditTool,
		"Edit",
		"Edit an existing grid manager",
		EUserInterfaceActionType::RadioButton,
		FInputChord());
}

#undef LOCTEXT_NAMESPACE
