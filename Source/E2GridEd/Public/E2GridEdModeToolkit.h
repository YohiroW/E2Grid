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

	// /** Mode Toolbar Palettes **/
	// virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const;
	// virtual FText GetToolPaletteDisplayName(FName PaletteName) const; 
	// virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder);
	// virtual void OnToolPaletteChanged(FName PaletteName) override;

	/** Modes Panel Header Information **/
	virtual FText GetActiveToolDisplayName() const;
	virtual FText GetActiveToolMessage() const;

protected:
	// void OnChangeMode(FName ModeName);
	// bool IsModeEnabled(FName ModeName) const;
	// bool IsModeActive(FName ModeName) const;
	//
	// void OnChangeTool(FName ToolName);
	// bool IsToolEnabled(FName ToolName) const;
	// bool IsToolActive(FName ToolName) const;

	/** FModeToolkit interface */
	virtual void RequestModeUITabs() override;
	virtual void InvokeUI() override;

	// TSharedRef<SDockTab> CreateInspectedObjectsDetailsViewTab(const FSpawnTabArgs& Args);

private:
	TSharedPtr<SWidget> InlineContent;
	
	TWeakPtr<SDockTab> InspectedObjectsTab;
	FMinorTabConfig InspectedObjectsTabInfo;

	const static TArray<FName> PaletteNames;
};



