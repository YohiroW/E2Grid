#include "Widgets/SE2GridBakeView.h"

#include "E2GridEdMode.h"
#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridBakeView"

void SE2GridBakeView::Construct(const FArguments& InArgs)
{
	EditorMode = InArgs._EditorMode;
	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TargetAsset", "Target Grid Map Asset"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 12.0f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UE2GridMapAsset::StaticClass())
				.ObjectPath(this, &SE2GridBakeView::GetAssetPath)
				.OnObjectChanged(this, &SE2GridBakeView::OnAssetChanged)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(LOCTEXT(
					"BuildHint",
					"Layout comes from the Grid page. Agent and collision settings are stored on the selected manager. Build samples world collision and only replaces the asset after validation succeeds."))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("BuildButton", "Build Grid Map Asset"))
				.IsEnabled(this, &SE2GridBakeView::CanBuild)
				.OnClicked(this, &SE2GridBakeView::OnBuildClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(this, &SE2GridBakeView::GetStatusText)
			]
		]
	];
}

FString SE2GridBakeView::GetAssetPath() const
{
	if (const UE2GridEdMode* Mode = EditorMode.Get())
	{
		if (const AE2GridManager* Manager = Mode->GetActiveGridManager())
		{
			return Manager->GridMapAsset ? Manager->GridMapAsset->GetPathName() : FString();
		}
	}
	return FString();
}

void SE2GridBakeView::OnAssetChanged(const FAssetData& AssetData)
{
	if (UE2GridEdMode* Mode = EditorMode.Get())
	{
		if (AE2GridManager* Manager = Mode->GetActiveGridManager())
		{
			Manager->Modify();
			Manager->GridMapAsset = Cast<UE2GridMapAsset>(AssetData.GetAsset());
			Manager->MarkPackageDirty();
		}
	}
}

FReply SE2GridBakeView::OnBuildClicked()
{
	if (UE2GridEdMode* Mode = EditorMode.Get())
	{
		Mode->BuildActiveGrid();
	}
	return FReply::Handled();
}

bool SE2GridBakeView::CanBuild() const
{
	const UE2GridEdMode* Mode = EditorMode.Get();
	return Mode && Mode->CanBuildActiveGrid();
}

FText SE2GridBakeView::GetStatusText() const
{
	const UE2GridEdMode* Mode = EditorMode.Get();
	return FText::FromString(Mode ? Mode->GetLastBuildSummary() : FString());
}

#undef LOCTEXT_NAMESPACE
