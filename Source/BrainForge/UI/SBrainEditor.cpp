#include "SBrainEditor.h"
#include "SGraphCanvas.h"
#include "SNeonWidgets.h"
#include "Core/ForgeStyle.h"
#include "Game/ForgeGameInstance.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "Forge"

namespace
{
	FText Pct(float V) { return FText::FromString(FString::Printf(TEXT("%.1f%%"), V)); }
}

void SBrainEditor::Construct(const FArguments& InArgs, FForgeSession* InSession)
{
	Session = InSession;
	OnNavigate = InArgs._OnNavigate;
	const FForgeStyle& Style = FForgeStyle::Get();

	ChildSlot
	[
		SNew(SOverlay)

		// ---------------- canvas fills everything
		+ SOverlay::Slot()
		[
			SAssignNew(Canvas, SGraphCanvas, Session)
		]

		// ---------------- HUD chrome
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 0)
			[
				BuildTopBar()
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 0)
			[
				BuildTrialStrip()
			]

			+ SVerticalBox::Slot().FillHeight(1.f).Padding(8, 6)
			[
				SNew(SHorizontalBox)

				// left column
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SBox).WidthOverride(252)
					[
						BuildLeftPanel()
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(10, 0)
					[
						BuildBreadcrumb()
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
					[
						SAssignNew(ToastBox, SVerticalBox)
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SSpacer)
					]
				]

				// right column
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SBox).WidthOverride(266)
					[
						BuildRightPanel()
					]
				]
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
			[
				BuildBottomBar()
			]
		]

		// ---------------- modal layer (generation rewards / victory)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(ModalBox, SVerticalBox)
		]
	];

	RefreshHeuristics();
	RefreshPatterns();
	RefreshSelection();
}

// ============================================================== top bar

TSharedRef<SWidget> SBrainEditor::BuildTopBar()
{
	const FForgeStyle& Style = FForgeStyle::Get();

	auto SpeedButton = [this, &Style](int32 Sp, const TCHAR* Label) -> TSharedRef<SWidget>
	{
		return SNew(SBox).WidthOverride(44).Padding(2, 0)
		[
			SNew(SNeonButton)
			.Text(FText::FromString(Label))
			.FontSize(10)
			.Color(FForgeStyle::Cyan())
			.OnClicked(FSimpleDelegate::CreateLambda([this, Sp]() { SetSpeed(Sp); }))
		];
	};

	return SNew(SBorder)
	.BorderImage(&Style.PanelBrushDark)
	.Padding(FMargin(14, 8))
	[
		SNew(SHorizontalBox)

		// logo
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(148).HeightOverride(48)
			[
				SNew(SImage).Image(&Style.Logo)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 0, 0).VAlign(VAlign_Center)
		[
			ForgeUI::StatBlock(LOCTEXT("FitnessScore", "FITNESS SCORE"),
				TAttribute<FText>::CreateLambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("%s  +%.1f/s"),
						*FText::AsNumber((int64)Session->FitnessScore).ToString(), Session->ScoreRate));
				}),
				FForgeStyle::Green(), 20)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(20, 0, 0, 0).VAlign(VAlign_Center)
		[
			ForgeUI::StatBlock(LOCTEXT("Viability", "VIABILITY"),
				TAttribute<FText>::CreateLambda([this]() { return Pct(Session->DisplayFitness()); }),
				FForgeStyle::Gold(), 20)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(20, 0, 0, 0).VAlign(VAlign_Center)
		[
			ForgeUI::StatBlock(LOCTEXT("BestRun", "BEST RUN"),
				TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber((int64)Session->BestRun); }),
				FForgeStyle::TextBright(), 16)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(20, 0, 0, 0).VAlign(VAlign_Center)
		[
			ForgeUI::StatBlock(LOCTEXT("Generation", "GENERATION"),
				TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber(Session->Generation); }),
				FForgeStyle::TextBright(), 16)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(20, 0, 0, 0).VAlign(VAlign_Center)
		[
			ForgeUI::StatBlock(LOCTEXT("Phase", "PHASE"),
				TAttribute<FText>::CreateLambda([this]() { return FText::FromString(PhaseName(Session->Phase)); }),
				FForgeStyle::Purple(), 11)
		]

		// gauges
		+ SHorizontalBox::Slot().AutoWidth().Padding(24, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SRadialGauge)
				.Size(40)
				.Color(FForgeStyle::SoftGreen())
				.Percent(TAttribute<float>::CreateLambda([this]() { return Session->Brain.SignalEfficiency(); }))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0).VAlign(VAlign_Center)
			[
				ForgeUI::StatBlock(LOCTEXT("Efficiency", "SIGNAL EFFICIENCY"),
					TAttribute<FText>::CreateLambda([this]() { return Pct(Session->Brain.SignalEfficiency() * 100.f); }),
					FForgeStyle::TextBright(), 14)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(18, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SRadialGauge)
				.Size(40)
				.Color(FForgeStyle::Cyan())
				.Percent(TAttribute<float>::CreateLambda([this]() { return Session->Brain.GlobalStability(); }))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0).VAlign(VAlign_Center)
			[
				ForgeUI::StatBlock(LOCTEXT("Stability", "STABILITY"),
					TAttribute<FText>::CreateLambda([this]() { return Pct(Session->Brain.GlobalStability() * 100.f); }),
					FForgeStyle::TextBright(), 14)
			]
		]

		+ SHorizontalBox::Slot().FillWidth(1.f) [ SNew(SSpacer) ]

		// playback
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
			[
				SNew(SBox).WidthOverride(44)
				[
					SNew(SNeonButton)
					.Text(TAttribute<FText>::CreateLambda([this]()
					{
						return FText::FromString(Session->Speed == 0 ? TEXT(">") : TEXT("||"));
					}))
					.FontSize(12)
					.Color(FForgeStyle::Teal())
					.OnClicked(FSimpleDelegate::CreateLambda([this]()
					{
						SetSpeed(Session->Speed == 0 ? 1 : 0);
					}))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth() [ SpeedButton(1, TEXT("1x")) ]
			+ SHorizontalBox::Slot().AutoWidth() [ SpeedButton(2, TEXT("2x")) ]
			+ SHorizontalBox::Slot().AutoWidth() [ SpeedButton(4, TEXT("4x")) ]
			+ SHorizontalBox::Slot().AutoWidth() [ SpeedButton(8, TEXT("8x")) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("%dx"), Session->Speed));
				}))
				.Font(FForgeStyle::Get().Font(12, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::Cyan()))
			]
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(44)
			[
				SNew(SNeonButton)
				.Text(LOCTEXT("Help", "?"))
				.FontSize(12)
				.Color(FForgeStyle::Blue())
				.OnClicked(FSimpleDelegate::CreateLambda([this]()
				{
					OnNavigate.ExecuteIfBound((int32)EForgeScreen::HowToPlay);
				}))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(88)
			[
				SNew(SNeonButton)
				.Text(LOCTEXT("Menu", "MENU"))
				.FontSize(10)
				.Color(FForgeStyle::TextDim())
				.OnClicked(FSimpleDelegate::CreateLambda([this]()
				{
					OnNavigate.ExecuteIfBound((int32)EForgeScreen::MainMenu);
				}))
			]
		]
	];
}

