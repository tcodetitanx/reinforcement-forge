#include "ForgeAudio.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	constexpr float TwoPi = 6.2831853f;

	FORCEINLINE int16 ToPcm(float V)
	{
		return (int16)(FMath::Clamp(V, -1.f, 1.f) * 32000.f);
	}

	/** Simple additive tone with exponential decay. */
	void AddTone(TArray<float>& Buf, int32 SR, float StartSec, float DurSec, float Freq,
	             float Gain, float Attack = 0.005f, float DecayPow = 4.f, float Detune = 0.f)
	{
		const int32 Start = FMath::Max(0, (int32)(StartSec * SR));
		const int32 Count = (int32)(DurSec * SR);
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Idx = Start + i;
			if (Idx >= Buf.Num()) { break; }
			const float T = (float)i / Count;
			const float TimeSec = (float)i / SR;
			float Env = FMath::Pow(1.f - T, DecayPow);
			if (TimeSec < Attack) { Env *= TimeSec / Attack; }
			const float F = Freq * (1.f + Detune * T);
			Buf[Idx] += FMath::Sin(TwoPi * F * TimeSec) * Gain * Env;
		}
	}

	/** Frequency sweep tone. */
	void AddSweep(TArray<float>& Buf, int32 SR, float StartSec, float DurSec,
	              float FreqA, float FreqB, float Gain, float DecayPow = 2.f, float Distort = 0.f)
	{
		const int32 Start = FMath::Max(0, (int32)(StartSec * SR));
		const int32 Count = (int32)(DurSec * SR);
		float Phase = 0.f;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Idx = Start + i;
			if (Idx >= Buf.Num()) { break; }
			const float T = (float)i / Count;
			const float F = FMath::Lerp(FreqA, FreqB, T);
			Phase += TwoPi * F / SR;
			float V = FMath::Sin(Phase);
			if (Distort > 0.f)
			{
				V = FMath::Clamp(V * (1.f + Distort * 3.f), -1.f, 1.f); // soft clip
			}
			const float Env = FMath::Pow(1.f - T, DecayPow) * FMath::Min(1.f, T * 20.f);
			Buf[Idx] += V * Gain * Env;
		}
	}

	/** Filtered noise burst (one-pole lowpass). */
	void AddNoise(TArray<float>& Buf, int32 SR, float StartSec, float DurSec,
	              float Gain, float Cutoff01, FRandomStream& Rng, float DecayPow = 3.f)
	{
		const int32 Start = FMath::Max(0, (int32)(StartSec * SR));
		const int32 Count = (int32)(DurSec * SR);
		float Lp = 0.f;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Idx = Start + i;
			if (Idx >= Buf.Num()) { break; }
			const float T = (float)i / Count;
			const float White = Rng.FRandRange(-1.f, 1.f);
			Lp += (White - Lp) * Cutoff01;
			Buf[Idx] += Lp * Gain * FMath::Pow(1.f - T, DecayPow);
		}
	}

	TArray<int16> Render(const TArray<float>& Buf)
	{
		TArray<int16> Out;
		Out.SetNumUninitialized(Buf.Num());
		for (int32 i = 0; i < Buf.Num(); ++i) { Out[i] = ToPcm(Buf[i]); }
		return Out;
	}

	TArray<float> MakeBuf(int32 SR, float Seconds)
	{
		TArray<float> Buf;
		Buf.SetNumZeroed((int32)(SR * Seconds));
		return Buf;
	}
}

void UForgeAudioManager::Initialize()
{
	MusicRng.Initialize(1337);
	SynthesizeAll();
}

void UForgeAudioManager::SynthesizeAll()
{
	for (int32 i = 0; i < (int32)EForgeSound::COUNT; ++i)
	{
		Cache[i] = Synth((EForgeSound)i);
	}
}

