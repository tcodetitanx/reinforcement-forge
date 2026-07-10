#include "SMenuScreens.h"
#include "SNeonWidgets.h"
#include "Core/ForgeStyle.h"
#include "Core/SaveSystem.h"
#include "Game/ForgeGameInstance.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Misc/DateTime.h"
#include "GenericPlatform/GenericPlatformMisc.h"

#define LOCTEXT_NAMESPACE "Forge"

namespace
{
	TSharedRef<SWidget> ScreenFrame(TSharedRef<SWidget> Content)
	{
		const FForgeStyle& Style = FForgeStyle::Get();
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(&Style.WhiteBrush).ColorAndOpacity(FSlateColor(FForgeStyle::Background()))
			]
			+ SOverlay::Slot()
			[
				Content
			];
	}

	TSharedRef<SWidget> MenuButton(const FText& Label, FLinearColor Color, TFunction<void()> Fn, bool bFilled = false)
	{
		return SNew(SBox).WidthOverride(320).HeightOverride(46).Padding(FMargin(0, 5))
		[
			SNew(SNeonButton)
			.Text(Label)
			.FontSize(13)
			.Color(Color)
			.bFilled(bFilled)
			.OnClicked(FSimpleDelegate::CreateLambda([Fn]() { Fn(); }))
		];
	}

	TSharedRef<SWidget> BackRow(const FOnForgeNavigate& Nav, EForgeScreen Target = EForgeScreen::MainMenu)
	{
		FOnForgeNavigate NavCopy = Nav;
		return SNew(SBox).WidthOverride(200).Padding(FMargin(0, 20, 0, 0))
		[
			SNew(SNeonButton)
			.Text(LOCTEXT("Back", "<  BACK"))
			.FontSize(11)
			.Color(FForgeStyle::TextDim())
			.OnClicked(FSimpleDelegate::CreateLambda([NavCopy, Target]() { NavCopy.ExecuteIfBound((int32)Target); }))
		];
	}
}

// ============================================================== main menu

void SMainMenu::Construct(const FArguments& InArgs, UForgeGameInstance* InGI)
{
	GI = InGI;
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	const bool bHasSave = GI->HasAnySave();
	const bool bHasLiveSession = GI->HasSession();

	TSharedRef<SVerticalBox> Buttons = SNew(SVerticalBox);

	if (bHasLiveSession)
	{
		Buttons->AddSlot().AutoHeight()
		[
			MenuButton(LOCTEXT("Resume", "RESUME"), FForgeStyle::Gold(), [this]()
			{
				OnNavigate.ExecuteIfBound((int32)EForgeScreen::BrainEditor);
			}, true)
		];
	}
	else if (bHasSave)
	{
		Buttons->AddSlot().AutoHeight()
		[
			MenuButton(LOCTEXT("Continue", "CONTINUE EVOLUTION"), FForgeStyle::Gold(), [this]()
			{
				GI->ContinueLatest();
			}, true)
		];
	}

	Buttons->AddSlot().AutoHeight()
	[
		MenuButton(LOCTEXT("NewBrain", "NEW BRAIN"), FForgeStyle::SoftGreen(), [this]()
		{
			OnNavigate.ExecuteIfBound((int32)EForgeScreen::NewBrain);
		})
	];
	Buttons->AddSlot().AutoHeight()
	[
		MenuButton(LOCTEXT("HowToPlay", "HOW TO PLAY"), FForgeStyle::Blue(), [this]()
		{
			OnNavigate.ExecuteIfBound((int32)EForgeScreen::HowToPlay);
		})
	];
	if (bHasSave)
	{
		Buttons->AddSlot().AutoHeight()
		[
			MenuButton(LOCTEXT("Load", "LOAD EXPERIMENT"), FForgeStyle::Cyan(), [this]()
			{
				OnNavigate.ExecuteIfBound((int32)EForgeScreen::LoadExperiment);
			})
		];
	}
	if (bHasLiveSession)
	{
		Buttons->AddSlot().AutoHeight()
		[
			MenuButton(LOCTEXT("Archive", "RESEARCH ARCHIVE"), FForgeStyle::Purple(), [this]()
			{
				OnNavigate.ExecuteIfBound((int32)EForgeScreen::ResearchArchive);
			})
		];
	}
	Buttons->AddSlot().AutoHeight()
	[
		MenuButton(LOCTEXT("Settings", "SETTINGS"), FForgeStyle::Teal(), [this]()
		{
			OnNavigate.ExecuteIfBound((int32)EForgeScreen::Settings);
		})
	];
	Buttons->AddSlot().AutoHeight()
	[
		MenuButton(LOCTEXT("Credits", "CREDITS"), FForgeStyle::TextDim(), [this]()
		{
			OnNavigate.ExecuteIfBound((int32)EForgeScreen::Credits);
		})
	];
	Buttons->AddSlot().AutoHeight()
	[
		MenuButton(LOCTEXT("Exit", "EXIT"), FForgeStyle::Red(), []()
		{
			FPlatformMisc::RequestExit(false);
		})
	];

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.22f) [ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox).WidthOverride(444).HeightOverride(144)
				[
					SNew(SImage).Image(&Style.Logo)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Tagline", "Grow a human mind from noise."))
				.Font(Style.Font(12))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 26, 0, 0)
			[
				Buttons
			]
			+ SVerticalBox::Slot().FillHeight(1.f) [ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 14)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Footer", "Reinforcement Forge 1.0.0   |   (c) 2026 Bull Axiom"))
				.Font(Style.Font(9))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim() * 0.7f))
			]
		)
	];
}