// ============================================================== trial strip

TSharedRef<SWidget> SBrainEditor::BuildTrialStrip()
{
	const FForgeStyle& Style = FForgeStyle::Get();

	return SNew(SBorder)
	.BorderImage(&Style.PanelBrushDark)
	.Padding(FMargin(16, 6))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([this]() { return TrialLabel(); }))
			.Font(Style.Font(11, true))
			.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(280).HeightOverride(8)
			[
				SNew(SBarChart)
				.Data(nullptr)
				.MinHeight(8)
				.Visibility(EVisibility::Hidden)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([this]()
			{
				if (Session->Runner.IsActive())
				{
					const int32 Blocks = 24;
					const int32 Filled = (int32)(Session->Runner.Progress() * Blocks);
					FString Bar;
					for (int32 i = 0; i < Blocks; ++i) { Bar += (i < Filled) ? TEXT("|") : TEXT("."); }
					return FText::FromString(Bar);
				}
				return FText::FromString(TEXT("  idle - the brain hums  "));
			}))
			.Font(Style.Font(10))
			.ColorAndOpacity(FSlateColor(FForgeStyle::Teal()))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(14, 0, 0, 0).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([this]()
			{
				return Session->Runner.IsActive() ? LOCTEXT("Running", "Running...")
					: (Session->bAutoRun ? LOCTEXT("Queued", "Queued") : LOCTEXT("Paused", "Paused"));
			}))
			.Font(Style.Font(9))
			.ColorAndOpacity(FSlateColor(FForgeStyle::SoftGreen()))
		]
	];
}

// ============================================================== breadcrumb

TSharedRef<SWidget> SBrainEditor::BuildBreadcrumb()
{
	const FForgeStyle& Style = FForgeStyle::Get();

	return SNew(SBox)
	.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
	{
		return Session->OpenRegionId != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed;
	}))
	[
		SNew(SBorder)
		.BorderImage(&Style.PanelBrush)
		.Padding(FMargin(10, 6))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SNeonButton)
				.Text(LOCTEXT("BackBrain", "< WHOLE BRAIN"))
				.FontSize(9)
				.Color(FForgeStyle::Cyan())
				.HotkeyHint(TEXT("Esc"))
				.OnClicked(FSimpleDelegate::CreateLambda([this]()
				{
					Session->ExitRegion();
					if (Canvas.IsValid()) { Canvas->ClearSelection(); Canvas->FocusContent(); }
				}))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([this]()
				{
					if (const FBrainRegion* R = Session->OpenRegion())
					{
						return FText::FromString(FString::Printf(TEXT("EDITING: %s   stability %.0f%%   energy %.1f"),
							*R->DisplayName(), R->Net.Stability * 100.f, R->Net.EnergyUse));
					}
					return FText::GetEmpty();
				}))
				.Font(Style.Font(10, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
		]
	];
}

// ============================================================== left panel

