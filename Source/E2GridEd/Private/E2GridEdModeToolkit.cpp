#include "E2GridEdModeToolkit.h"
#include "E2GridEdModeCommands.h"
#include "E2GridEdMode.h"
#include "EditorModeManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SE2GridBakeView.h"
#include "Widgets/SE2GridDebugView.h"
#include "Widgets/SE2GridManagerView.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

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
	TSharedRef<FUICommandList> CommandList = GetToolkitCommands();
	const FE2GridEdModeCommands& Commands = FE2GridEdModeCommands::Get();
	CommandList->MapAction(
		Commands.NewTool,
		FUIAction(
			FExecuteAction::CreateSP(this, &FE2GridEdModeToolkit::OnChangeTool, EE2GridEdModeTool::New),
			FCanExecuteAction::CreateSP(this, &FE2GridEdModeToolkit::IsToolEnabled, EE2GridEdModeTool::New),
			FIsActionChecked::CreateSP(this, &FE2GridEdModeToolkit::IsToolActive, EE2GridEdModeTool::New)));
	CommandList->MapAction(
		Commands.EditTool,
		FUIAction(
			FExecuteAction::CreateSP(this, &FE2GridEdModeToolkit::OnChangeTool, EE2GridEdModeTool::Edit),
			FCanExecuteAction::CreateSP(this, &FE2GridEdModeToolkit::IsToolEnabled, EE2GridEdModeTool::Edit),
			FIsActionChecked::CreateSP(this, &FE2GridEdModeToolkit::IsToolActive, EE2GridEdModeTool::Edit)));

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

void FE2GridEdModeToolkit::BuildToolPalette(FName PaletteName, FToolBarBuilder& ToolbarBuilder)
{
	if (PaletteName == E2GridEdModePalettes::Grid)
	{
		const FE2GridEdModeCommands& Commands = FE2GridEdModeCommands::Get();
		ToolbarBuilder.AddToolBarButton(
			Commands.NewTool,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.PlusCircle"));

		ToolbarBuilder.AddToolBarButton(
			Commands.EditTool,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"));
	}
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
	if (GetCurrentPalette() == E2GridEdModePalettes::Grid)
	{
		if (const UE2GridEdMode* E2GridEdMode = Cast<UE2GridEdMode>(GetScriptableEditorMode().Get()))
		{
			return E2GridEdMode->IsCreatingGridManager()
				? LOCTEXT("ActiveTool.New", "New")
				: LOCTEXT("ActiveTool.Edit", "Edit");
		}
	}
	return GetToolPaletteDisplayName(GetCurrentPalette());
}

FText FE2GridEdModeToolkit::GetActiveToolMessage() const
{
	return FModeToolkit::GetActiveToolMessage();
}

void FE2GridEdModeToolkit::OnChangeTool(EE2GridEdModeTool InTool)
{
	if (UE2GridEdMode* E2GridEdMode = Cast<UE2GridEdMode>(GetScriptableEditorMode().Get()))
	{
		E2GridEdMode->SetActiveTool(InTool);
	}
}

bool FE2GridEdModeToolkit::IsToolEnabled(EE2GridEdModeTool InTool) const
{
	const UE2GridEdMode* E2GridEdMode = Cast<UE2GridEdMode>(GetScriptableEditorMode().Get());
	return E2GridEdMode && E2GridEdMode->CanActivateTool(InTool);
}

bool FE2GridEdModeToolkit::IsToolActive(EE2GridEdModeTool InTool) const
{
	const UE2GridEdMode* E2GridEdMode = Cast<UE2GridEdMode>(GetScriptableEditorMode().Get());
	return E2GridEdMode && E2GridEdMode->IsToolActive(InTool);
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
