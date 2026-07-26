#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FE2GridEdModeCommands : public TCommands<FE2GridEdModeCommands>
{
public:
	FE2GridEdModeCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> NewTool;
	TSharedPtr<FUICommandInfo> EditTool;
};
