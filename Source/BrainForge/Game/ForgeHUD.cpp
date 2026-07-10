#include "ForgeHUD.h"
#include "UI/SBrainEditor.h"
#include "UI/SMenuScreens.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"

AForgeHUD::AForgeHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

UForgeGameInstance* AForgeHUD::GI() const
{
	return GetGameInstance<UForgeGameInstance>();
}

void AForgeHUD::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine && GEngine->GameViewport)
	{
		SAssignNew(Root, SOverlay);
		GEngine->GameViewport->AddViewportWidgetContent(Root.ToSharedRef(), 0);
		RebuildScreen();
	}
}

void AForgeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine && GEngine->GameViewport && Root.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Root.ToSharedRef());
	}
	Root.Reset();
	CurrentWidget.Reset();
	EditorWidget.Reset();
	Super::EndPlay(EndPlayReason);
}

void AForgeHUD::HandleNavigate(int32 Screen)
{
	if (UForgeGameInstance* G = GI())
	{
		G->Navigate((EForgeScreen)Screen);
	}
}

void AForgeHUD::RebuildScreen()
{
	UForgeGameInstance* G = GI();
	if (!G || !Root.IsValid()) { return; }

	Root->ClearChildren();
	EditorWidget.Reset();

	FOnForgeNavigate Nav = FOnForgeNavigate::CreateUObject(this, &AForgeHUD::HandleNavigate);

	// screens that need a session fall back to the menu
	EForgeScreen Screen = G->CurrentScreen;
	if (Screen == EForgeScreen::BrainEditor && !G->HasSession())
	{
		Screen = EForgeScreen::MainMenu;
		G->CurrentScreen = Screen;
	}

	switch (Screen)
	{
	case EForgeScreen::BrainEditor:
		CurrentWidget = SAssignNew(EditorWidget, SBrainEditor, G->Session.Get()).OnNavigate(Nav);
		break;
	case EForgeScreen::NewBrain:
		CurrentWidget = SNew(SNewBrainScreen, G).OnNavigate(Nav);
		break;
	case EForgeScreen::LoadExperiment:
		CurrentWidget = SNew(SLoadScreen, G).OnNavigate(Nav);
		break;
	case EForgeScreen::ResearchArchive:
		CurrentWidget = SNew(SResearchArchive, G).OnNavigate(Nav);
		break;
	case EForgeScreen::Settings:
		CurrentWidget = SNew(SSettingsScreen, G).OnNavigate(Nav);
		break;
	case EForgeScreen::Credits:
		CurrentWidget = SNew(SCreditsScreen).OnNavigate(Nav);
		break;
	default:
		CurrentWidget = SNew(SMainMenu, G).OnNavigate(Nav);
		break;
	}

	Root->AddSlot() [ CurrentWidget.ToSharedRef() ];
	BuiltScreen = Screen;
	G->bScreenDirty = false;

	if (EditorWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(EditorWidget);
	}
}

void AForgeHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UForgeGameInstance* G = GI();
	if (!G) { return; }

	if (G->bScreenDirty || G->CurrentScreen != BuiltScreen)
	{
		RebuildScreen();
	}

	const float Fitness01 = G->HasSession() ? G->Session->DisplayFitness() / 100.f : 0.05f;
	if (G->Audio)
	{
		G->Audio->TickMusic(GetWorld(), Fitness01, DeltaSeconds);
	}

	if (G->HasSession())
	{
		FForgeSession& S = *G->Session;

		// simulation only advances while the editor is on-screen
		if (BuiltScreen == EForgeScreen::BrainEditor)
		{
			S.Update(DeltaSeconds);
		}

		// drain events: sounds + toasts
		for (const FForgeEvent& E : S.EventQueue)
		{
			if (G->Audio)
			{
				G->Audio->Play(E.Sound, GetWorld());
			}
			if (!E.Text.IsEmpty() && EditorWidget.IsValid())
			{
				EditorWidget->AddToast(E.Text, E.bImportant);
			}
		}
		S.EventQueue.Reset();

		// gentle tutorial drip (guide: experimentation in darkness, but not lost in it)
		if (G->Settings.bShowTutorialHints && EditorWidget.IsValid() && BuiltScreen == EForgeScreen::BrainEditor)
		{
			struct FHint { int32 AtTrial; const TCHAR* Text; };
			static const FHint Hints[] =
			{
				{ 1,  TEXT("Trials run on their own. Watch which regions light up.") },
				{ 3,  TEXT("Nobody knows what these regions do yet. Consistency is evidence.") },
				{ 6,  TEXT("Try MUTATE (M) - most mutations fail, but evolution is patient.") },
				{ 10, TEXT("Ctrl-drag between regions to wire them. Signals need paths.") },
				{ 15, TEXT("Double-click a region to edit the neurons inside it.") },
				{ 20, TEXT("PRUNE (P) trims dead pathways. A quiet brain is a cheap brain.") },
				{ 26, TEXT("Generations offer adaptations. There are no wrong picks, only lineages.") },
			};
			if (NextHintIndex < UE_ARRAY_COUNT(Hints) && S.TrialCounter >= Hints[NextHintIndex].AtTrial)
			{
				EditorWidget->AddToast(Hints[NextHintIndex].Text, false);
				NextHintIndex++;
			}
		}

		// autosave: every 90 seconds and on generation rollover
		if (G->Settings.bAutosave)
		{
			AutosaveTimer += DeltaSeconds;
			const bool bGenChanged = S.Generation != LastSavedGeneration && S.PendingOffers.Num() == 0;
			if (AutosaveTimer > 90.f || bGenChanged)
			{
				AutosaveTimer = 0.f;
				LastSavedGeneration = S.Generation;
				G->SaveNow();
			}
		}
	}
}