// ============================================================== new brain

void SNewBrainScreen::Construct(const FArguments& InArgs, UForgeGameInstance* InGI)
{
	GI = InGI;
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	const int32 DefaultSeed = (int32)(FDateTime::UtcNow().GetTicks() % 899999) + 100000;

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.25f) [ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SBox).WidthOverride(480)
				[
					ForgeUI::Panel(LOCTEXT("NewBrainTitle", "NEW BRAIN"),
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("NewBrainDesc", "A fresh genome. Fifteen structures, four of them barely awake.\nEvery seed hides different pressures, signal meanings and latent talents."))
							.Font(Style.Font(10))
							.AutoWrapText(true)
							.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 2)
						[
							SNew(STextBlock).Text(LOCTEXT("LineageName", "LINEAGE NAME"))
							.Font(Style.Font(9, true)).ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(NameBox, SEditableTextBox)
							.Text(FText::FromString(FString::Printf(TEXT("Lineage %s"), *FDateTime::Now().ToString(TEXT("%b%d-%H%M")))))
							.Font(Style.Font(11))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 2)
						[
							SNew(STextBlock).Text(LOCTEXT("Seed", "SEED  (share it - same seed, same hidden world)"))
							.Font(Style.Font(9, true)).ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SAssignNew(SeedBox, SEditableTextBox)
								.Text(FText::FromString(FString::FromInt(DefaultSeed)))
								.Font(Style.Font(11))
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0)
							[
								SNew(SNeonButton)
								.Text(LOCTEXT("Reroll", "REROLL"))
								.FontSize(9)
								.Color(FForgeStyle::Cyan())
								.OnClicked(FSimpleDelegate::CreateLambda([this]()
								{
									const int32 NewSeed = (int32)(FDateTime::UtcNow().GetTicks() % 899999) + 100000;
									SeedBox->SetText(FText::FromString(FString::FromInt(NewSeed)));
								}))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
						[
							SNew(SNeonButton)
							.Text(LOCTEXT("Begin", "BEGIN EVOLUTION"))
							.FontSize(13)
							.Color(FForgeStyle::Gold())
							.bFilled(true)
							.OnClicked(FSimpleDelegate::CreateLambda([this]()
							{
								const FString SeedStr = SeedBox->GetText().ToString();
								const int32 Seed = FCString::Atoi(*SeedStr) != 0 ? FCString::Atoi(*SeedStr) : (int32)FDateTime::UtcNow().GetTicks();
								GI->StartNewGame(Seed, NameBox->GetText().ToString());
							}))
						]
					)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				BackRow(OnNavigate)
			]
			+ SVerticalBox::Slot().FillHeight(1.f) [ SNew(SSpacer) ]
		)
	];
}

