#pragma once

#include "CoreMinimal.h"
#include "ForgeTypes.h"
#include "NeuralNetwork.h"

/** Functional roles the 15 major regions play in trials (guide section 5.1). */
enum class ERegionRole : uint8
{
	Vision,
	Hearing,
	TouchPain,
	Motor,
	Balance,
	HungerThirst,
	ThreatDetection,
	Memory,
	Language,
	Emotion,
	SocialCognition,
	Planning,
	Attention,
	RewardMotivation,
	SelfPreservation,
	COUNT
};

BRAINFORGE_API const TCHAR* RegionRoleName(ERegionRole Role);
BRAINFORGE_API const TCHAR* RegionRoleDescription(ERegionRole Role);
BRAINFORGE_API EForgePhase RegionUnlockPhase(ERegionRole Role);
BRAINFORGE_API bool RegionIsSensory(ERegionRole Role);
BRAINFORGE_API bool RegionIsMotor(ERegionRole Role);

/** A major brain region: a node in the whole-brain graph AND a full internal network. */
struct BRAINFORGE_API FBrainRegion
{
	int32 Id = 0;
	ERegionRole Role = ERegionRole::Motor;
	FString HiddenLabel;          // "Region 07", "Signal Hub"...
	FVector2D Pos = FVector2D::ZeroVector;

	bool bAwake = false;          // dormant regions do nothing and appear dark
	float DiscoveryConfidence = 0.f; // 0..1, name reveals at 0.5, description at 0.8

	FNeuralNetwork Net;

	// aggregate runtime values exposed to the parent graph
	float In[MaxChannels] = {0, 0, 0, 0};
	float Out[MaxChannels] = {0, 0, 0, 0};
	float Activation = 0.f;       // aggregate for viz
	float ActivityDuringTrial = 0.f; // accumulated, reset per trial (discovery)

	FString DisplayName(bool bShort = false) const;
	bool NameKnown() const { return DiscoveryConfidence >= 0.5f; }
	bool FullyKnown() const { return DiscoveryConfidence >= 0.8f; }
};

/** Connection between two regions in the whole-brain graph. */
struct BRAINFORGE_API FRegionLink
{
	int32 Id = 0;
	int32 Source = 0;             // region id
	int32 Target = 0;
	int32 SourceChannel = 0;      // 0..3 output channel of source
	int32 TargetChannel = 0;      // 0..3 input channel of target
	float Weight = 0.7f;
	float LastSignal = 0.f;
	float Traffic = 0.f;
	bool bLocked = false;
};

/**
 * The whole-brain graph: ~15 regions plus inter-region links.
 * Uses level-of-detail simulation (guide section 7): the open region runs its full
 * network, closed regions evaluate their compiled response tables.
 */
struct BRAINFORGE_API FBrainGraph
{
	int32 NextId = 1;
	TArray<FBrainRegion> Regions;
	TArray<FRegionLink> Links;

	// external sensory inputs written by the trial runner, keyed by sensory role
	float SensoryIn[(int32)ERegionRole::COUNT][MaxChannels] = {};

	FBrainRegion* FindRegion(int32 Id);
	const FBrainRegion* FindRegion(int32 Id) const;
	FBrainRegion* FindByRole(ERegionRole Role);
	const FBrainRegion* FindByRole(ERegionRole Role) const;
	FRegionLink* FindLink(int32 Id);

	FRegionLink* AddLink(int32 Source, int32 Target, int32 SrcCh, int32 DstCh, float Weight);
	bool RemoveLink(int32 Id);
	bool HasLink(int32 Source, int32 Target, int32 SrcCh, int32 DstCh) const;

	/** Advance one sim tick. OpenRegionId's network is fully simulated; others use compiled models. */
	void Tick(FRandomStream& Rng, int32 OpenRegionId);

	/** Recompile any dirty closed networks (call between trials). */
	void CompileDirty(FRandomStream& Rng, int32 OpenRegionId);

	/** Aggregate brain stats. */
	float GlobalStability() const;
	float GlobalEnergyUse() const;
	float SignalEfficiency() const;

	int32 AwakeCount() const;

	void ResetTrialAccumulators();
	void ResetAllState();
};

/** Create a fresh seeded brain with all 15 regions laid out like a brain silhouette. */
BRAINFORGE_API void BuildStarterBrain(FBrainGraph& Brain, FRandomStream& Rng);
