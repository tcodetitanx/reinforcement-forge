#pragma once

#include "CoreMinimal.h"
#include "ForgeTypes.h"
#include "BrainGraph.h"

/**
 * Multi-dimensional hidden fitness (guide sections 2.4 / 11).
 * Overall = geometric mean of dims * stability * efficiency.
 */
struct BRAINFORGE_API FFitnessState
{
	float Dims[(int32)EFitnessDim::COUNT];
	float StabilityMod = 0.7f;
	float EfficiencyMod = 0.6f;

	// which dims the player has identified (guide section 12)
	bool DimKnown[(int32)EFitnessDim::COUNT];

	FFitnessState();

	float Overall() const;                 // 0..1
	float OverallPercent() const { return Overall() * 100.f; }
	void ApplyTrialScore(EFitnessDim Dim, float Score01, float Weight, float LearningRate);
	int32 KnownDimCount() const;
	int32 HiddenDimCount() const { return FitnessDimCount() - KnownDimCount(); }
};

/** Symbolic hidden human agent (guide section 10). */
struct BRAINFORGE_API FHiddenAgent
{
	float Health = 1.f;
	float Energy = 1.f;
	float BodyTemp = 0.5f;      // 0 freezing .. 1 overheating, 0.5 ideal
	float Hunger = 0.2f;
	float Fear = 0.f;
	float Pain = 0.f;
	float PosX = 0.f;           // -1..1 lane
	float VelX = 0.f;
	float Balance = 1.f;
	float MemoryTrace = 0.f;    // how strongly the agent held the cue
	float SocialBond = 0.f;
	float Comprehension = 0.f;

	// per-trial accumulators
	float EnergySpent = 0.f;
	float PainTaken = 0.f;
	int32 FirstReactionTick = -1;

	void Reset(FRandomStream& Rng);
};

/** One completed trial (guide sections 9 / 34). */
struct BRAINFORGE_API FTrialResult
{
	int32 TrialNumber = 0;
	ETrialType Type = ETrialType::ApproachReward;
	bool bNameKnown = false;

	float Score01 = 0.f;                  // trial performance
	float FitnessBefore = 0.f;            // overall %
	float FitnessAfter = 0.f;
	float StabilityDelta = 0.f;
	float EnergyDeltaPct = 0.f;           // brain energy use change

	TArray<FString> DiscoveryLines;       // new heuristics / reveals earned this trial
	TArray<int32> DominantRegions;        // region ids that carried the trial

	FString VisibleSummary() const;
};

/** Static definition of a trial category. */
struct BRAINFORGE_API FTrialDef
{
	ETrialType Type = ETrialType::ApproachReward;
	const TCHAR* RealName = TEXT("");
	const TCHAR* HiddenName = TEXT("");
	const TCHAR* Description = TEXT("");
	EForgePhase PhaseReq = EForgePhase::Reflex;
	int32 DurationTicks = 160;
	// primary fitness dimensions trained by this trial
	EFitnessDim PrimaryDim = EFitnessDim::Survival;
	EFitnessDim SecondaryDim = EFitnessDim::Mobility;
};

BRAINFORGE_API const FTrialDef& GetTrialDef(ETrialType Type);
BRAINFORGE_API TArray<ETrialType> TrialsForPhase(EForgePhase Phase);

/**
 * Runs one trial tick-by-tick so the UI can animate the brain while it happens.
 * Create with Begin(), call Step() until done, then Finalize().
 */
class BRAINFORGE_API FTrialRunner
{
public:
	void Begin(ETrialType Type, int32 TrialNumber, FBrainGraph& Brain, FRandomStream& Rng);
	/** Advance one sim tick. Returns false when the trial has consumed its duration. */
	bool Step(FBrainGraph& Brain, FRandomStream& Rng, int32 OpenRegionId);
	/** Compute the result. Call once after Step returns false. */
	FTrialResult Finalize(FBrainGraph& Brain, FFitnessState& Fitness, FRandomStream& Rng);

	bool IsActive() const { return bActive; }
	float Progress() const { return Def ? FMath::Clamp((float)Tick / Def->DurationTicks, 0.f, 1.f) : 0.f; }
	ETrialType CurrentType() const { return Type; }
	int32 CurrentTick() const { return Tick; }

	const FHiddenAgent& Agent() const { return AgentState; }

private:
	void WriteSensoryInputs(FBrainGraph& Brain, FRandomStream& Rng);
	void ApplyMotorOutputs(FBrainGraph& Brain, FRandomStream& Rng);
	float EvaluateScore(FBrainGraph& Brain) const;

	bool bActive = false;
	ETrialType Type = ETrialType::ApproachReward;
	const FTrialDef* Def = nullptr;
	int32 Tick = 0;
	int32 TrialNumber = 0;

	FHiddenAgent AgentState;

	// scenario variables
	float TargetX = 0.6f;        // reward / threat / warmth position
	float ThreatX = -0.6f;
	float CueDirection = 1.f;    // remembered direction for memory trials
	int32 CueEndTick = 40;
	float VoicePattern = 0.5f;   // pattern id the agent must echo
	float Accumulated = 0.f;     // generic per-scenario accumulator
	float ReachedAt = -1.f;
};