// ============================================================== load screen

void SLoadScreen::Construct(const FArguments& InArgs, UForgeGameInstance* InGI)
{
	GI = InGI;
	OnNavigate = InArgs._OnNavigate;

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.12f) [ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LoadTitle", "SAVED LINEAGES"))
				.Font(FForgeStyle::Get().Font(20, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Cyan()))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).Padding(0, 16)
			[
				SNew(SBox).WidthOverride(640)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(SlotList, SVerticalBox)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 30)
			[
				BackRow(OnNavigate)
			]
		)
	];

	RefreshSlots();
}

void SLoadScreen::RefreshSlots()
{
	if (!SlotList.IsValid()) { return; }
	const FForgeStyle& Style = FForgeStyle::Get();
	SlotList->ClearChildren();

	const TArray<FSaveSlotInfo> Slots = ForgeSave::ListSlots();
	if (Slots.Num() == 0)
	{
		SlotList->AddSlot().AutoHeight().Padding(0, 20)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoSaves", "No saved lineages. Every brain so far has been lost to time."))
			.Font(Style.Font(11))
			.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
		];
		return;
	}

	for (const FSaveSlotInfo& Slot : Slots)
	{
		const FString File = Slot.FileName;
		const FString BranchName = Slot.LineageName + TEXT(" fork");

		SlotList->AddSlot().AutoHeight().Padding(0, 5)
		[
			SNew(SBorder)
			.BorderImage(&Style.PanelBrush)
			.Padding(14)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Slot.LineageName))
						.Font(Style.Font(13, true))
						.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("viability %.1f%%   gen %d   %d trials   seed %d   %s"),
							Slot.Fitness, Slot.Generation, Slot.TrialCount, Slot.Seed, *Slot.Timestamp.ToString(TEXT("%Y-%m-%d %H:%M")))))
						.Font(Style.Font(9))
						.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(SNeonButton).Text(LOCTEXT("LoadSlot", "LOAD")).FontSize(10).Color(FForgeStyle::SoftGreen())
					.OnClicked(FSimpleDelegate::CreateLambda([this, File]() { GI->LoadGame(File); }))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(SNeonButton).Text(LOCTEXT("Branch", "BRANCH")).FontSize(10).Color(FForgeStyle::Purple())
					.OnClicked(FSimpleDelegate::CreateLambda([this, File, BranchName]()
					{
						GI->BranchLineage(File, BranchName);
						RefreshSlots();
					}))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(SNeonButton).Text(LOCTEXT("Delete", "DELETE")).FontSize(10).Color(FForgeStyle::Red())
					.OnClicked(FSimpleDelegate::CreateLambda([this, File]()
					{
						ForgeSave::DeleteSlot(File);
						RefreshSlots();
					}))
				]
			]
		];
	}
}

// ============================================================== research archive

