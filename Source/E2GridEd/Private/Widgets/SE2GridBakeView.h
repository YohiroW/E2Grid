#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UE2GridEdMode;
class UE2GridMapAsset;
struct FAssetData;

class SE2GridBakeView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridBakeView) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UE2GridEdMode>, EditorMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString GetTargetAssetPath() const;
	void OnTargetAssetChanged(const FAssetData& AssetData);
	FReply OnBuildClicked();
	bool CanBuild() const;
	FText GetResultText() const;

	TWeakObjectPtr<UE2GridEdMode> EditorMode;
	TWeakObjectPtr<UE2GridMapAsset> TargetAsset;
	FText LastResult;
};
