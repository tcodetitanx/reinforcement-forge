#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Core/ForgeSession.h"

class SGraphCanvas;
class SVerticalBox;
class STextBlock;

DECLARE_DELEGATE_OneParam(FOnForgeNavigate, int32 /*EForgeScreen as int*/);

/**
 * The main gameplay screen: top status bar, reinforcement-loop panel,
 * live charts, pattern memory, selected-detail panel, bottom toolbar,
 * toasts, generation-reward and victory modals - arranged like the
 * target screenshot.
 */
class SBrainEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBrainEditor) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FForgeSession* InSession);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	/** Called by the HUD when the session emits a text event. */
	void AddToast(const FString& Text, bool bImportant);

private:
	FForgeSession* Session = nullptr;
	FOnForgeNavigate OnNavigate;
	TSharedPtr<SGraphCanvas> Canvas;

	// dynamic sub-panels rebuilt when content changes
	TSharedPtr<SVerticalBox> HeuristicsBox;
	TSharedPtr<SVerticalBox> PatternBox;
	TSharedPtr<SVerticalBox> SelectionBox;
	TSharedPtr<SVerticalBox> ToastBox;
	TSharedPtr<SVerticalBox> ModalBox;
	int32 LastHeuristicCount = -1;
	int32 LastPatternHash = -1;
	int32 LastSelectionHash = -1;
	int32 LastModalHash = -1;

	struct FToast
	{
		TSharedPtr<SWidget> Widget;
		float Life = 4.f;
	};
	TArray<FToast> Toasts;

	// builders
	TSharedRef<SWidget> BuildTopBar();
	TSharedRef<SWidget> BuildLeftPanel();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<SWidget> BuildBottomBar();
	TSharedRef<SWidget> BuildTrialStrip();
	TSharedRef<SWidget> BuildBreadcrumb();

	void RefreshHeuristics();
	void RefreshPatterns();
	void RefreshSelection();
	void RefreshModal();

	// action helpers
	void DoRunTrial();
	void SetSpeed(int32 NewSpeed);

	FText TrialLabel() const;
	int32 SelectionHash() const;
};
