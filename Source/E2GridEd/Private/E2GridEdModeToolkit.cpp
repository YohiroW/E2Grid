#include "E2GridEdModeToolkit.h"
#include "E2GridEdMode.h"
#include "EditorModeManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "E2GridEdModeToolkit"

void FE2GridEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	InlineContent =
		SNew(SBorder)
		.Padding(8)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("InitialTitle", "Grid"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateGrid", "Create Grid"))
				.OnClicked_Lambda([]()
				{
					// TODO: 这里放创建 Grid Actor / Grid Asset 的逻辑
					return FReply::Handled();
				})
			]
		];

	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

FName FE2GridEdModeToolkit::GetToolkitFName() const
{
	return FName("E2GridEdMode");
}

FText FE2GridEdModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Grid");
}

FEdMode* FE2GridEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(UE2GridEdMode::EM_E2GridEdModeId);
}

TSharedPtr<SWidget> FE2GridEdModeToolkit::GetInlineContent() const
{
	return InlineContent;
}

FText FE2GridEdModeToolkit::GetActiveToolDisplayName() const
{
	return FModeToolkit::GetActiveToolDisplayName();
}

FText FE2GridEdModeToolkit::GetActiveToolMessage() const
{
	return FModeToolkit::GetActiveToolMessage();
}

void FE2GridEdModeToolkit::RequestModeUITabs()
{
	FModeToolkit::RequestModeUITabs();
}

void FE2GridEdModeToolkit::InvokeUI()
{
	FModeToolkit::InvokeUI();
}

#undef LOCTEXT_NAMESPACE
