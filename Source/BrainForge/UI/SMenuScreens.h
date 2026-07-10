#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "SBrainEditor.h" // FOnForgeNavigate

class UForgeGameInstance;
class SEditableTextBox;
class SVerticalBox;

/** Main menu: Continue Evolution / New Brain / Load Experiment / Research Archive / Settings / Credits / Exit. */
class SMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainMenu) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, UForgeGameInstance* InGI);

private:
	UForgeGameInstance* GI = nullptr;
	FOnForgeNavigate OnNavigate;
};

/** New Brain: lineage name + seed entry. */
class SNewBrainScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNewBrainScreen) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, UForgeGameInstance* InGI);

private:
	UForgeGameInstance* GI = nullptr;
	FOnForgeNavigate OnNavigate;
	TSharedPtr<SEditableTextBox> NameBox;
	TSharedPtr<SEditableTextBox> SeedBox;
};

/** Load Experiment: saved lineages with load / branch / delete. */
class SLoadScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadScreen) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, UForgeGameInstance* InGI);

private:
	UForgeGameInstance* GI = nullptr;
	FOnForgeNavigate OnNavigate;
	TSharedPtr<SVerticalBox> SlotList;
	void RefreshSlots();
};

/** Research Archive: functions, heuristics, fitness contributors, trial history, log. */
class SResearchArchive : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SResearchArchive) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, UForgeGameInstance* InGI);

private:
	UForgeGameInstance* GI = nullptr;
	FOnForgeNavigate OnNavigate;
};

/** Settings: audio / display / gameplay. */
class SSettingsScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsScreen) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, UForgeGameInstance* InGI);

private:
	UForgeGameInstance* GI = nullptr;
	FOnForgeNavigate OnNavigate;
	void ApplyDisplay();
};

/** Credits. */
class SCreditsScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreditsScreen) {}
		SLATE_EVENT(FOnForgeNavigate, OnNavigate)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);

private:
	FOnForgeNavigate OnNavigate;
};
