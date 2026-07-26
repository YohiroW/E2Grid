#include "E2GridEdModeToolkit.h"
#include "E2GridEdMode.h"
#include "EditorModeManager.h"
#include "Widgets/SE2GridBakeView.h"
#include "Widgets/SE2GridDebugView.h"
#include "Widgets/SE2GridManagerView.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SNullWidget.h"

#define LOCTEXT_NAMESPACE "E2GridEdModeToolkit"

namespace E2GridEdModePalettes
{
	const FName Grid(TEXT("Grid"));
	const FName Bake(TEXT("Bake"));
	const FName Debug(TEXT("Debug"));
}

const TArray<FName> FE2GridEdModeToolkit::PaletteNames =
{
	E2GridEdModePalettes::Grid,
	E2GridEdModePalettes::Bake,
	E2GridEdModePalettes::Debug
};

void FE2GridEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	UE2GridEdMode* E2GridEdMode = CastChecked<UE2GridEdMode>(InOwningMode.Get());

	InlineContent = SAssignNew(PageSwitcher, SWidgetSwitcher)
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(GridView, SE2GridManagerView)
			.EditorMode(E2GridEdMode)
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SE2GridBakeView)
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SE2GridDebugView)
		];

	FModeToolkit::Init(InitToolkitHost, InOwningMode);
	SetCurrentPalette(E2GridEdModePalettes::Grid);
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

void FE2GridEdModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteName) const
{
	InPaletteName = PaletteNames;
}

FText FE2GridEdModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const
{
	if (PaletteName == E2GridEdModePalettes::Grid)
	{
		return LOCTEXT("Palette.Grid", "Grid");
	}
	if (PaletteName == E2GridEdModePalettes::Bake)
	{
		return LOCTEXT("Palette.Bake", "Bake");
	}
	if (PaletteName == E2GridEdModePalettes::Debug)
	{
		return LOCTEXT("Palette.Debug", "Debug");
	}

	return FText::GetEmpty();
}

TSharedRef<SWidget> FE2GridEdModeToolkit::CreatePaletteWidget(
	TSharedPtr<FUICommandList> InCommandList,
	FName InToolbarCustomizationName,
	FName InPaletteName)
{
	return SNullWidget::NullWidget;
}

void FE2GridEdModeToolkit::OnToolPaletteChanged(FName PaletteName)
{
	const int32 PageIndex = PaletteNames.IndexOfByKey(PaletteName);
	if (PageIndex == INDEX_NONE)
	{
		return;
	}

	if (PageSwitcher.IsValid())
	{
		PageSwitcher->SetActiveWidgetIndex(PageIndex);
	}

	if (UE2GridEdMode* E2GridEdMode = Cast<UE2GridEdMode>(GetScriptableEditorMode().Get()))
	{
		E2GridEdMode->SetActivePage(static_cast<EE2GridEdModePage>(PageIndex));
	}
}

void FE2GridEdModeToolkit::RefreshSettings()
{
	if (GridView.IsValid())
	{
		GridView->RefreshSettings();
	}
}

FText FE2GridEdModeToolkit::GetActiveToolDisplayName() const
{
	return GetToolPaletteDisplayName(GetCurrentPalette());
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