void SResearchArchive::Construct(const FArguments& InArgs, UForgeGameInstance* InGI)
{
	GI = InGI;
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

	if (!GI->HasSession())
	{
		Content->AddSlot().AutoHeight().Padding(20)
		[
			SNew(STextBlock).Text(LOCTEXT("NoSession", "No active experiment."))
			.Font(Style.Font(12)).ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
		];
	}
	else
	{
		FForgeSession& S = *GI->Session;

		// ---- discovered brain functions
		TSharedRef<SVerticalBox> Regions = SNew(SVerticalBox);
		for (const FBrainRegion& R : S.Brain.Regions)
		{
			const FString Line = R.bAwake
				? FString::Printf(TEXT("%s   -   %s   (confidence %.0f%%)"),
					*R.HiddenLabel,
					R.NameKnown() ? RegionRoleName(R.Role) : TEXT("function unknown"),
					R.DiscoveryConfidence * 100.f)
				: FString::Printf(TEXT("%s   -   dormant"), *R.HiddenLabel);

			Regions->AddSlot().AutoHeight().Padding(0, 2)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Line)).Font(Style.Font(10))
					.ColorAndOpacity(FSlateColor(R.NameKnown() ? FForgeStyle::TextBright() : FForgeStyle::TextDim()))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(R.FullyKnown() ? FText::FromString(RegionRoleDescription(R.Role)) : FText::GetEmpty())
					.Font(Style.Font(9))
					.AutoWrapText(true)
					.Visibility(R.FullyKnown() ? EVisibility::Visible : EVisibility::Collapsed)
					.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
				]
			];
		}
		Content->AddSlot().AutoHeight().Padding(0, 6)
		[
			ForgeUI::Panel(LOCTEXT("Functions", "DISCOVERED BRAIN FUNCTIONS"), Regions)
		];

		// ---- fitness contributors
		TSharedRef<SVerticalBox> Dims = SNew(SVerticalBox);
		int32 Hidden = 0;
		for (int32 i = 0; i < FitnessDimCount(); ++i)
		{
			if (!S.Fitness.DimKnown[i]) { Hidden++; continue; }
			Dims->AddSlot().AutoHeight().Padding(0, 2)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s : %.0f%%"), FitnessDimName((EFitnessDim)i), S.Fitness.Dims[i] * 100.f)))
				.Font(Style.Font(10))
				.ColorAndOpacity(FSlateColor(FForgeStyle::SoftGreen()))
			];
		}
		Dims->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Unidentified contributors: %d hidden systems"), Hidden)))
			.Font(Style.Font(10))
			.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
		];
		Content->AddSlot().AutoHeight().Padding(0, 6)
		[
			ForgeUI::Panel(LOCTEXT("Contributors", "FITNESS CONTRIBUTORS"), Dims)
		];

		// ---- heuristics
		TSharedRef<SVerticalBox> Heur = SNew(SVerticalBox);
		if (S.Discovery.Heuristics.Num() == 0)
		{
			Heur->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("NoH", "Nothing understood yet.")).Font(Style.Font(10))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
			];
		}
		for (const FHeuristic& H : S.Discovery.Heuristics)
		{
			Heur->AddSlot().AutoHeight().Padding(0, 2)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s  [%s]  (confidence %.0f%%, n=%d)"),
					*H.Text, *H.BonusLabel, H.Confidence * 100.f, H.SampleSize)))
				.Font(Style.Font(10))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			];
		}
		Content->AddSlot().AutoHeight().Padding(0, 6)
		[
			ForgeUI::Panel(LOCTEXT("AllHeuristics", "HEURISTICS"), Heur)
		];

		// ---- trial history
		TSharedRef<SVerticalBox> Trials = SNew(SVerticalBox);
		const int32 Show = FMath::Min(25, S.History.Num());
		for (int32 i = S.History.Num() - 1; i >= S.History.Num() - Show; --i)
		{
			const FTrialResult& R = S.History[i];
			Trials->AddSlot().AutoHeight().Padding(0, 1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("#%04d  %s   score %.0f%%"),
					R.TrialNumber, *R.VisibleSummary(), R.Score01 * 100.f)))
				.Font(Style.Font(9))
				.ColorAndOpacity(FSlateColor(R.Score01 > 0.5f ? FForgeStyle::SoftGreen() : FForgeStyle::TextDim()))
			];
		}
		Content->AddSlot().AutoHeight().Padding(0, 6)
		[
			ForgeUI::Panel(LOCTEXT("TrialHistory", "TRIAL HISTORY"), Trials)
		];

		// ---- experiment log
		TSharedRef<SVerticalBox> Log = SNew(SVerticalBox);
		const int32 ShowLog = FMath::Min(40, S.ArchiveLog.Num());
		for (int32 i = S.ArchiveLog.Num() - 1; i >= S.ArchiveLog.Num() - ShowLog; --i)
		{
			Log->AddSlot().AutoHeight().Padding(0, 1)
			[
				SNew(STextBlock).Text(FText::FromString(S.ArchiveLog[i])).Font(Style.Font(9))
				.AutoWrapText(true).ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
			];
		}
		Content->AddSlot().AutoHeight().Padding(0, 6)
		[
			ForgeUI::Panel(LOCTEXT("Log", "EXPERIMENT LOG"), Log)
		];

		// ---- stats
		Content->AddSlot().AutoHeight().Padding(0, 6)
		[
			ForgeUI::Panel(LOCTEXT("Stats", "STATISTICS"),
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(
					TEXT("Seed %d   |   Generation %d   |   Trials %d\nMutations %d   |   Prunes %d   |   Patterns %d / %d\nKnowledge %.0f%%"),
					S.Seed, S.Generation, S.TrialCounter, S.MutationCount, S.PruneCount,
					S.Patterns.Patterns.Num(), S.Patterns.SlotCap, S.Knowledge() * 100.f)))
				.Font(Style.Font(10))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			)
		];
	}

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 30, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ArchiveTitle", "RESEARCH ARCHIVE"))
				.Font(Style.Font(20, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Purple()))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center)
			[
				SNew(SBox).WidthOverride(700)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot() [ Content ]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
			[
				BackRow(OnNavigate, GI->HasSession() ? EForgeScreen::BrainEditor : EForgeScreen::MainMenu)
			]
		)
	];
}

