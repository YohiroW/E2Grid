#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UE2GridEdMode;

class SE2GridDebugView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridDebugView) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UE2GridEdMode>, EditorMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FText GetManagerSummary() const;
	FText GetAssetSummary() const;
	FText GetRuntimeSummary() const;

	TWeakObjectPtr<UE2GridEdMode> EditorMode;
};
