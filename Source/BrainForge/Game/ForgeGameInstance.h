#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/ForgeSession.h"
#include "Core/SaveSystem.h"
#include "Core/ForgeAudio.h"
#include "ForgeGameInstance.generated.h"

/** Which top-level screen the game is showing. */
enum class EForgeScreen : uint8
{
	MainMenu,
	NewBrain,
	LoadExperiment,
	BrainEditor,
	ResearchArchive,
	Settings,
	Credits,
	HowToPlay
};

UCLASS()
class BRAINFORGE_API UForgeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// ---- screen routing (HUD rebuilds when dirty)
	EForgeScreen CurrentScreen = EForgeScreen::MainMenu;
	bool bScreenDirty = true;
	void Navigate(EForgeScreen Screen) { CurrentScreen = Screen; bScreenDirty = true; }

	// ---- game state
	TUniquePtr<FForgeSession> Session;
	FForgeSettings Settings;

	UPROPERTY()
	TObjectPtr<UForgeAudioManager> Audio;

	bool HasSession() const { return Session.IsValid(); }
	bool HasAnySave() const;

	void StartNewGame(int32 Seed, const FString& LineageName);
	bool LoadGame(const FString& FileName);
	bool ContinueLatest();
	void SaveNow();
	bool BranchLineage(const FString& SrcFile, const FString& NewName);

	void ApplyAudioSettings();
	void ApplyDisplaySettings();
};