// ============================================================== settings

void SSettingsScreen::Construct(const FArguments& InArgs, UForgeGameInstance* InGI)
{
	GI = InGI;
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	auto VolumeRow = [this, &Style](const FText& Label, float* Value) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(140)
			[
				SNew(STextBlock).Text(Label).Font(Style.Font(10, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SSlider)
			.Value(*Value)
			.OnValueChanged(FOnFloatValueChanged::CreateLambda([this, Value](float V)
			{
				*Value = V;
				GI->ApplyAudioSettings();
			}))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([Value]()
			{
				return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(*Value * 100.f)));
			}))
			.Font(Style.Font(10))
			.ColorAndOpacity(FSlateColor(FForgeStyle::Cyan()))
		];
	};

	auto ChoiceRow = [this, &Style](const FText& Label, std::initializer_list<FText> Options, int32* Value, TFunction<void()> OnChanged) -> TSharedRef<SWidget>
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(140)
			[
				SNew(STextBlock).Text(Label).Font(Style.Font(10, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
		];
		int32 Idx = 0;
		for (const FText& Opt : Options)
		{
			const int32 ThisIdx = Idx++;
			Row->AddSlot().AutoWidth().Padding(3, 0)
			[
				SNew(SNeonButton)
				.Text(Opt)
				.FontSize(9)
				.Color(FForgeStyle::Teal())
				.IsEnabledAttr(TAttribute<bool>::CreateLambda([Value, ThisIdx]() { return *Value != ThisIdx; }))
				.OnClicked(FSimpleDelegate::CreateLambda([Value, ThisIdx, OnChanged]()
				{
					*Value = ThisIdx;
					OnChanged();
				}))
			];
		}
		return Row;
	};

	auto ToggleRow = [this, &Style](const FText& Label, bool* Value) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(140)
			[
				SNew(STextBlock).Text(Label).Font(Style.Font(10, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SNeonButton)
			.Text(TAttribute<FText>::CreateLambda([Value]()
			{
				return *Value ? LOCTEXT("On", "ON") : LOCTEXT("Off", "OFF");
			}))
			.FontSize(9)
			.Color(FForgeStyle::SoftGreen())
			.OnClicked(FSimpleDelegate::CreateLambda([Value]() { *Value = !*Value; }))
		];
	};

	FForgeSettings& S = GI->Settings;

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 40, 0, 10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SettingsTitle", "SETTINGS"))
				.Font(Style.Font(20, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Teal()))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center)
			[
				SNew(SBox).WidthOverride(560)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
						[
							ForgeUI::Panel(LOCTEXT("Audio", "AUDIO"),
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ VolumeRow(LOCTEXT("Master", "Master Volume"), &S.MasterVolume) ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ VolumeRow(LOCTEXT("Sfx", "Effects Volume"), &S.SfxVolume) ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ VolumeRow(LOCTEXT("Music", "Music Volume"), &S.MusicVolume) ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("MusicNote", "The score is generated live - it grows more musical as your brain gets fitter."))
									.Font(Style.Font(9)).AutoWrapText(true)
									.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
								]
							)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
						[
							ForgeUI::Panel(LOCTEXT("Display", "DISPLAY"),
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
								[
									ChoiceRow(LOCTEXT("WindowMode", "Window Mode"),
										{ LOCTEXT("FS", "FULLSCREEN"), LOCTEXT("WFS", "BORDERLESS"), LOCTEXT("Win", "WINDOWED") },
										&S.WindowMode, [this]() { ApplyDisplay(); })
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
								[
									ChoiceRow(LOCTEXT("Quality", "Quality"),
										{ LOCTEXT("QLow", "LOW"), LOCTEXT("QMed", "MEDIUM"), LOCTEXT("QHigh", "HIGH"), LOCTEXT("QEpic", "EPIC") },
										&S.QualityLevel, [this]() { ApplyDisplay(); })
								]
							)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
						[
							ForgeUI::Panel(LOCTEXT("Gameplay", "GAMEPLAY"),
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ ToggleRow(LOCTEXT("Autosave", "Autosave"), &S.bAutosave) ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ ToggleRow(LOCTEXT("Hints", "Tutorial Hints"), &S.bShowTutorialHints) ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4) [ ToggleRow(LOCTEXT("Flicker", "Reduced Flicker"), &S.bReducedFlicker) ]
							)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SNeonButton)
						.Text(LOCTEXT("SaveSettings", "SAVE & BACK"))
						.FontSize(11)
						.Color(FForgeStyle::Gold())
						.OnClicked(FSimpleDelegate::CreateLambda([this]()
						{
							GI->Settings.Save();
							OnNavigate.ExecuteIfBound((int32)(GI->HasSession() ? EForgeScreen::BrainEditor : EForgeScreen::MainMenu));
						}))
					]
				]
			]
		)
	];
}