TSharedRef<SWidget> SBrainEditor::BuildLeftPanel()
{
	const FForgeStyle& Style = FForgeStyle::Get();

	auto ActionBtn = [this](const FText& Label, FLinearColor Color, const FString& Hotkey, TFunction<void()> Fn, float Cost = 0.f) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(0, 3))
		[
			SNew(SNeonButton)
			.Text(Label)
			.Color(Color)
			.FontSize(10)
			.HotkeyHint(Hotkey)
			.IsEnabledAttr(TAttribute<bool>::CreateLambda([this, Cost]() { return Cost <= 0.f || Session->CanAfford(Cost); }))
			.OnClicked(FSimpleDelegate::CreateLambda([Fn]() { Fn(); }))
		];
	};

	return SNew(SVerticalBox)

	+ SVerticalBox::Slot().AutoHeight()
	[
		ForgeUI::Panel(LOCTEXT("Loop", "REINFORCEMENT LOOP"),
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LoopDesc", "Increase fitness through iterative wiring and reinforcement."))
				.Font(Style.Font(9))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
			]
			+ SVerticalBox::Slot().AutoHeight() [ ActionBtn(LOCTEXT("RunTrial", "RUN TRIAL"), FForgeStyle::SoftGreen(), TEXT("Space"), [this]() { DoRunTrial(); }) ]
			+ SVerticalBox::Slot().AutoHeight() [ ActionBtn(LOCTEXT("Randomize", "RANDOMIZE"), FForgeStyle::Cyan(), TEXT("R"), [this]() { Session->ActionRandomize(); }, ForgeCost::Randomize) ]
			+ SVerticalBox::Slot().AutoHeight() [ ActionBtn(LOCTEXT("Mutate", "MUTATE"), FForgeStyle::Purple(), TEXT("M"), [this]() { Session->ActionMutate(false); }, ForgeCost::Mutate) ]
			+ SVerticalBox::Slot().AutoHeight() [ ActionBtn(LOCTEXT("Prune", "PRUNE"), FForgeStyle::Red(), TEXT("P"), [this]() { Session->ActionPrune(); }, ForgeCost::Prune) ]
			+ SVerticalBox::Slot().AutoHeight() [ ActionBtn(LOCTEXT("RewardPulse", "REWARD PULSE"), FForgeStyle::Gold(), TEXT("Q"), [this]() { Session->ActionRewardPulse(); }, ForgeCost::RewardPulse) ]
			+ SVerticalBox::Slot().AutoHeight() [ ActionBtn(LOCTEXT("LockPattern", "LOCK PATTERN"), FForgeStyle::Blue(), TEXT("L"), [this]()
			{
				if (Canvas.IsValid())
				{
					TArray<int32> Conns;
					if (Canvas->SelectedConnId != INDEX_NONE) { Conns.Add(Canvas->SelectedConnId); }
					Session->ActionLock(Canvas->SelectedNodeIds, Conns);
				}
			}, ForgeCost::LockPattern) ]
		)
	]

	+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
	[
		SNew(SBox)
		.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
		{
			return Session->TutorialDone() ? EVisibility::Collapsed : EVisibility::Visible;
		}))
		[
			ForgeUI::Panel(LOCTEXT("FirstSteps", "FIRST STEPS"),
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::CreateLambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("Step %d of %d"),
							FMath::Min(Session->TutorialStage + 1, FForgeSession::NumTutorialSteps), FForgeSession::NumTutorialSteps));
					}))
					.Font(Style.Font(8, true))
					.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::CreateLambda([this]()
					{
						return FText::FromString(Session->TutorialObjective());
					}))
					.Font(Style.Font(10))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FForgeStyle::Gold()))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::CreateLambda([this]()
					{
						return FText::FromString(Session->TutorialProgress());
					}))
					.Font(Style.Font(9))
					.Visibility(TAttribute<EVisibility>::CreateLambda([this]()
					{
						return Session->TutorialProgress().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
					}))
					.ColorAndOpacity(FSlateColor(FForgeStyle::SoftGreen()))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("StepReward", "each step: +40 energy"))
					.Font(Style.Font(8))
					.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
				],
				FForgeStyle::Gold())
		]
	]

	+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
	[
		ForgeUI::Panel(LOCTEXT("Heuristics", "DISCOVERED HEURISTICS"),
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(HeuristicsBox, SVerticalBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(SNeonButton)
				.Text(LOCTEXT("Insights", "VIEW ALL INSIGHTS  >"))
				.FontSize(9)
				.Color(FForgeStyle::TextDim())
				.OnClicked(FSimpleDelegate::CreateLambda([this]()
				{
					OnNavigate.ExecuteIfBound((int32)EForgeScreen::ResearchArchive);
				}))
			]
		)
	];
}

void SBrainEditor::RefreshHeuristics()
{
	if (!HeuristicsBox.IsValid()) { return; }
	const FForgeStyle& Style = FForgeStyle::Get();
	HeuristicsBox->ClearChildren();

	const TArray<FHeuristic>& List = Session->Discovery.Heuristics;
	if (List.Num() == 0)
	{
		HeuristicsBox->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoHeuristics", "No heuristics discovered yet.\nRun trials. Watch. Guess."))
			.Font(Style.Font(9))
			.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
		];
	}

	const int32 Show = FMath::Min(4, List.Num());
	for (int32 i = List.Num() - Show; i < List.Num(); ++i)
	{
		HeuristicsBox->AddSlot().AutoHeight().Padding(0, 3)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(List[i].Text))
				.Font(Style.Font(9))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(List[i].BonusLabel))
				.Font(Style.Font(9, true))
				.ColorAndOpacity(FSlateColor(FForgeStyle::SoftGreen()))
			]
		];
	}
	LastHeuristicCount = List.Num();
}

// ============================================================== right panel

