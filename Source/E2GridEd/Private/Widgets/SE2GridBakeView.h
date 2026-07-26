#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SE2GridBakeView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridBakeView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
