#pragma once

#include "CoreMinimal.h"

class FForgeSession;

/** Persistent player settings. */
struct BRAINFORGE_API FForgeSettings
{
	float MasterVolume = 0.8f;
	float SfxVolume = 0.8f;
	float MusicVolume = 0.6f;
	int32 WindowMode = 1;        // 0 fullscreen, 1 windowed-fullscreen, 2 windowed
	int32 ResolutionX = 1920;
	int32 ResolutionY = 1080;
	int32 QualityLevel = 2;      // 0 low 1 medium 2 high 3 epic
	bool bAutosave = true;
	bool bShowTutorialHints = true;
	bool bReducedFlicker = false; // accessibility: dampens jitter/flash effects

	void Save() const;
	void Load();
};

/** Metadata of one saved lineage. */
struct BRAINFORGE_API FSaveSlotInfo
{
	FString FileName;        // without extension
	FString LineageName;
	int32 Seed = 0;
	int32 Generation = 1;
	int32 TrialCount = 0;
	float Fitness = 0.f;
	FDateTime Timestamp;
};

/**
 * JSON save/load of full sessions, supporting multiple lineage slots
 * (guide section 26: evolutionary lineages).
 */
namespace ForgeSave
{
	BRAINFORGE_API FString SaveDir();

	BRAINFORGE_API bool SaveSession(const FForgeSession& Session, const FString& FileName);
	BRAINFORGE_API bool LoadSession(FForgeSession& Session, const FString& FileName);
	BRAINFORGE_API TArray<FSaveSlotInfo> ListSlots();
	BRAINFORGE_API bool DeleteSlot(const FString& FileName);

	/** Sanitize a lineage name into a filesystem-safe slot file name. */
	BRAINFORGE_API FString SlotFileFor(const FString& LineageName);
}
