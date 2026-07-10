#include "ForgeGameInstance.h"
#include "Core/ForgeStyle.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"

void UForgeGameInstance::Init()
{
	Super::Init();

	Settings.Load();

	FForgeStyle::Get().Initialize();

	Audio = NewObject<UForgeAudioManager>(this);
	Audio->Initialize();
	ApplyAudioSettings();
	ApplyDisplaySettings();

	// smoke-test / demo hook: boot straight into a fresh brain
	int32 AutoSeed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("forgeautostart="), AutoSeed) ||
		FParse::Param(FCommandLine::Get(), TEXT("forgeautostart")))
	{
		StartNewGame(AutoSeed != 0 ? AutoSeed : 123456, TEXT("Autostart"));
	}
}

void UForgeGameInstance::Shutdown()
{
	if (Session.IsValid() && Settings.bAutosave)
	{
		SaveNow();
	}
	Settings.Save();
	Super::Shutdown();
}

bool UForgeGameInstance::HasAnySave() const
{
	return ForgeSave::ListSlots().Num() > 0;
}

void UForgeGameInstance::StartNewGame(int32 Seed, const FString& LineageName)
{
	Session = MakeUnique<FForgeSession>();
	Session->NewGame(Seed, LineageName);
	Navigate(EForgeScreen::BrainEditor);
}

bool UForgeGameInstance::LoadGame(const FString& FileName)
{
	TUniquePtr<FForgeSession> NewSession = MakeUnique<FForgeSession>();
	if (!ForgeSave::LoadSession(*NewSession, FileName))
	{
		return false;
	}
	Session = MoveTemp(NewSession);
	Navigate(EForgeScreen::BrainEditor);
	return true;
}

bool UForgeGameInstance::ContinueLatest()
{
	const TArray<FSaveSlotInfo> Slots = ForgeSave::ListSlots();
	if (Slots.Num() == 0) { return false; }
	return LoadGame(Slots[0].FileName);
}

void UForgeGameInstance::SaveNow()
{
	if (Session.IsValid())
	{
		ForgeSave::SaveSession(*Session, ForgeSave::SlotFileFor(Session->LineageName));
	}
}

bool UForgeGameInstance::BranchLineage(const FString& SrcFile, const FString& NewName)
{
	TUniquePtr<FForgeSession> Branch = MakeUnique<FForgeSession>();
	if (!ForgeSave::LoadSession(*Branch, SrcFile))
	{
		return false;
	}
	Branch->LineageName = NewName;
	Branch->ArchiveLog.Add(FString::Printf(TEXT("Lineage branched as '%s'."), *NewName));
	const bool bOk = ForgeSave::SaveSession(*Branch, ForgeSave::SlotFileFor(NewName));
	return bOk;
}

void UForgeGameInstance::ApplyAudioSettings()
{
	if (Audio)
	{
		Audio->MasterVolume = Settings.MasterVolume;
		Audio->SfxVolume = Settings.SfxVolume;
		Audio->MusicVolume = Settings.MusicVolume;
	}
}

void UForgeGameInstance::ApplyDisplaySettings()
{
	if (!GEngine) { return; }
	if (UGameUserSettings* GUS = GEngine->GetGameUserSettings())
	{
		const EWindowMode::Type Modes[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
		GUS->SetFullscreenMode(Modes[FMath::Clamp(Settings.WindowMode, 0, 2)]);
		GUS->SetScreenResolution(FIntPoint(Settings.ResolutionX, Settings.ResolutionY));
		GUS->SetOverallScalabilityLevel(FMath::Clamp(Settings.QualityLevel, 0, 3));
		GUS->ApplySettings(false);
	}
}
