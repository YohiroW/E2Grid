#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SE2GridDebugView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridDebugView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