TSharedRef<SWidget> SBrainEditor::BuildRightPanel()
{
	const FForgeStyle& Style = FForgeStyle::Get();

	auto LegendRow = [&Style](FLinearColor Color, const FText& Label) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(18).HeightOverride(3)
			[
				SNew(SImage).Image(&FForgeStyle::Get().WhiteBrush).ColorAndOpacity(FSlateColor(Color))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0)
		[
			SNew(STextBlock).Text(Label).Font(FForgeStyle::Get().Font(8)).ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
		];
	};

	return SNew(SScrollBox)
	.ScrollBarVisibility(EVisibility::Collapsed)

	+ SScrollBox::Slot()
	[
		ForgeUI::Panel(LOCTEXT("RewardHistory", "REWARD HISTORY (LIVE)"),
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(78)
				[
					SNew(SLineChart)
					.Data(&Session->FitnessCurve)
					.Color(FForgeStyle::SoftGreen())
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("viability %.1f%%  |  %d trials"),
						Session->DisplayFitness(), Session->TrialCounter));
				}))
				.Font(Style.Font(9))
				.ColorAndOpacity(FSlateColor(FForgeStyle::SoftGreen()))
			]
		)
	]

	+ SScrollBox::Slot().Padding(0, 8, 0, 0)
	[
		ForgeUI::Panel(LOCTEXT("RecentScores", "RECENT TRIAL SCORES"),
			SNew(SBox).HeightOverride(64)
			[
				SNew(SBarChart).Data(&Session->RecentScores)
			]
		)
	]

	+ SScrollBox::Slot().Padding(0, 8, 0, 0)
	[
		ForgeUI::Panel(LOCTEXT("Legend", "LEGEND"),
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Green(), LOCTEXT("LegInput", "Input")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Blue(), LOCTEXT("LegRelay", "Relay")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Purple(), LOCTEXT("LegThresh", "Threshold")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Orange(), LOCTEXT("LegMemory", "Memory")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Gold(), LOCTEXT("LegReward", "Reward")) ]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Cyan(), LOCTEXT("LegExc", "Excitatory")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Purple(), LOCTEXT("LegInh", "Inhibitory")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Red(), LOCTEXT("LegUnstable", "Unstable")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Blue(), LOCTEXT("LegDelay", "Delayed")) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2) [ LegendRow(FForgeStyle::Teal(), LOCTEXT("LegOut", "Output")) ]
			]
		)
	]

	+ SScrollBox::Slot().Padding(0, 8, 0, 0)
	[
		ForgeUI::Panel(LOCTEXT("PatternMemory", "PATTERN MEMORY"),
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("%d / %d slots"),
						Session->Patterns.Patterns.Num(), Session->Patterns.SlotCap));
				}))
				.Font(Style.Font(9))
				.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
			[
				SAssignNew(PatternBox, SVerticalBox)
			]
		)
	]

	+ SScrollBox::Slot().Padding(0, 8, 0, 0)
	[
		ForgeUI::Panel(LOCTEXT("Selected", "SELECTED"),
			SAssignNew(SelectionBox, SVerticalBox)
		)
	];
}

void SBrainEditor::RefreshPatterns()
{
	if (!PatternBox.IsValid()) { return; }
	const FForgeStyle& Style = FForgeStyle::Get();
	PatternBox->ClearChildren();

	// capture-current-selection button (subnetwork view only)
	PatternBox->AddSlot().AutoHeight().Padding(0, 2)
	[
		SNew(SNeonButton)
		.Text(LOCTEXT("SavePattern", "CAPTURE SELECTION"))
		.FontSize(9)
		.Color(FForgeStyle::Teal())
		.IsEnabledAttr(TAttribute<bool>::CreateLambda([this]()
		{
			return Session->OpenRegionId != INDEX_NONE && Canvas.IsValid() && Canvas->SelectedNodeIds.Num() >= 2 && Session->Patterns.HasSpace();
		}))
		.OnClicked(FSimpleDelegate::CreateLambda([this]()
		{
			if (Canvas.IsValid()) { Session->ActionSavePattern(Canvas->SelectedNodeIds); }
		}))
	];

	for (const FSavedPattern& P : Session->Patterns.Patterns)
	{
		const int32 Pid = P.Id;
		PatternBox->AddSlot().AutoHeight().Padding(0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SNeonButton)
				.Text(FText::FromString(FString::Printf(TEXT("%s (%dn)"), *P.Name, P.Nodes.Num())))
				.FontSize(9)
				.Color(P.bNamed ? FForgeStyle::Gold() : FForgeStyle::TextDim())
				.IsEnabledAttr(TAttribute<bool>::CreateLambda([this]() { return Session->OpenRegionId != INDEX_NONE; }))
				.OnClicked(FSimpleDelegate::CreateLambda([this, Pid]()
				{
					if (Canvas.IsValid())
					{
						Canvas->PendingPatternId = Pid;
						Session->PushEvent(EForgeSound::Click, TEXT("Click the canvas to stamp the pattern (35 energy)."));
					}
				}))
			]
		];
	}
	LastPatternHash = Session->Patterns.Patterns.Num() * 100 + Session->Patterns.SlotCap;
}

int32 SBrainEditor::SelectionHash() const
{
	if (!Canvas.IsValid()) { return 0; }
	int32 H = Session->OpenRegionId * 7919;
	for (int32 Id : Canvas->SelectedNodeIds) { H = H * 31 + Id; }
	H = H * 31 + Canvas->SelectedConnId;
	H = H * 31 + Canvas->SelectedRegionId;
	H = H * 31 + Canvas->SelectedLinkId;
	return H;
}

