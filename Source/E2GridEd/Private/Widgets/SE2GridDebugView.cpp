#include "Widgets/SE2GridDebugView.h"

#include "E2GridEdMode.h"
#include "E2GridManager.h"
#include "E2GridMapAsset.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridDebugView"

void SE2GridDebugView::Construct(const FArguments& InArgs)
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
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "HeadingExtraSmall")
				.Text(LOCTEXT("Title", "Built Grid Debug"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &SE2GridDebugView::GetManagerSummary)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(this, &SE2GridDebugView::GetAssetSummary)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(LOCTEXT(
					"Legend",
					"Viewport: green outlines are standable cells, yellow outlines are traversal-only cells, red outlines are blocked cells, and blue lines show built traversal."))
			]
		]
	];
}

FText SE2GridDebugView::GetManagerSummary() const
{
	const AE2GridManager* Manager = EditorMode.IsValid() ? EditorMode->GetActiveGridManager() : nullptr;
	return Manager
		? FText::Format(LOCTEXT("ManagerSummary", "Manager: {0}"), FText::FromString(Manager->GetActorLabel()))
		: LOCTEXT("NoManager", "Manager: none selected");
}

FText SE2GridDebugView::GetAssetSummary() const
{
	const AE2GridManager* Manager = EditorMode.IsValid() ? EditorMode->GetActiveGridManager() : nullptr;
	if (!Manager || !Manager->GridMapAsset)
	{
		return LOCTEXT("NoAsset", "Asset: none\nSelect a Manager and Build a target Map Asset first.");
	}

	const UE2GridMapAsset* Asset = Manager->GridMapAsset;
	const FE2GridMapLayout& Layout = Asset->GetLayout();
	FString ValidationError;
	const bool bValid = Asset->Validate(&ValidationError);
	return FText::Format(
		LOCTEXT(
			"AssetSummary",
			"Asset: {0}\nBuild Version: {1} (current: {2})\nLayout: {3} x {4}, {5} cm\nSparse Cells: {6}\nValidation: {7}"),
		FText::FromString(Asset->GetPathName()),
		FText::AsNumber(Asset->GetBuildVersion()),
		FText::AsNumber(E2GRID_MAP_BUILD_VERSION),
		FText::AsNumber(Layout.GridDimension.X),
		FText::AsNumber(Layout.GridDimension.Y),
		FText::AsNumber(Layout.CellSize),
		FText::AsNumber(Asset->GetCells().Num()),
		bValid ? LOCTEXT("Valid", "Valid") : FText::FromString(ValidationError));
}

#undef LOCTEXT_NAMESPACE
