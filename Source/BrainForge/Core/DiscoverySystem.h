#pragma once

#include "CoreMinimal.h"
#include "ForgeTypes.h"

struct FBrainGraph;
struct FTrialResult;
struct FFitnessState;
struct FRandomStream;

/** A discovered rule of thumb (guide section 14). Grants a small real bonus so insight matters. */
struct BRAINFORGE_API FHeuristic
{
	int32 Id = 0;
	FString Text;
	float Confidence = 0.5f;
	int32 SampleSize = 1;
	float EfficiencyBonus = 0.f;   // additive to global efficiency modifier
	float StabilityBonus = 0.f;
	FString BonusLabel;            // "+9%"
};

/**
 * Tracks the player's scientific understanding, separate from brain performance
 * (guide section 13).
 */
struct BRAINFORGE_API FDiscoverySystem
{
	TArray<FHeuristic> Heuristics;
	int32 TrialTypeRuns[(int32)ETrialType::COUNT] = {};
	TSet<int32> TriggeredHeuristicIds;

	// action counters feeding heuristic triggers
	int32 PruneActions = 0;
	int32 ReinforceActions = 0;
	int32 LockActions = 0;
	int32 MutateActions = 0;
	float PrevStability = 0.7f;
	int32 StabilityRiseStreak = 0;

	bool TrialNameKnown(ETrialType Type) const { return TrialTypeRuns[(int32)Type] >= 5; }

	/** Overall player knowledge 0..1 (drives mutation preview accuracy). */
	float Knowledge() const;

	float TotalEfficiencyBonus() const;
	float TotalStabilityBonus() const;

	/**
	 * Called after every trial. Updates region confidence, reveals fitness dims,
	 * may trigger new heuristics. Returns display lines for anything discovered.
	 */
	TArray<FString> OnTrialComplete(FBrainGraph& Brain, const FTrialResult& Result, FFitnessState& Fitness, FRandomStream& Rng);
};