void SSettingsScreen::ApplyDisplay()
{
	GI->ApplyDisplaySettings();
}

// ============================================================== how to play

namespace
{
	struct FManualSection
	{
		const TCHAR* Heading;
		const TCHAR* Body;
	};
	struct FManualPage
	{
		const TCHAR* Title;
		TArray<FManualSection> Sections;
	};

	TArray<FManualPage> BuildManual()
	{
		return
		{
			{ TEXT("WHAT YOU ARE DOING"),
			{
				{ TEXT("The goal"),
				  TEXT("Grow a viable human brain - 100% viability. You never control the human. You breed the wiring that produces one.") },
				{ TEXT("You start blind"),
				  TEXT("Nobody tells you what \"Region 06\" does or what \"Unknown Trial 07\" tests. You learn by watching what lights up when things go well - exactly like real evolution, but faster and with better music.") },
				{ TEXT("The vibe"),
				  TEXT("This plays like Mini Motorways: trials stream by themselves, you calmly reshape the network, and every generation you pick one upgrade. There is no fail state - only brains that work strangely.") },
			}},
			{ TEXT("READING THE SCREEN"),
			{
				{ TEXT("The graph"),
				  TEXT("Hexagons are brain regions. Bright ones are awake; dark dashed ones are dormant. Lines between them are pathways - pulses travel along them when signals flow. The brighter a region, the harder it is working.") },
				{ TEXT("Top bar"),
				  TEXT("FITNESS SCORE is your cumulative points (goes up forever). VIABILITY is the real goal - the hidden fitness of the brain, 0-100%. SIGNAL EFFICIENCY and STABILITY are health gauges: low stability leads to seizures.") },
				{ TEXT("Right panels"),
				  TEXT("Reward history (your viability over time), recent trial scores (green bars = good trials), the color legend, pattern memory, and details of whatever you selected.") },
				{ TEXT("Bottom bar"),
				  TEXT("Tools and the big gold RUN 100 TRIALS button. The ENERGY counter is your only currency.") },
			}},
			{ TEXT("TRIALS AND FITNESS"),
			{
				{ TEXT("Trials"),
				  TEXT("A trial is a compressed life moment: find warmth, dodge a threat, remember a direction. The game feeds signals into sensory regions and reads what the motor regions do. You'll see the trial strip up top ticking away.") },
				{ TEXT("Scoring"),
				  TEXT("Each trial scores 0-100% and nudges hidden fitness dimensions (survival, mobility, memory...). Viability is a GEOMETRIC mean - a brain great at language but unable to avoid pain stays near zero. Balance wins.") },
				{ TEXT("Energy"),
				  TEXT("Every finished trial pays energy; good trials pay triple. Every action costs energy. If you're broke, just let it run for a while - calm is a strategy.") },
				{ TEXT("Speed"),
				  TEXT("1x / 2x / 4x / 8x and pause, top right (or keys 0-4). Space makes sure trials are flowing.") },
			}},
			{ TEXT("YOUR SEVEN TOOLS"),
			{
				{ TEXT("MUTATE (M) - 50e"),
				  TEXT("Random rewiring. Most mutations do nothing or hurt; a few are gold. The preview is vague on purpose - it sharpens as your knowledge grows.") },
				{ TEXT("PRUNE (P) - 15e"),
				  TEXT("Deletes the weakest, least-used connections. Cheap, safe, and it raises stability and efficiency. When in doubt, prune.") },
				{ TEXT("REWARD PULSE (Q) - 30e"),
				  TEXT("Strengthens every pathway that was recently active. Fire it right after a good trial to lock in whatever just worked.") },
				{ TEXT("RANDOMIZE (R) - 40e"),
				  TEXT("Scrambles weights. A panic button for a stuck brain.") },
				{ TEXT("CONNECT (C or Ctrl-drag) - 10e"),
				  TEXT("Wire regions (or neurons) together. Signals can't use paths that don't exist - sensory regions need routes to motor regions.") },
				{ TEXT("LOCK (L) - 20e"),
				  TEXT("Protects selected structures from mutation. Lock the good stuff, then mutate fearlessly.") },
				{ TEXT("RUN 100 TRIALS - 100e"),
				  TEXT("Queues a hands-off batch. Set 8x, sip coffee, watch the line go up.") },
			}},
			{ TEXT("REGIONS AND SUBNETWORKS"),
			{
				{ TEXT("Enter a region"),
				  TEXT("Double-click any awake region. Inside is its own network: green INPUT hexes (what the region hears), teal OUTPUT hexes (what it says to the rest of the brain), and neurons between them.") },
				{ TEXT("Edit inside"),
				  TEXT("Same tools work locally and hit harder. + ADD NODE grows neurons (the button cycles types: relay, threshold, memory, inhibitor, oscillator, reward). Select a connection to BOOST, DAMPEN, or CUT it. Esc returns to the whole brain.") },
				{ TEXT("Awakening"),
				  TEXT("Dormant regions can awaken (150e, double-click) once your phase allows. Each awake region adds capability - and energy drain. Don't wake everything at once.") },
				{ TEXT("Auto-test (T)"),
				  TEXT("Inside a region, runs a quick calibration and reports its stability and energy draw.") },
			}},
			{ TEXT("DISCOVERY AND PATTERNS"),
			{
				{ TEXT("Names emerge"),
				  TEXT("When a region consistently carries scoring trials, its confidence grows: \"Region 06\" becomes \"Vision? 55%\" and eventually a confirmed function with a description. The Research Archive keeps everything you've learned.") },
				{ TEXT("Heuristics"),
				  TEXT("Sometimes the game hands you an observation (\"Loop suppression improves stability +9%\"). These aren't flavor - each discovered heuristic grants its bonus permanently.") },
				{ TEXT("Pattern memory"),
				  TEXT("Inside a region, select 2-6 neurons (Shift-click) and CAPTURE SELECTION. Stamp that motif into other regions later (35e). Use a pattern three times and the game names it.") },
			}},
			{ TEXT("GENERATIONS, PHASES, WINNING"),
			{
				{ TEXT("Generations"),
				  TEXT("Every 25 trials the flow pauses and you pick ONE of three adaptations - more energy, a pattern slot, an insight probe, permanent stability, a plasticity surge, or a free awakening. This is your build. Lineages diverge here.") },
				{ TEXT("Phases"),
				  TEXT("Viability milestones unlock development: Reflex, then Perception (8%), Memory (20%), Social (35%), Language (50%), Cognition (65%). New phases bring new trial types and new regions to awaken.") },
				{ TEXT("Trouble"),
				  TEXT("Stability under 30% for too long causes a seizure - fitness is lost. Prune, dampen, or lock to calm a chaotic brain. Strange-but-viable is fine. Chaos is not.") },
				{ TEXT("Winning"),
				  TEXT("100% viability = a viable human brain. Your save is a lineage - BRANCH it from Load Experiment to explore different futures from the same past. Share your seed so friends face the same hidden world.") },
			}},
		};
	}
}

