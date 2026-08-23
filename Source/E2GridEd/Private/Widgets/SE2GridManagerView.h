#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "E2GridManager.h"

class IDetailsView;
class UE2GridEdMode;
class AE2GridManager;
struct FPropertyChangedEvent;

struct FE2GridManagerListItem
{
	explicit FE2GridManagerListItem(AE2GridManager* InGridManager = nullptr)
		: GridManager(InGridManager)
	{
	}

	TWeakObjectPtr<AE2GridManager> GridManager;
};

class SE2GridManagerView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridManagerView) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UE2GridEdMode>, EditorMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshSettings();

private:
	void RebuildManagerOptions();
	void SyncSelectedManager();
	void OnManagerComboOpening();
	void OnManagerSelectionChanged(TSharedPtr<FE2GridManagerListItem> InItem, ESelectInfo::Type InSelectInfo);
	TSharedRef<SWidget> GenerateManagerOptionWidget(TSharedPtr<FE2GridManagerListItem> InItem) const;
	FText GetSelectedManagerText() const;
	int32 GetCommitButtonIndex() const;
	int32 GetGridDetailsIndex() const;
	EVisibility GetManagerSelectorVisibility() const;
	EVisibility GetGridDetailsVisibility() const;
	EVisibility GetRevertVisibility() const;
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	FReply OnRefreshClicked();
	FReply OnCommitClicked();
	FReply OnRevertClicked();
	bool CanCommit() const;
	bool CanRevert() const;

	TWeakObjectPtr<UE2GridEdMode> EditorMode;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<IDetailsView> GridDetailsView;
	TSharedPtr<SComboBox<TSharedPtr<FE2GridManagerListItem>>> ManagerComboBox;
	TArray<TSharedPtr<FE2GridManagerListItem>> ManagerOptions;
	bool bUpdatingSelection = false;
};
