#include "SE2GridEdModePages.h"

#include "E2GridEdMode.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridEdModePages"

namespace
{
	TSharedRef<SWidget> MakePlaceholderPage(const FText& PageName)
	{
		return SNew(SBorder)
			.Padding(12.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("NotImplemented", "{0} is not implemented yet."), PageName))
			];
	}
}

void SE2GridCreatePanel::Construct(const FArguments& InArgs)
{
	EditorMode = InArgs._EditorMode;
	UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	check(E2GridEdMode);

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(E2GridEdMode->GetCreateSettings());
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SE2GridCreatePanel::OnFinishedChangingProperties);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				DetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("CreateGridManager", "Create Grid Manager"))
				.IsEnabled(this, &SE2GridCreatePanel::CanCreate)
				.OnClicked(this, &SE2GridCreatePanel::OnCreateClicked)
			]
		]
	];
}

void SE2GridCreatePanel::RefreshSettings()
{
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
}

void SE2GridCreatePanel::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->NotifyCreateSettingsChanged(false);
	}
}

FReply SE2GridCreatePanel::OnCreateClicked()
{
	if (UE2GridEdMode* E2GridEdMode = EditorMode.Get())
	{
		E2GridEdMode->CreateGridManager();
	}

	return FReply::Handled();
}

bool SE2GridCreatePanel::CanCreate() const
{
	const UE2GridEdMode* E2GridEdMode = EditorMode.Get();
	return E2GridEdMode && E2GridEdMode->CanCreateGridManager();
}

void SE2GridEditPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		MakePlaceholderPage(LOCTEXT("EditPage", "Edit"))
	];
}

void SE2GridBakePanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		MakePlaceholderPage(LOCTEXT("BakePage", "Bake"))
	];
}

void SE2GridDebugPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		MakePlaceholderPage(LOCTEXT("DebugPage", "Debug"))
	];
}

#undef LOCTEXT_NAMESPACE