void SBrainEditor::RefreshSelection()
{
	if (!SelectionBox.IsValid() || !Canvas.IsValid()) { return; }
	const FForgeStyle& Style = FForgeStyle::Get();
	SelectionBox->ClearChildren();

	auto AddLine = [this, &Style](const FString& Text, FLinearColor Color, int32 Size = 9, bool bBold = false)
	{
		SelectionBox->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(STextBlock).Text(FText::FromString(Text)).Font(Style.Font(Size, bBold))
			.AutoWrapText(true).ColorAndOpacity(FSlateColor(Color))
		];
	};

	// ---- region selected (brain view)
	if (Session->OpenRegionId == INDEX_NONE && Canvas->SelectedRegionId != INDEX_NONE)
	{
		if (const FBrainRegion* R = Session->Brain.FindRegion(Canvas->SelectedRegionId))
		{
			AddLine(R->DisplayName(), FForgeStyle::Gold(), 12, true);
			if (R->FullyKnown())
			{
				AddLine(RegionRoleDescription(R->Role), FForgeStyle::TextDim());
			}
			else if (R->NameKnown())
			{
				AddLine(FString::Printf(TEXT("Probable function. Confidence %.0f%%."), R->DiscoveryConfidence * 100.f), FForgeStyle::TextDim());
			}
			else
			{
				AddLine(TEXT("Function unknown. Run trials and watch when it lights up."), FForgeStyle::TextDim());
			}
			if (R->bAwake)
			{
				AddLine(FString::Printf(TEXT("nodes %d   links %d"), R->Net.Nodes.Num(), R->Net.Connections.Num()), FForgeStyle::TextBright());
				AddLine(FString::Printf(TEXT("stability %.0f%%   energy %.1f"), R->Net.Stability * 100.f, R->Net.EnergyUse), FForgeStyle::TextBright());

				const int32 Rid = R->Id;
				SelectionBox->AddSlot().AutoHeight().Padding(0, 5)
				[
					SNew(SNeonButton)
					.Text(LOCTEXT("EnterRegion", "ENTER SUBNETWORK"))
					.FontSize(9)
					.Color(FForgeStyle::Cyan())
					.OnClicked(FSimpleDelegate::CreateLambda([this, Rid]()
					{
						Session->EnterRegion(Rid);
						if (Canvas.IsValid()) { Canvas->ClearSelection(); Canvas->FocusContent(); }
					}))
				];
			}
			else
			{
				const bool bReady = (int32)RegionUnlockPhase(R->Role) <= (int32)Session->Phase;
				AddLine(bReady ? TEXT("Dormant. It could awaken.") : TEXT("Dormant. Not developmentally ready."), FForgeStyle::TextDim());
				if (bReady)
				{
					const int32 Rid = R->Id;
					SelectionBox->AddSlot().AutoHeight().Padding(0, 5)
					[
						SNew(SNeonButton)
						.Text(TAttribute<FText>::CreateLambda([this]()
						{
							return Session->bFreeAwaken ? LOCTEXT("AwakenFree", "AWAKEN (FREE)") : LOCTEXT("Awaken", "AWAKEN (150)");
						}))
						.FontSize(9)
						.Color(FForgeStyle::Gold())
						.OnClicked(FSimpleDelegate::CreateLambda([this, Rid]() { Session->ActionAwakenRegion(Rid); }))
					];
				}
			}
		}
	}
	// ---- region link selected
	else if (Session->OpenRegionId == INDEX_NONE && Canvas->SelectedLinkId != INDEX_NONE)
	{
		AddLine(TEXT("Inter-region pathway"), FForgeStyle::Cyan(), 11, true);
		const int32 Lid = Canvas->SelectedLinkId;
		SelectionBox->AddSlot().AutoHeight().Padding(0, 5)
		[
			SNew(SNeonButton)
			.Text(LOCTEXT("CutLink", "SEVER PATHWAY (5)"))
			.FontSize(9)
			.Color(FForgeStyle::Red())
			.OnClicked(FSimpleDelegate::CreateLambda([this, Lid]()
			{
				Session->ActionDisconnectRegionLink(Lid);
				if (Canvas.IsValid()) { Canvas->SelectedLinkId = INDEX_NONE; }
			}))
		];
	}
	// ---- node(s) selected (network view)
	else if (Session->OpenRegionId != INDEX_NONE && Canvas->SelectedNodeIds.Num() > 0)
	{
		FNeuralNetwork* Net = Session->OpenNetwork();
		if (Net && Canvas->SelectedNodeIds.Num() == 1)
		{
			if (const FNeuralNode* N = Net->FindNode(Canvas->SelectedNodeIds[0]))
			{
				AddLine(FString::Printf(TEXT("%s node  #%d"), NodeTypeName(N->Type), N->Id), FForgeStyle::NodeTypeColor((uint8)N->Type), 12, true);
				AddLine(FString::Printf(TEXT("activation %.2f   avg %.2f"), N->Activation, N->AvgActivity), FForgeStyle::TextBright());
				AddLine(FString::Printf(TEXT("threshold %.2f   bias %+.2f"), N->Threshold, N->Bias), FForgeStyle::TextBright());
				AddLine(FString::Printf(TEXT("decay %.2f/s   noise %.2f   plasticity %.2f"), N->Decay, N->Noise, N->Plasticity), FForgeStyle::TextBright());
				if (N->bLocked) { AddLine(TEXT("LOCKED - protected from mutation"), FForgeStyle::Gold()); }
			}
		}
		else if (Net)
		{
			AddLine(FString::Printf(TEXT("%d nodes selected"), Canvas->SelectedNodeIds.Num()), FForgeStyle::TextBright(), 11, true);
			AddLine(TEXT("Capture as a pattern, or lock them."), FForgeStyle::TextDim());
		}

		TArray<int32> Ids = Canvas->SelectedNodeIds;
		SelectionBox->AddSlot().AutoHeight().Padding(0, 5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 2, 0)
			[
				SNew(SNeonButton)
				.Text(LOCTEXT("DeleteNode", "REMOVE"))
				.FontSize(9)
				.Color(FForgeStyle::Red())
				.OnClicked(FSimpleDelegate::CreateLambda([this, Ids]()
				{
					if (FNeuralNetwork* Net2 = Session->OpenNetwork())
					{
						for (int32 Id : Ids)
						{
							const FNeuralNode* N = Net2->FindNode(Id);
							if (N && !N->bLocked && N->Type != ENodeType::Input && N->Type != ENodeType::Output)
							{
								Net2->RemoveNode(Id);
							}
						}
						Session->PushEvent(EForgeSound::Prune);
					}
					if (Canvas.IsValid()) { Canvas->ClearSelection(); }
				}))
			]
		];
	}
	// ---- connection selected
	else if (Session->OpenRegionId != INDEX_NONE && Canvas->SelectedConnId != INDEX_NONE)
	{
		FNeuralNetwork* Net = Session->OpenNetwork();
		FNeuralConnection* C = Net ? Net->FindConnection(Canvas->SelectedConnId) : nullptr;
		if (C)
		{
			AddLine(FString::Printf(TEXT("%s connection"), ConnTypeName(C->Type)), FForgeStyle::ConnTypeColor((uint8)C->Type), 12, true);
			AddLine(FString::Printf(TEXT("weight %+.2f   traffic %.2f"), C->Weight, C->Traffic), FForgeStyle::TextBright());
			AddLine(FString::Printf(TEXT("reinforced x%d   delay %.1fs"), C->ReinforceCount, C->Delay), FForgeStyle::TextBright());

			const int32 Cid = C->Id;
			SelectionBox->AddSlot().AutoHeight().Padding(0, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 2, 0)
				[
					SNew(SNeonButton).Text(LOCTEXT("Boost", "BOOST")).FontSize(9).Color(FForgeStyle::SoftGreen())
					.OnClicked(FSimpleDelegate::CreateLambda([this, Cid]() { Session->ActionBoost(Cid, +0.3f); }))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0)
				[
					SNew(SNeonButton).Text(LOCTEXT("Dampen", "DAMPEN")).FontSize(9).Color(FForgeStyle::Blue())
					.OnClicked(FSimpleDelegate::CreateLambda([this, Cid]() { Session->ActionBoost(Cid, -0.3f); }))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0, 0, 0)
				[
					SNew(SNeonButton).Text(LOCTEXT("Cut", "CUT")).FontSize(9).Color(FForgeStyle::Red())
					.OnClicked(FSimpleDelegate::CreateLambda([this, Cid]()
					{
						Session->ActionDisconnect(Cid);
						if (Canvas.IsValid()) { Canvas->SelectedConnId = INDEX_NONE; }
					}))
				]
			];
		}
	}
	else
	{
		AddLine(Session->OpenRegionId == INDEX_NONE
			? TEXT("Select a region. Double-click to enter it. Ctrl-drag between regions to wire them.")
			: TEXT("Select a node or connection. Ctrl-drag between nodes to wire them."),
			FForgeStyle::TextDim());
	}

	LastSelectionHash = SelectionHash();
}

