#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ForgeTypes.h"
#include "ForgeAudio.generated.h"

class USoundWaveProcedural;
class UAudioComponent;

/**
 * All game audio is synthesized in-engine at startup (guide section 28):
 * distinct cues per signal type, and a generative ambient score that grows
 * more musical and coherent as the brain's fitness rises.
 */
UCLASS()
class BRAINFORGE_API UForgeAudioManager : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 SampleRate = 48000;

	void Initialize();

	/** Fire-and-forget cue. */
	void Play(EForgeSound Sound, UWorld* World, float VolumeScale = 1.f);

	/** Keep the generative music buffer fed. Call every frame. */
	void TickMusic(UWorld* World, float Fitness01, float DeltaSeconds);

	void StopMusic();

	// settings 0..1
	float MasterVolume = 0.8f;
	float SfxVolume = 0.8f;
	float MusicVolume = 0.6f;

private:
	void SynthesizeAll();
	TArray<int16> Synth(EForgeSound Sound) const;

	// cached PCM per cue
	TArray<int16> Cache[(int32)EForgeSound::COUNT];

	// music state
	UPROPERTY()
	TObjectPtr<USoundWaveProcedural> MusicWave;
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;

	int32 MusicChordStep = 0;
	FRandomStream MusicRng;
	float Musicality = 0.f;    // smoothed fitness driving coherence

	void QueueMusicChunk();
};
