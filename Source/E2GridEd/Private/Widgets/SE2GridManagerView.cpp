#include "Widgets/SE2GridManagerView.h"

#include "E2GridEdModeGridSettings.h"
#include "E2GridEdModeSettings.h"
#include "E2GridEdMode.h"
#include "E2GridManager.h"
#include "Engine/Level.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridManagerView"

namespace
{
	TSharedRef<IDetailsView> MakeSettingsDetailsView(UObject* SettingsObject)
	{
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedRef<IDetailsView> Result = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		Result->SetObject(SettingsObject);
		return Result;
	}

	FText GetGridManagerDisplayName(const AE2GridManager* GridManager)
	{
		if (!IsValid(GridManager))
		{
			return LOCTEXT("InvalidGridManager", "Invalid Grid Manager");
		}

		const UObject* LevelOuter = GridManager->GetLevel() ? GridManager->GetLevel()->GetOuter() : nullptr;
		return LevelOuter
			? FText::Format(
				LOCTEXT("GridManagerWithLevel", "{0} — {1}"),
				FText::FromString(GridManager->GetActorLabel()),
				FText::FromString(LevelOuter->GetName()))
			: FText::FromString(GridManager->GetActorLabel());
	}

}

void SE2GridManagerView::Construct(const FArguments& InArgs)
{
	EditorMode = InArgs._EditorMode;
	UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	check(E2GridEdMode);

	DetailsView = MakeSettingsDetailsView(E2GridEdMode->GetSettings());
	GridDetailsView = MakeSettingsDetailsView(E2GridEdMode->GetGridSettings());
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SE2GridManagerView::OnFinishedChangingProperties);
	RebuildManagerOptions();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
					.Visibility(this, &SE2GridManagerView::GetManagerSelectorVisibility)
					.Text(LOCTEXT("GridManagerLabel", "Grid Manager"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				.Visibility(this, &SE2GridManagerView::GetManagerSelectorVisibility)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(ManagerComboBox, SComboBox<TSharedPtr<FE2GridManagerListItem>>)
						.OptionsSource(&ManagerOptions)
						.OnComboBoxOpening(this, &SE2GridManagerView::OnManagerComboOpening)
						.OnGenerateWidget(this, &SE2GridManagerView::GenerateManagerOptionWidget)
						.OnSelectionChanged(this, &SE2GridManagerView::OnManagerSelectionChanged)
						[
							SNew(STextBlock)
								.Text(this, &SE2GridManagerView::GetSelectedManagerText)
						]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
						.Text(LOCTEXT("RefreshGridManagers", "Refresh"))
						.OnClicked(this, &SE2GridManagerView::OnRefreshClicked)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				DetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
				.Visibility(this, &SE2GridManagerView::GetGridDetailsVisibility)
				.Padding(8.0f)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectedGridLabel", "Selected Grid"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(SWidgetSwitcher)
						.WidgetIndex(this, &SE2GridManagerView::GetGridDetailsIndex)
						+ SWidgetSwitcher::Slot()
						[
							SNew(STextBlock)
							.AutoWrapText(true)
							.Text(LOCTEXT(
								"SelectGridHint",
								"Hover and click a grid cell to inspect its runtime data."))
						]
						+ SWidgetSwitcher::Slot()
						[
							GridDetailsView.ToSharedRef()
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SButton)
						.HAlign(HAlign_Center)
						.Visibility(this, &SE2GridManagerView::GetRevertVisibility)
						.Text(LOCTEXT("RevertGridManagerSettings", "Revert"))
						.IsEnabled(this, &SE2GridManagerView::CanRevert)
						.OnClicked(this, &SE2GridManagerView::OnRevertClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SWidgetSwitcher)
					.WidgetIndex(this, &SE2GridManagerView::GetCommitButtonIndex)
					+ SWidgetSwitcher::Slot()
					[
						SNew(SButton)
							.ButtonStyle(&FAppStyle::Get(), "PrimaryButton")
							.TextStyle(&FAppStyle::Get(), "PrimaryButtonText")
							.HAlign(HAlign_Center)
							.Text(LOCTEXT("CreateGridManager", "Create"))
							.IsEnabled(this, &SE2GridManagerView::CanCommit)
							.OnClicked(this, &SE2GridManagerView::OnCommitClicked)
					]
					+ SWidgetSwitcher::Slot()
					[
						SNew(SButton)
							.HAlign(HAlign_Center)
							.Text(LOCTEXT("ApplyGridManagerSettings", "Apply"))
							.IsEnabled(this, &SE2GridManagerView::CanCommit)
							.OnClicked(this, &SE2GridManagerView::OnCommitClicked)
					]
				]
			]
		]
	];

	SyncSelectedManager();
}