TArray<int16> UForgeAudioManager::Synth(EForgeSound Sound) const
{
	const int32 SR = SampleRate;
	FRandomStream Rng((int32)Sound * 7919 + 13);

	switch (Sound)
	{
	case EForgeSound::Click:
	{
		TArray<float> B = MakeBuf(SR, 0.06f);
		AddTone(B, SR, 0.f, 0.05f, 1800.f, 0.30f, 0.001f, 5.f);
		AddNoise(B, SR, 0.f, 0.015f, 0.10f, 0.5f, Rng);
		return Render(B);
	}
	case EForgeSound::Hover:
	{
		TArray<float> B = MakeBuf(SR, 0.03f);
		AddTone(B, SR, 0.f, 0.025f, 2400.f, 0.10f, 0.001f, 4.f);
		return Render(B);
	}
	case EForgeSound::Connect:
	{
		TArray<float> B = MakeBuf(SR, 0.14f);
		AddSweep(B, SR, 0.f, 0.09f, 420.f, 1250.f, 0.30f, 2.f);
		AddNoise(B, SR, 0.f, 0.05f, 0.16f, 0.8f, Rng, 4.f);
		AddTone(B, SR, 0.06f, 0.07f, 1650.f, 0.14f, 0.001f, 5.f);
		return Render(B);
	}
	case EForgeSound::Disconnect:
	{
		TArray<float> B = MakeBuf(SR, 0.12f);
		AddSweep(B, SR, 0.f, 0.1f, 950.f, 280.f, 0.26f, 2.5f);
		AddNoise(B, SR, 0.02f, 0.04f, 0.10f, 0.6f, Rng, 4.f);
		return Render(B);
	}
	case EForgeSound::RewardChime:
	{
		TArray<float> B = MakeBuf(SR, 0.65f);
		AddTone(B, SR, 0.f, 0.6f, 659.25f, 0.20f, 0.008f, 3.f, 0.002f);
		AddTone(B, SR, 0.03f, 0.55f, 987.77f, 0.15f, 0.008f, 3.f, -0.002f);
		AddTone(B, SR, 0.07f, 0.5f, 1318.5f, 0.11f, 0.01f, 3.5f);
		return Render(B);
	}
	case EForgeSound::Inhibit:
	{
		TArray<float> B = MakeBuf(SR, 0.2f);
		AddTone(B, SR, 0.f, 0.18f, 180.f, 0.28f, 0.01f, 2.5f);
		return Render(B);
	}
	case EForgeSound::Crackle:
	{
		TArray<float> B = MakeBuf(SR, 0.3f);
		for (int32 i = 0; i < 9; ++i)
		{
			AddNoise(B, SR, Rng.FRandRange(0.f, 0.24f), Rng.FRandRange(0.008f, 0.03f), 0.22f, Rng.FRandRange(0.4f, 0.95f), Rng, 2.f);
		}
		return Render(B);
	}
	case EForgeSound::MemoryEcho:
	{
		TArray<float> B = MakeBuf(SR, 0.5f);
		AddTone(B, SR, 0.f, 0.1f, 1200.f, 0.22f, 0.002f, 4.f);
		AddTone(B, SR, 0.16f, 0.1f, 1200.f, 0.13f, 0.002f, 4.f);
		AddTone(B, SR, 0.32f, 0.1f, 1200.f, 0.07f, 0.002f, 4.f);
		return Render(B);
	}
	case EForgeSound::Mutate:
	{
		TArray<float> B = MakeBuf(SR, 0.45f);
		AddSweep(B, SR, 0.f, 0.4f, 190.f, 820.f, 0.24f, 1.6f, 0.7f);
		AddNoise(B, SR, 0.1f, 0.25f, 0.07f, 0.3f, Rng, 2.f);
		return Render(B);
	}
	case EForgeSound::TrialWin:
	{
		TArray<float> B = MakeBuf(SR, 0.7f);
		AddTone(B, SR, 0.f, 0.6f, 523.25f, 0.14f, 0.02f, 2.5f);
		AddTone(B, SR, 0.02f, 0.6f, 659.25f, 0.12f, 0.02f, 2.5f);
		AddTone(B, SR, 0.05f, 0.62f, 783.99f, 0.11f, 0.02f, 2.8f);
		AddTone(B, SR, 0.12f, 0.5f, 1046.5f, 0.07f, 0.03f, 3.f);
		return Render(B);
	}
	case EForgeSound::TrialSoft:
	{
		TArray<float> B = MakeBuf(SR, 0.25f);
		AddTone(B, SR, 0.f, 0.2f, 880.f, 0.12f, 0.004f, 3.5f);
		AddTone(B, SR, 0.03f, 0.18f, 1320.f, 0.05f, 0.004f, 4.f);
		return Render(B);
	}
	case EForgeSound::Failure:
	{
		TArray<float> B = MakeBuf(SR, 1.0f);
		AddSweep(B, SR, 0.f, 0.85f, 310.f, 55.f, 0.34f, 1.4f);
		AddNoise(B, SR, 0.f, 0.6f, 0.14f, 0.12f, Rng, 1.6f);
		AddTone(B, SR, 0.4f, 0.5f, 48.f, 0.22f, 0.05f, 1.6f);
		return Render(B);
	}
	case EForgeSound::Discovery:
	{
		TArray<float> B = MakeBuf(SR, 0.85f);
		AddTone(B, SR, 0.f, 0.75f, 880.f, 0.16f, 0.004f, 3.f, 0.001f);
		AddTone(B, SR, 0.05f, 0.7f, 1318.5f, 0.11f, 0.006f, 3.2f);
		AddTone(B, SR, 0.12f, 0.65f, 1760.f, 0.08f, 0.008f, 3.4f);
		AddTone(B, SR, 0.2f, 0.55f, 2637.f, 0.05f, 0.01f, 3.8f);
		return Render(B);
	}
	case EForgeSound::Generation:
	{
		TArray<float> B = MakeBuf(SR, 1.4f);
		AddTone(B, SR, 0.f, 1.3f, 220.f, 0.15f, 0.25f, 1.8f);
		AddTone(B, SR, 0.f, 1.3f, 277.18f, 0.12f, 0.3f, 1.8f);
		AddTone(B, SR, 0.f, 1.3f, 329.63f, 0.12f, 0.35f, 1.8f);
		AddTone(B, SR, 0.15f, 1.1f, 493.88f, 0.07f, 0.4f, 2.f);
		return Render(B);
	}
	case EForgeSound::Prune:
	{
		TArray<float> B = MakeBuf(SR, 0.09f);
		AddNoise(B, SR, 0.f, 0.05f, 0.22f, 0.9f, Rng, 3.f);
		AddTone(B, SR, 0.01f, 0.06f, 500.f, 0.16f, 0.001f, 4.f);
		return Render(B);
	}
	case EForgeSound::Lock:
	{
		TArray<float> B = MakeBuf(SR, 0.22f);
		AddTone(B, SR, 0.f, 0.08f, 220.f, 0.3f, 0.001f, 3.f);
		AddTone(B, SR, 0.06f, 0.14f, 1100.f, 0.14f, 0.002f, 4.f);
		return Render(B);
	}
	case EForgeSound::Awaken:
	{
		TArray<float> B = MakeBuf(SR, 1.7f);
		AddSweep(B, SR, 0.f, 1.2f, 80.f, 160.f, 0.26f, 1.2f);
		AddTone(B, SR, 0.4f, 1.2f, 320.f, 0.12f, 0.5f, 1.8f);
		AddTone(B, SR, 0.7f, 0.9f, 640.f, 0.08f, 0.4f, 2.f);
		AddTone(B, SR, 0.9f, 0.7f, 960.f, 0.05f, 0.3f, 2.2f);
		return Render(B);
	}
	case EForgeSound::EnergyLow:
	{
		TArray<float> B = MakeBuf(SR, 0.3f);
		AddTone(B, SR, 0.f, 0.09f, 440.f, 0.2f, 0.002f, 2.f);
		AddTone(B, SR, 0.15f, 0.09f, 415.3f, 0.2f, 0.002f, 2.f);
		return Render(B);
	}
	case EForgeSound::Victory:
	{
		TArray<float> B = MakeBuf(SR, 3.0f);
		const float Chord[4][3] = { {261.6f, 329.6f, 392.f}, {293.7f, 370.f, 440.f}, {329.6f, 415.3f, 493.9f}, {392.f, 493.9f, 587.3f} };
		for (int32 c = 0; c < 4; ++c)
		{
			for (int32 n = 0; n < 3; ++n)
			{
				AddTone(B, SR, c * 0.5f, 1.6f, Chord[c][n], 0.10f, 0.08f, 2.2f);
				AddTone(B, SR, c * 0.5f + 0.05f, 1.2f, Chord[c][n] * 2.f, 0.04f, 0.05f, 2.6f);
			}
		}
		AddTone(B, SR, 2.0f, 1.0f, 1046.5f, 0.09f, 0.01f, 3.f);
		return Render(B);
	}
	default:
		break;
	}

	TArray<float> B = MakeBuf(SR, 0.05f);
	return Render(B);
}

