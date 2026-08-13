#include "Widgets/SE2GridBakeView.h"

#include "E2GridEdMode.h"
#include "E2GridMapAsset.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridBakeView"

void SE2GridBakeView::Construct(const FArguments& InArgs)
{
	EditorMode = InArgs._EditorMode;
	LastResult = LOCTEXT(
		"BuildHint",
		"Choose a target asset. Build samples world collision into temporary data and replaces the asset only after validation succeeds.");

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
				.Text(LOCTEXT("TargetAssetLabel", "Target Grid Map Asset"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UE2GridMapAsset::StaticClass())
				.ObjectPath(this, &SE2GridBakeView::GetTargetAssetPath)
				.OnObjectChanged(this, &SE2GridBakeView::OnTargetAssetChanged)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get(), "PrimaryButton")
				.TextStyle(&FAppStyle::Get(), "PrimaryButtonText")
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("BuildGridMap", "Build Grid Map Asset"))
				.IsEnabled(this, &SE2GridBakeView::CanBuild)
				.OnClicked(this, &SE2GridBakeView::OnBuildClicked)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(this, &SE2GridBakeView::GetResultText)
			]
		]
	];
}

FString SE2GridBakeView::GetTargetAssetPath() const
{
	return TargetAsset.IsValid() ? TargetAsset->GetPathName() : FString();
}

void SE2GridBakeView::OnTargetAssetChanged(const FAssetData& AssetData)
{
	TargetAsset = Cast<UE2GridMapAsset>(AssetData.GetAsset());
}

FReply SE2GridBakeView::OnBuildClicked()
{
	if (UE2GridEdMode* Mode = EditorMode.Get())
	{
		Mode->BuildGridMap(TargetAsset.Get(), LastResult);
	}
	return FReply::Handled();
}

bool SE2GridBakeView::CanBuild() const
{
	const UE2GridEdMode* Mode = EditorMode.Get();
	return Mode && Mode->CanBuildGridMap(TargetAsset.Get());
}

FText SE2GridBakeView::GetResultText() const
{
	return LastResult;
}

#undef LOCTEXT_NAMESPACE