void SHowToPlayScreen::Construct(const FArguments& InArgs, UForgeGameInstance* InGI)
{
	GI = InGI;
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 28, 0, 4)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("HowToTitle", "HOW TO PLAY"))
				.Font(Style.Font(20, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Gold()))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).Padding(0, 8)
			[
				SNew(SBox).WidthOverride(720)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(PageBox, SVerticalBox)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 24)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					SNew(SBox).WidthOverride(120)
					[
						SNew(SNeonButton).Text(LOCTEXT("PrevPage", "<  PREV")).FontSize(10).Color(FForgeStyle::Cyan())
						.OnClicked(FSimpleDelegate::CreateLambda([this]() { ShowPage(PageIndex - 1); }))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::CreateLambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("%d / 7"), PageIndex + 1));
					}))
					.Font(Style.Font(11, true))
					.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					SNew(SBox).WidthOverride(120)
					[
						SNew(SNeonButton).Text(LOCTEXT("NextPage", "NEXT  >")).FontSize(10).Color(FForgeStyle::Cyan())
						.OnClicked(FSimpleDelegate::CreateLambda([this]() { ShowPage(PageIndex + 1); }))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(24, 0, 0, 0)
				[
					SNew(SBox).WidthOverride(160)
					[
						SNew(SNeonButton).Text(LOCTEXT("ManualDone", "GOT IT")).FontSize(10).Color(FForgeStyle::Gold())
						.OnClicked(FSimpleDelegate::CreateLambda([this]()
						{
							OnNavigate.ExecuteIfBound((int32)(GI->HasSession() ? EForgeScreen::BrainEditor : EForgeScreen::MainMenu));
						}))
					]
				]
			]
		)
	];

	ShowPage(0);
}