void UForgeAudioManager::Play(EForgeSound Sound, UWorld* World, float VolumeScale)
{
	if (!World) { return; }
	const TArray<int16>& Pcm = Cache[(int32)Sound];
	if (Pcm.Num() == 0) { return; }

	const float Volume = MasterVolume * SfxVolume * VolumeScale;
	if (Volume <= 0.01f) { return; }

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->SampleByteSize = 2;
	Wave->bLooping = false;
	Wave->SoundGroup = SOUNDGROUP_Effects;
	const float Duration = (float)Pcm.Num() / SampleRate;
	Wave->Duration = Duration;
	Wave->QueueAudio((const uint8*)Pcm.GetData(), Pcm.Num() * sizeof(int16));

	UAudioComponent* Comp = UGameplayStatics::SpawnSound2D(World, Wave, Volume);
	if (Comp)
	{
		// procedural waves report indefinite duration; stop them manually
		FTimerHandle Handle;
		TWeakObjectPtr<UAudioComponent> WeakComp = Comp;
		World->GetTimerManager().SetTimer(Handle, [WeakComp]()
		{
			if (WeakComp.IsValid()) { WeakComp->Stop(); }
		}, Duration + 0.15f, false);
	}
}

void UForgeAudioManager::TickMusic(UWorld* World, float Fitness01, float DeltaSeconds)
{
	if (!World) { return; }

	Musicality = FMath::FInterpTo(Musicality, FMath::Clamp(Fitness01, 0.f, 1.f), DeltaSeconds, 0.05f);

	if (!MusicWave)
	{
		MusicWave = NewObject<USoundWaveProcedural>(this);
		MusicWave->SetSampleRate(SampleRate);
		MusicWave->NumChannels = 1;
		MusicWave->SampleByteSize = 2;
		MusicWave->bLooping = true;
		MusicWave->SoundGroup = SOUNDGROUP_Music;
		MusicWave->Duration = INDEFINITELY_LOOPING_DURATION;
	}

	if (!MusicComponent || !MusicComponent->IsPlaying())
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(World, MusicWave, 1.f, 1.f, 0.f, nullptr, true, false);
	}

	if (MusicComponent)
	{
		MusicComponent->SetVolumeMultiplier(MasterVolume * MusicVolume);
	}

	// keep about 2 seconds queued
	while (MusicWave->GetAvailableAudioByteCount() < SampleRate * 2 * (int32)sizeof(int16))
	{
		QueueMusicChunk();
	}
}