// ============================================================== bottom bar

TSharedRef<SWidget> SBrainEditor::BuildBottomBar()
{
	const FForgeStyle& Style = FForgeStyle::Get();

	auto ToolBtn = [this](const FText& Label, FLinearColor Color, const FString& Hotkey, TFunction<void()> Fn, TAttribute<bool> EnabledAttr = true) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(3, 0))
		[
			SNew(SNeonButton)
			.Text(Label)
			.Color(Color)
			.FontSize(9)
			.HotkeyHint(Hotkey)
			.IsEnabledAttr(EnabledAttr)
			.OnClicked(FSimpleDelegate::CreateLambda([Fn]() { Fn(); }))
		];
	};

	TAttribute<bool> InNetwork = TAttribute<bool>::CreateLambda([this]() { return Session->OpenRegionId != INDEX_NONE; });

	return SNew(SBorder)
	.BorderImage(&Style.PanelBrushDark)
	.Padding(FMargin(10, 8))
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("AddNode", "+ ADD NODE"), FForgeStyle::SoftGreen(), TEXT("A"), [this]()
		{
			if (Canvas.IsValid() && Session->OpenRegionId != INDEX_NONE)
			{
				// cycle: relay -> threshold -> memory -> inhibitor -> oscillator -> reward
				static int32 CycleIdx = 0;
				static const ENodeType Cycle[] = { ENodeType::Relay, ENodeType::Threshold, ENodeType::Memory, ENodeType::Inhibitor, ENodeType::Oscillator, ENodeType::Reward };
				Canvas->PendingAddNodeType = (int32)Cycle[CycleIdx++ % 6];
				Session->PushEvent(EForgeSound::Click, FString::Printf(TEXT("Click the canvas to grow a %s node (25 energy)."),
					NodeTypeName((ENodeType)Canvas->PendingAddNodeType)));
			}
		}, InNetwork) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("Connect", "CONNECT"), FForgeStyle::Cyan(), TEXT("C"), [this]()
		{
			if (Canvas.IsValid())
			{
				Canvas->bConnectArmed = true;
				Session->PushEvent(EForgeSound::Click, TEXT("Drag from a node to another to wire them (10 energy)."));
			}
		}) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("Disconnect", "DISCONNECT"), FForgeStyle::Red(), TEXT("X"), [this]()
		{
			if (Canvas.IsValid())
			{
				if (Canvas->SelectedConnId != INDEX_NONE) { Session->ActionDisconnect(Canvas->SelectedConnId); Canvas->SelectedConnId = INDEX_NONE; }
				else if (Canvas->SelectedLinkId != INDEX_NONE) { Session->ActionDisconnectRegionLink(Canvas->SelectedLinkId); Canvas->SelectedLinkId = INDEX_NONE; }
			}
		}) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("Boost2", "BOOST"), FForgeStyle::SoftGreen(), TEXT("B"), [this]()
		{
			if (Canvas.IsValid() && Canvas->SelectedConnId != INDEX_NONE) { Session->ActionBoost(Canvas->SelectedConnId, +0.3f); }
		}, InNetwork) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("Dampen2", "DAMPEN"), FForgeStyle::Blue(), TEXT("D"), [this]()
		{
			if (Canvas.IsValid() && Canvas->SelectedConnId != INDEX_NONE) { Session->ActionBoost(Canvas->SelectedConnId, -0.3f); }
		}, InNetwork) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("MutateCluster", "MUTATE CLUSTER"), FForgeStyle::Purple(), TEXT("K"), [this]()
		{
			Session->ActionMutate(true);
		}) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("DupMotif", "DUPLICATE MOTIF"), FForgeStyle::Teal(), TEXT("N"), [this]()
		{
			Session->ActionDuplicateMotif();
		}, InNetwork) ]

		+ SHorizontalBox::Slot().AutoWidth() [ ToolBtn(LOCTEXT("AutoTest", "AUTO-TEST"), FForgeStyle::Gold(), TEXT("T"), [this]()
		{
			Session->ActionAutoTest();
		}, InNetwork) ]

		+ SHorizontalBox::Slot().FillWidth(1.f) [ SNew(SSpacer) ]

		// run 100 trials - the big gold button
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(230).HeightOverride(40)
			[
				SNew(SNeonButton)
				.Text(TAttribute<FText>::CreateLambda([this]()
				{
					if (Session->BatchRemaining > 0)
					{
						return FText::FromString(FString::Printf(TEXT("BATCH RUNNING  %d LEFT"), Session->BatchRemaining));
					}
					return LOCTEXT("Run100", "RUN 100 TRIALS   (100 energy)");
				}))
				.FontSize(11)
				.Color(FForgeStyle::Gold())
				.bFilled(true)
				.IsEnabledAttr(TAttribute<bool>::CreateLambda([this]()
				{
					return Session->BatchRemaining == 0 && Session->CanAfford(ForgeCost::RunHundred);
				}))
				.OnClicked(FSimpleDelegate::CreateLambda([this]() { Session->ActionRunBatch(); }))
			]
		]

		// energy
		+ SHorizontalBox::Slot().AutoWidth().Padding(14, 0, 4, 0).VAlign(VAlign_Center)
		[
			ForgeUI::StatBlock(LOCTEXT("Energy", "ENERGY"),
				TAttribute<FText>::CreateLambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("%d / %d"),
						FMath::RoundToInt(Session->Energy), FMath::RoundToInt(Session->EnergyCap)));
				}),
				FForgeStyle::Gold(), 18)
		]
	];
}

