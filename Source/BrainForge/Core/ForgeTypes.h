#pragma once

#include "CoreMinimal.h"

/** Node archetypes available inside subnetworks. Mirrors the sprite sheet legend. */
enum class ENodeType : uint8
{
	Input,      // receives an aggregate input channel of the region
	Relay,      // passes signal (linear)
	Threshold,  // step/sigmoid gate
	Memory,     // slow accumulator with long decay
	Oscillator, // rhythmic self-activation
	Inhibitor,  // outputs negative signal
	Reward,     // broadcasts plasticity boost when active
	Output,     // drives an aggregate output channel of the region
	COUNT
};

/** Connection behavior types (guide section 5.4). */
enum class EConnType : uint8
{
	Excitatory,
	Inhibitory,
	Modulatory,  // scales target plasticity instead of activation
	Memory,      // very slow decay of transmitted signal
	Unstable,    // randomly drops or spikes
	Delayed,     // long transmission delay
	COUNT
};

/** Hidden fitness dimensions combined by geometric mean (guide section 11). */
enum class EFitnessDim : uint8
{
	Survival,
	Perception,
	Mobility,
	Regulation,
	Memory,
	Communication,
	Social,
	Planning,
	Adaptability,
	COUNT
};

/** Progression phases (guide section 24). */
enum class EForgePhase : uint8
{
	Reflex = 0,
	Perception,
	MemoryPhase,
	Social,
	Language,
	Cognition,
	COUNT
};

/** Trial scenario categories (guide sections 9 / 29). */
enum class ETrialType : uint8
{
	ApproachReward,
	AvoidThreat,
	MaintainEnergy,
	NavigateObstacle,
	RememberDirection,
	FindWarmth,
	RecognizeFood,
	EscapePredator,
	InterpretVoice,
	SocialBond,
	UseTool,
	PlanWinter,
	COUNT
};

/** All synthesized sound cues. */
enum class EForgeSound : uint8
{
	Click,          // soft digital click - input activation / UI press
	Hover,          // faint tick
	Connect,        // electric snap + spark
	Disconnect,     // reverse snap
	RewardChime,    // warm harmonic chime
	Inhibit,        // muted low pulse
	Crackle,        // unstable connection electrical crackle
	MemoryEcho,     // repeating echo blip
	Mutate,         // rising distortion tone
	TrialWin,       // subtle layered chord
	TrialSoft,      // neutral trial end tick-chord
	Failure,        // signal collapse + low drop
	Discovery,      // resolving crystalline tone
	Generation,     // gentle fanfare pad
	Prune,          // snip
	Lock,           // solid clunk-chime
	Awaken,         // deep bloom swell
	EnergyLow,      // warning blip
	Victory,        // full harmonic resolve
	COUNT
};

FORCEINLINE int32 FitnessDimCount() { return (int32)EFitnessDim::COUNT; }

BRAINFORGE_API const TCHAR* FitnessDimName(EFitnessDim Dim);
BRAINFORGE_API const TCHAR* PhaseName(EForgePhase Phase);
BRAINFORGE_API const TCHAR* NodeTypeName(ENodeType Type);
BRAINFORGE_API const TCHAR* ConnTypeName(EConnType Type);