void SHowToPlayScreen::ShowPage(int32 Index)
{
	static const TArray<FManualPage> Manual = BuildManual();
	PageIndex = FMath::Clamp(Index, 0, Manual.Num() - 1);
	if (!PageBox.IsValid()) { return; }

	const FForgeStyle& Style = FForgeStyle::Get();
	const FManualPage& Page = Manual[PageIndex];
	PageBox->ClearChildren();

	TSharedRef<SVerticalBox> Sections = SNew(SVerticalBox);
	for (const FManualSection& S : Page.Sections)
	{
		Sections->AddSlot().AutoHeight().Padding(0, 7)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(S.Heading))
				.Font(Style.Font(11, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Cyan()))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(S.Body))
				.Font(Style.Font(10))
				.AutoWrapText(true)
				.LineHeightPercentage(1.15f)
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
		];
	}

	PageBox->AddSlot().AutoHeight()
	[
		ForgeUI::Panel(FText::FromString(Page.Title), Sections, FForgeStyle::Gold())
	];
}

// ============================================================== credits

void SCreditsScreen::Construct(const FArguments& InArgs)
{
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	ChildSlot
	[
		ScreenFrame(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.3f) [ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CreditsTitle", "REINFORCEMENT FORGE"))
				.Font(Style.Font(22, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Cyan()))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 20)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CreditsBody",
					"A Bull Axiom experiment\n\n"
					"Design, code, sound and generative score\nsynthesized in-engine\n\n"
					"Built with Unreal Engine 5.8\n\n"
					"Every brain you grow is yours alone.\nShare your seed; the darkness is deterministic."))
				.Font(Style.Font(11))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				BackRow(OnNavigate)
			]
			+ SVerticalBox::Slot().FillHeight(1.f) [ SNew(SSpacer) ]
		)
	];
}

#undef LOCTEXT_NAMESPACE