// ============================================================== modals / toasts

void SBrainEditor::RefreshModal()
{
	if (!ModalBox.IsValid()) { return; }
	const FForgeStyle& Style = FForgeStyle::Get();
	ModalBox->ClearChildren();

	// generation reward choice
	if (Session->PendingOffers.Num() > 0)
	{
		TSharedRef<SHorizontalBox> Offers = SNew(SHorizontalBox);
		for (const FGenerationOffer& O : Session->PendingOffers)
		{
			const int32 Oid = O.Id;
			Offers->AddSlot().AutoWidth().Padding(8, 0)
			[
				SNew(SBox).WidthOverride(190)
				[
					SNew(SBorder)
					.BorderImage(&Style.PanelBrush)
					.Padding(14)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(O.Title)).Font(Style.Font(12, true))
							.ColorAndOpacity(FSlateColor(FForgeStyle::Gold()))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
						[
							SNew(STextBlock).Text(FText::FromString(O.Desc)).Font(Style.Font(9))
							.AutoWrapText(true).ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
						[
							SNew(SNeonButton).Text(LOCTEXT("Choose", "SELECT")).FontSize(10).Color(FForgeStyle::Gold())
							.OnClicked(FSimpleDelegate::CreateLambda([this, Oid]() { Session->ChooseGenerationReward(Oid); }))
						]
					]
				]
			];
		}

		ModalBox->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(&Style.PanelBrushDark)
			.Padding(22)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("GENERATION %d"), Session->Generation)))
					.Font(Style.Font(20, true))
					.ColorAndOpacity(FSlateColor(FForgeStyle::Gold()))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PickOne", "The lineage adapts. Choose one:"))
					.Font(Style.Font(10))
					.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
				[
					Offers
				]
			]
		];
	}
	// victory
	else if (Session->bWon)
	{
		ModalBox->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(&Style.PanelBrushDark)
			.Padding(30)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("WinTitle", "A VIABLE HUMAN BRAIN"))
					.Font(Style.Font(26, true))
					.ColorAndOpacity(FSlateColor(FForgeStyle::Gold()))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10)
				[
					SNew(SBox).WidthOverride(460)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("WinBody", "It began as noise. You reinforced what worked, pruned what didn't, and never once saw the whole picture.\n\nIt thinks. It remembers. It plans. It was not assembled - it was grown."))
						.Font(Style.Font(11))
						.AutoWrapText(true)
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FSlateColor(FForgeStyle::TextBright()))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Seed %d   |   Generation %d   |   %d trials"),
						Session->Seed, Session->Generation, Session->TrialCounter)))
					.Font(Style.Font(10))
					.ColorAndOpacity(FSlateColor(FForgeStyle::TextDim()))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SBox).WidthOverride(200)
						[
							SNew(SNeonButton).Text(LOCTEXT("KeepEvolving", "KEEP EVOLVING")).FontSize(11).Color(FForgeStyle::SoftGreen())
							.OnClicked(FSimpleDelegate::CreateLambda([this]() { Session->bWon = false; }))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SBox).WidthOverride(200)
						[
							SNew(SNeonButton).Text(LOCTEXT("ToMenu", "MAIN MENU")).FontSize(11).Color(FForgeStyle::TextDim())
							.OnClicked(FSimpleDelegate::CreateLambda([this]()
							{
								Session->bWon = false;
								OnNavigate.ExecuteIfBound((int32)EForgeScreen::MainMenu);
							}))
						]
					]
				]
			]
		];
	}

	LastModalHash = Session->PendingOffers.Num() * 1000 + Session->Generation + (Session->bWon ? 5000000 : 0);
}

