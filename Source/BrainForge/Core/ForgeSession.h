#pragma once

#include "CoreMinimal.h"
#include "ForgeTypes.h"
#include "BrainGraph.h"
#include "TrialSystem.h"
#include "MutationEngine.h"
#include "DiscoverySystem.h"
#include "PatternLibrary.h"

/** Something the UI/audio layer should react to. Drained once per frame. */
struct BRAINFORGE_API FForgeEvent
{
	EForgeSound Sound = EForgeSound::Click;
	FString Text;              // empty = sound only, no toast
	bool bImportant = false;   // important toasts linger longer
};

/** One selectable reward offered at the end of a generation (Mini Motorways-style pick). */
struct BRAINFORGE_API FGenerationOffer
{
	int32 Id = 0;
	FString Title;
	FString Desc;
};

/** Energy costs for player actions. */
namespace ForgeCost
{
	constexpr float AddNode = 25.f;
	constexpr float Connect = 10.f;
	constexpr float Disconnect = 5.f;
	constexpr float Mutate = 50.f;
	constexpr float MutateCluster = 100.f;
	constexpr float Prune = 15.f;
	constexpr float RewardPulse = 30.f;
	constexpr float LockPattern = 20.f;
	constexpr float Randomize = 40.f;
	constexpr float AwakenRegion = 150.f;
	constexpr float InsertPattern = 35.f;
	constexpr float Boost = 15.f;
	constexpr float Dampen = 15.f;
	constexpr float DuplicateMotif = 60.f;
	constexpr float AutoTest = 25.f;
	constexpr float RunHundred = 100.f;
}

/**
 * A full play-through: seeded brain + fitness + trial flow + economy + progression.
 * The game loop feel is Mini Motorways: trials stream continuously while the player
 * calmly reshapes the network; every generation offers a pick-one upgrade.
 */
class BRAINFORGE_API FForgeSession
{
public:
	// ---- identity
	int32 Seed = 0;
	FString LineageName = TEXT("Lineage Alpha");
	FRandomStream Rng;

	// ---- world
	FBrainGraph Brain;
	FFitnessState Fitness;
	FDiscoverySystem Discovery;
	FPatternLibrary Patterns;
	FTrialRunner Runner;

	// ---- progression
	EForgePhase Phase = EForgePhase::Reflex;
	int32 Generation = 1;
	int32 TrialCounter = 0;
	int32 TrialsThisGeneration = 0;
	static constexpr int32 TrialsPerGeneration = 25;

	// ---- economy
	float Energy = 250.f;
	float EnergyCap = 500.f;

	// ---- score juice
	float FitnessScore = 0.f;      // cumulative points (top bar big number)
	float BestRun = 0.f;           // best single-trial score points
	float ScoreRate = 0.f;         // points/sec EMA for "+18.6/s"

	// ---- pacing
	int32 Speed = 1;               // 0 pause, 1, 2, 4, 8
	bool bAutoRun = true;
	int32 BatchRemaining = 0;      // "Run 100 Trials"
	int32 OpenRegionId = INDEX_NONE; // INDEX_NONE = whole-brain view

	// ---- modifiers from generation rewards / heuristics
	float LearningRateMult = 1.f;
	int32 LearningBoostTrialsLeft = 0;
	bool bFreeAwaken = false;
	float PermStabilityBonus = 0.f;

	// ---- state flags
	bool bWon = false;
	bool bEverWon = false;
	TArray<FGenerationOffer> PendingOffers;   // non-empty = modal reward choice up
	int32 LowStabilityStreak = 0;

	// ---- history (charts + archive)
	TArray<FTrialResult> History;             // capped
	TArray<float> FitnessCurve;               // one point per trial (display %)
	TArray<float> RecentScores;               // last 24 trial scores (bar chart)
	TArray<FString> ArchiveLog;               // discoveries, mutations, failures
	int32 MutationCount = 0;
	int32 PruneCount = 0;

	// ---- event queue for UI/audio
	TArray<FForgeEvent> EventQueue;

	// =======================================================================

	void NewGame(int32 InSeed, const FString& InLineageName);

	/** Main update called from the HUD every frame. */
	void Update(float DeltaSeconds);

	float DisplayFitness() const;    // 0..100 (%)
	bool CanAfford(float Cost) const { return Energy >= Cost; }
	float Knowledge() const { return Discovery.Knowledge(); }

	/** Network currently being edited (open region's net), or null in whole-brain view. */
	FNeuralNetwork* OpenNetwork();
	FBrainRegion* OpenRegion();

	// ---- player actions (all return false if not affordable / not valid)
	bool ActionRunBatch();
	bool ActionRandomize();
	bool ActionMutate(bool bCluster);
	bool ActionPrune();
	bool ActionRewardPulse();
	bool ActionLock(const TArray<int32>& NodeIds, const TArray<int32>& ConnIds);
	bool ActionAddNode(ENodeType Type, const FVector2D& Pos);
	bool ActionConnect(int32 SrcNodeId, int32 DstNodeId, EConnType Type);
	bool ActionDisconnect(int32 ConnId);
	bool ActionBoost(int32 ConnId, float Delta);
	bool ActionDuplicateMotif();
	bool ActionAutoTest();
	bool ActionAwakenRegion(int32 RegionId);
	bool ActionSavePattern(const TArray<int32>& NodeIds);
	bool ActionInsertPattern(int32 PatternId, const FVector2D& Pos);
	bool ActionConnectRegions(int32 SrcRegionId, int32 DstRegionId);
	bool ActionDisconnectRegionLink(int32 LinkId);
	void ChooseGenerationReward(int32 OfferId);

	void EnterRegion(int32 RegionId);
	void ExitRegion();

	void PushEvent(EForgeSound Sound, const FString& Text = FString(), bool bImportant = false);

	// preview for the mutate button tooltip
	FMutationPreview GetMutationPreview();

private:
	void StartNextTrial();
	void FinishTrial();
	void MakeGenerationOffers();
	void CheckPhaseAdvance();
	bool Spend(float Cost);

	float TickAccumulator = 0.f;
	float InterTrialPause = 0.f;
	float ScoreRateAccum = 0.f;
	float ScoreRateTimer = 0.f;
};
