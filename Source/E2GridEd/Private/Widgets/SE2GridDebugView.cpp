#include "Widgets/SE2GridDebugView.h"

#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridDebugView"

void SE2GridDebugView::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NotImplemented", "Debug is not implemented yet."))
		]
	];
}

#undef LOCTEXT_NAMESPACE
