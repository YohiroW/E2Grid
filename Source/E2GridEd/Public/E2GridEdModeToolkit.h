#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "E2GridEdMode.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/BaseToolkit.h"
#include "Framework/SlateDelegates.h"

class FE2GridEdModeToolkit : public FModeToolkit
{
public:
	/** Initializes the landscape mode toolkit */
	virtual void Init(const TSharedPtr<class IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	/** Mode Toolbar Palettes **/
	virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const override;
	virtual FText GetToolPaletteDisplayName(FName PaletteName) const override;
	virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder) override;
	virtual void OnToolPaletteChanged(FName PaletteName) override;

	void RefreshSettings();

	/** Modes Panel Header Information **/
	virtual FText GetActiveToolDisplayName() const;
	virtual FText GetActiveToolMessage() const;

protected:
	void OnChangeTool(EE2GridEdModeTool InTool);
	bool IsToolEnabled(EE2GridEdModeTool InTool) const;
	bool IsToolActive(EE2GridEdModeTool InTool) const;

	/** FModeToolkit interface */
	virtual void RequestModeUITabs() override;
	virtual void InvokeUI() override;

	// TSharedRef<SDockTab> CreateInspectedObjectsDetailsViewTab(const FSpawnTabArgs& Args);

private:
	TSharedPtr<SWidget> InlineContent;
	TSharedPtr<class SWidgetSwitcher> PageSwitcher;
	TSharedPtr<class SE2GridManagerView> GridView;
	
	TWeakPtr<SDockTab> InspectedObjectsTab;
	FMinorTabConfig InspectedObjectsTabInfo;

	const static TArray<FName> PaletteNames;
};



