#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UE2GridEdMode;
struct FAssetData;

class SE2GridBakeView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridBakeView) {}
		SLATE_ARGUMENT(UE2GridEdMode*, EditorMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString GetAssetPath() const;
	void OnAssetChanged(const FAssetData& AssetData);
	FReply OnBuildClicked();
	bool CanBuild() const;
	FText GetStatusText() const;

	TWeakObjectPtr<UE2GridEdMode> EditorMode;
};