void SE2GridManagerView::RefreshSettings()
{
	RebuildManagerOptions();
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
	if (GridDetailsView.IsValid())
	{
		GridDetailsView->ForceRefresh();
	}
}

void SE2GridManagerView::RebuildManagerOptions()
{
	ManagerOptions.Reset();
	if (const UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		for (const TWeakObjectPtr<AE2GridManager>& GridManager : E2GridEdMode->GetGridManagers())
		{
			if (GridManager.IsValid())
			{
				ManagerOptions.Add(MakeShared<FE2GridManagerListItem>(GridManager.Get()));
			}
		}
	}

	if (ManagerComboBox.IsValid())
	{
		ManagerComboBox->RefreshOptions();
		SyncSelectedManager();
	}
}

void SE2GridManagerView::SyncSelectedManager()
{
	if (!ManagerComboBox.IsValid())
	{
		return;
	}

	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	const AE2GridManager* ActiveGridManager = E2GridEdMode ? E2GridEdMode->GetActiveGridManager() : nullptr;
	const TSharedPtr<FE2GridManagerListItem>* SelectedItem = ManagerOptions.FindByPredicate(
		[ActiveGridManager](const TSharedPtr<FE2GridManagerListItem>& Item)
		{
			return Item.IsValid() && Item->GridManager.Get() == ActiveGridManager;
		});

	TGuardValue<bool> UpdatingSelectionGuard(bUpdatingSelection, true);
	ManagerComboBox->SetSelectedItem(SelectedItem ? *SelectedItem : nullptr);
}

void SE2GridManagerView::OnManagerComboOpening()
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->RefreshGridManagers();
	}
}

void SE2GridManagerView::OnManagerSelectionChanged(
	TSharedPtr<FE2GridManagerListItem> InItem,
	ESelectInfo::Type InSelectInfo)
{
	if (bUpdatingSelection)
	{
		return;
	}

	UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	AE2GridManager* NewGridManager = InItem.IsValid() ? InItem->GridManager.Get() : nullptr;
	if (!E2GridEdMode || E2GridEdMode->GetActiveGridManager() == NewGridManager)
	{
		return;
	}

	if (!E2GridEdMode->ResolvePendingSettings())
	{
		SyncSelectedManager();
		return;
	}

	E2GridEdMode->SetActiveGridManager(NewGridManager);
}

TSharedRef<SWidget> SE2GridManagerView::GenerateManagerOptionWidget(
	TSharedPtr<FE2GridManagerListItem> InItem) const
{
	return SNew(STextBlock)
		.Text(InItem.IsValid() && InItem->GridManager.IsValid()
			? GetGridManagerDisplayName(InItem->GridManager.Get())
			: LOCTEXT("InvalidGridManagerOption", "Invalid Grid Manager"));
}

FText SE2GridManagerView::GetSelectedManagerText() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && E2GridEdMode->GetActiveGridManager()
		? GetGridManagerDisplayName(E2GridEdMode->GetActiveGridManager())
		: LOCTEXT("NoGridManager", "No Grid Manager");
}

int32 SE2GridManagerView::GetCommitButtonIndex() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && E2GridEdMode->IsCreatingGridManager() ? 0 : 1;
}

int32 SE2GridManagerView::GetGridDetailsIndex() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && E2GridEdMode->HasSelectedGrid() ? 1 : 0;
}

EVisibility SE2GridManagerView::GetManagerSelectorVisibility() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && !E2GridEdMode->IsCreatingGridManager()
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SE2GridManagerView::GetGridDetailsVisibility() const
{
	return GetManagerSelectorVisibility();
}

EVisibility SE2GridManagerView::GetRevertVisibility() const
{
	return GetManagerSelectorVisibility();
}

void SE2GridManagerView::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->NotifySettingsChanged(false);
	}
}

FReply SE2GridManagerView::OnRefreshClicked()
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->RefreshGridManagers();
	}
	return FReply::Handled();
}

FReply SE2GridManagerView::OnCommitClicked()
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->CommitSettings();
	}
	return FReply::Handled();
}

FReply SE2GridManagerView::OnRevertClicked()
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->RevertSettings();
	}
	return FReply::Handled();
}

bool SE2GridManagerView::CanCommit() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && E2GridEdMode->CanCommitSettings();
}

bool SE2GridManagerView::CanRevert() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && !E2GridEdMode->IsCreatingGridManager() &&
		E2GridEdMode->HasPendingSettings();
}

#undef LOCTEXT_NAMESPACE