void UForgeAudioManager::QueueMusicChunk()
{
	// one chunk = half a bar at ~66 bpm
	const int32 SR = SampleRate;
	const float ChunkSec = 1.8f;
	TArray<float> B = MakeBuf(SR, ChunkSec);

	// Am - Fmaj7 - Cmaj7 - Gadd9, frozen in a calm haze
	static const float Chords[4][4] =
	{
		{110.00f, 164.81f, 220.00f, 261.63f},  // A2 E3 A3 C4
		{ 87.31f, 174.61f, 220.00f, 261.63f},  // F2 F3 A3 C4
		{130.81f, 196.00f, 246.94f, 329.63f},  // C3 G3 B3 E4
		{ 98.00f, 196.00f, 246.94f, 293.66f},  // G2 G3 B3 D4
	};

	const float M = Musicality;
	const int32 ChordIdx = (MusicChordStep / 2) % 4;
	MusicChordStep++;

	// pad: rises with musicality; early game it is barely there
	const float PadGain = 0.015f + 0.065f * M;
	for (int32 n = 0; n < 4; ++n)
	{
		const float Detune = (1.f - M) * 0.012f * ((n % 2 == 0) ? 1.f : -1.f); // dissonant shimmer early
		AddTone(B, SR, 0.f, ChunkSec, Chords[ChordIdx][n] * (1.f + Detune), PadGain, 0.6f, 1.05f);
	}

	// deep noise floor: strong early, fades as the mind coheres
	AddNoise(B, SR, 0.f, ChunkSec, 0.030f * (1.f - M * 0.8f), 0.018f, MusicRng, 1.02f);

	// sparse blips: random/dissonant early, on-scale plucks later
	const float BlipChance = 0.35f + 0.45f * M;
	if (MusicRng.FRand() < BlipChance)
	{
		const float T = MusicRng.FRandRange(0.1f, ChunkSec - 0.5f);
		float Freq;
		if (M > 0.35f)
		{
			// A minor pentatonic: A C D E G
			static const float Penta[5] = {440.f, 523.25f, 587.33f, 659.25f, 783.99f};
			Freq = Penta[MusicRng.RandRange(0, 4)] * (MusicRng.FRand() < 0.3f ? 2.f : 1.f);
		}
		else
		{
			Freq = MusicRng.FRandRange(300.f, 1400.f); // unstructured early-brain noise
		}
		AddTone(B, SR, T, 0.4f, Freq, 0.035f + 0.045f * M, 0.005f, 3.f);
	}

	// heartbeat-ish sub pulse appears with coherence
	if (M > 0.5f && (MusicChordStep % 2 == 0))
	{
		AddTone(B, SR, 0.f, 0.25f, 55.f, 0.06f * M, 0.01f, 2.5f);
		AddTone(B, SR, 0.9f, 0.25f, 55.f, 0.05f * M, 0.01f, 2.5f);
	}

	// edge fades prevent chunk-boundary clicks
	const int32 Fade = SR / 30;
	for (int32 i = 0; i < Fade && i < B.Num(); ++i)
	{
		const float G = (float)i / Fade;
		B[i] *= G;
		B[B.Num() - 1 - i] *= G;
	}

	TArray<int16> Pcm = Render(B);
	MusicWave->QueueAudio((const uint8*)Pcm.GetData(), Pcm.Num() * sizeof(int16));
}

void UForgeAudioManager::StopMusic()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}
}
