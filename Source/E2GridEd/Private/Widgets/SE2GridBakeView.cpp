#include "Widgets/SE2GridBakeView.h"

#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SE2GridBakeView"

void SE2GridBakeView::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.0f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NotImplemented", "Bake is not implemented yet."))
		]
	];
}

#undef LOCTEXT_NAMESPACE
