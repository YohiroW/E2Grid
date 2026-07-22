#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class UE2GridEdMode;
struct FPropertyChangedEvent;

class SE2GridCreatePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridCreatePanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UE2GridEdMode>, EditorMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshSettings();

private:
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	FReply OnCreateClicked();
	bool CanCreate() const;

	TWeakObjectPtr<UE2GridEdMode> EditorMode;
	TSharedPtr<IDetailsView> DetailsView;
};

class SE2GridEditPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridEditPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class SE2GridBakePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridBakePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};

class SE2GridDebugPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SE2GridDebugPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