void SBrainEditor::AddToast(const FString& Text, bool bImportant)
{
	if (!ToastBox.IsValid()) { return; }
	const FForgeStyle& Style = FForgeStyle::Get();

	TSharedRef<SWidget> Toast = SNew(SBorder)
		.BorderImage(&Style.PanelBrush)
		.Padding(FMargin(14, 7))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Text))
			.Font(Style.Font(10, bImportant))
			.ColorAndOpacity(FSlateColor(bImportant ? FForgeStyle::Gold() : FForgeStyle::TextBright()))
		];

	ToastBox->AddSlot().AutoHeight().Padding(0, 2).HAlign(HAlign_Center) [ Toast ];

	FToast T;
	T.Widget = Toast;
	T.Life = bImportant ? 6.f : 3.2f;
	Toasts.Add(T);

	// keep at most 4 visible
	while (Toasts.Num() > 4)
	{
		if (Toasts[0].Widget.IsValid()) { ToastBox->RemoveSlot(Toasts[0].Widget.ToSharedRef()); }
		Toasts.RemoveAt(0);
	}
}

// ============================================================== tick / input

void SBrainEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// expire toasts
	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		Toasts[i].Life -= InDeltaTime;
		if (Toasts[i].Life <= 0.f)
		{
			if (Toasts[i].Widget.IsValid() && ToastBox.IsValid())
			{
				ToastBox->RemoveSlot(Toasts[i].Widget.ToSharedRef());
			}
			Toasts.RemoveAt(i);
		}
	}

	// refresh dynamic panels when their data changes
	if (Session->Discovery.Heuristics.Num() != LastHeuristicCount) { RefreshHeuristics(); }
	if (Session->Patterns.Patterns.Num() * 100 + Session->Patterns.SlotCap != LastPatternHash) { RefreshPatterns(); }
	if (SelectionHash() != LastSelectionHash) { RefreshSelection(); }
	const int32 ModalHash = Session->PendingOffers.Num() * 1000 + Session->Generation + (Session->bWon ? 5000000 : 0);
	if (ModalHash != LastModalHash) { RefreshModal(); }
}

void SBrainEditor::DoRunTrial()
{
	if (!Session->Runner.IsActive())
	{
		Session->bAutoRun = true;
		if (Session->Speed == 0) { Session->Speed = 1; }
	}
}

void SBrainEditor::SetSpeed(int32 NewSpeed)
{
	Session->Speed = NewSpeed;
	Session->PushEvent(EForgeSound::Click);
}

FText SBrainEditor::TrialLabel() const
{
	if (Session->Runner.IsActive())
	{
		const FTrialDef& D = GetTrialDef(Session->Runner.CurrentType());
		const bool bKnown = Session->Discovery.TrialNameKnown(Session->Runner.CurrentType());
		return FText::FromString(FString::Printf(TEXT("Trial %04d   %s"), Session->TrialCounter, bKnown ? D.RealName : D.HiddenName));
	}
	return FText::FromString(FString::Printf(TEXT("Trial %04d complete"), Session->TrialCounter));
}

FReply SBrainEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::SpaceBar) { DoRunTrial(); return FReply::Handled(); }
	if (Key == EKeys::R) { Session->ActionRandomize(); return FReply::Handled(); }
	if (Key == EKeys::M) { Session->ActionMutate(false); return FReply::Handled(); }
	if (Key == EKeys::K) { Session->ActionMutate(true); return FReply::Handled(); }
	if (Key == EKeys::P) { Session->ActionPrune(); return FReply::Handled(); }
	if (Key == EKeys::Q) { Session->ActionRewardPulse(); return FReply::Handled(); }
	if (Key == EKeys::T && Session->OpenRegionId != INDEX_NONE) { Session->ActionAutoTest(); return FReply::Handled(); }
	if (Key == EKeys::N && Session->OpenRegionId != INDEX_NONE) { Session->ActionDuplicateMotif(); return FReply::Handled(); }
	if (Key == EKeys::C && Canvas.IsValid()) { Canvas->bConnectArmed = true; return FReply::Handled(); }
	if (Key == EKeys::L && Canvas.IsValid())
	{
		TArray<int32> Conns;
		if (Canvas->SelectedConnId != INDEX_NONE) { Conns.Add(Canvas->SelectedConnId); }
		Session->ActionLock(Canvas->SelectedNodeIds, Conns);
		return FReply::Handled();
	}
	if (Key == EKeys::X && Canvas.IsValid())
	{
		if (Canvas->SelectedConnId != INDEX_NONE) { Session->ActionDisconnect(Canvas->SelectedConnId); Canvas->SelectedConnId = INDEX_NONE; }
		else if (Canvas->SelectedLinkId != INDEX_NONE) { Session->ActionDisconnectRegionLink(Canvas->SelectedLinkId); Canvas->SelectedLinkId = INDEX_NONE; }
		return FReply::Handled();
	}
	if (Key == EKeys::F) { if (Canvas.IsValid()) { Canvas->FocusContent(); } return FReply::Handled(); }
	if (Key == EKeys::Zero) { SetSpeed(0); return FReply::Handled(); }
	if (Key == EKeys::One) { SetSpeed(1); return FReply::Handled(); }
	if (Key == EKeys::Two) { SetSpeed(2); return FReply::Handled(); }
	if (Key == EKeys::Three) { SetSpeed(4); return FReply::Handled(); }
	if (Key == EKeys::Four) { SetSpeed(8); return FReply::Handled(); }
	if (Key == EKeys::Escape)
	{
		if (Session->OpenRegionId != INDEX_NONE)
		{
			Session->ExitRegion();
			if (Canvas.IsValid()) { Canvas->ClearSelection(); Canvas->FocusContent(); }
		}
		else
		{
			OnNavigate.ExecuteIfBound((int32)EForgeScreen::MainMenu);
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE
