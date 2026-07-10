#include "BrainGraph.h"
#include "Math/RandomStream.h"

const TCHAR* RegionRoleName(ERegionRole Role)
{
	switch (Role)
	{
	case ERegionRole::Vision:           return TEXT("Vision");
	case ERegionRole::Hearing:          return TEXT("Hearing");
	case ERegionRole::TouchPain:        return TEXT("Touch & Pain");
	case ERegionRole::Motor:            return TEXT("Motor Control");
	case ERegionRole::Balance:          return TEXT("Balance");
	case ERegionRole::HungerThirst:     return TEXT("Hunger & Thirst");
	case ERegionRole::ThreatDetection:  return TEXT("Threat Detection");
	case ERegionRole::Memory:           return TEXT("Memory");
	case ERegionRole::Language:         return TEXT("Language");
	case ERegionRole::Emotion:          return TEXT("Emotion");
	case ERegionRole::SocialCognition:  return TEXT("Social Cognition");
	case ERegionRole::Planning:         return TEXT("Planning");
	case ERegionRole::Attention:        return TEXT("Attention");
	case ERegionRole::RewardMotivation: return TEXT("Reward & Motivation");
	case ERegionRole::SelfPreservation: return TEXT("Self-Preservation");
	default:                            return TEXT("Unknown");
	}
}

const TCHAR* RegionRoleDescription(ERegionRole Role)
{
	switch (Role)
	{
	case ERegionRole::Vision:           return TEXT("Interprets luminance, motion and shape into obstacle, food and threat estimates.");
	case ERegionRole::Hearing:          return TEXT("Segments sound into direction, intensity, and voice-like patterns.");
	case ERegionRole::TouchPain:        return TEXT("Converts contact and tissue damage into pain and withdrawal signals.");
	case ERegionRole::Motor:            return TEXT("Drives limbs: locomotion, reach, grasp, withdrawal.");
	case ERegionRole::Balance:          return TEXT("Stabilizes posture against gravity and terrain.");
	case ERegionRole::HungerThirst:     return TEXT("Tracks internal energy and hydration, urges feeding and drinking.");
	case ERegionRole::ThreatDetection:  return TEXT("Fuses senses into fast danger estimates and alarm broadcasts.");
	case ERegionRole::Memory:           return TEXT("Stores associations, locations, and recent event traces.");
	case ERegionRole::Language:         return TEXT("Maps sound patterns to symbols and plans speech output.");
	case ERegionRole::Emotion:          return TEXT("Regulates arousal, fear, and contentment across the whole brain.");
	case ERegionRole::SocialCognition:  return TEXT("Models other minds: trust, cooperation, deception.");
	case ERegionRole::Planning:         return TEXT("Chains actions toward distant goals; simulates outcomes.");
	case ERegionRole::Attention:        return TEXT("Selects which signals are amplified and which are suppressed.");
	case ERegionRole::RewardMotivation: return TEXT("Broadcasts reinforcement, drives seeking behavior.");
	case ERegionRole::SelfPreservation: return TEXT("Overrides everything to keep the body alive.");
	default:                            return TEXT("");
	}
}

EForgePhase RegionUnlockPhase(ERegionRole Role)
{
	switch (Role)
	{
	case ERegionRole::Motor:
	case ERegionRole::TouchPain:
	case ERegionRole::RewardMotivation:
	case ERegionRole::HungerThirst:
		return EForgePhase::Reflex;

	case ERegionRole::Vision:
	case ERegionRole::Hearing:
	case ERegionRole::Balance:
	case ERegionRole::ThreatDetection:
		return EForgePhase::Perception;

	case ERegionRole::Memory:
	case ERegionRole::Attention:
		return EForgePhase::MemoryPhase;

	case ERegionRole::Emotion:
	case ERegionRole::SocialCognition:
		return EForgePhase::Social;

	case ERegionRole::Language:
		return EForgePhase::Language;

	case ERegionRole::Planning:
	case ERegionRole::SelfPreservation:
		return EForgePhase::Cognition;

	default:
		return EForgePhase::Reflex;
	}
}

bool RegionIsSensory(ERegionRole Role)
{
	switch (Role)
	{
	case ERegionRole::Vision:
	case ERegionRole::Hearing:
	case ERegionRole::TouchPain:
	case ERegionRole::Balance:
	case ERegionRole::HungerThirst:
		return true;
	default:
		return false;
	}
}

bool RegionIsMotor(ERegionRole Role)
{
	switch (Role)
	{
	case ERegionRole::Motor:
	case ERegionRole::Language:
	case ERegionRole::SocialCognition:
		return true;
	default:
		return false;
	}
}

FString FBrainRegion::DisplayName(bool bShort) const
{
	if (NameKnown())
	{
		if (FullyKnown() || bShort)
		{
			return RegionRoleName(Role);
		}
		return FString::Printf(TEXT("%s?"), RegionRoleName(Role));
	}
	return HiddenLabel;
}

FBrainRegion* FBrainGraph::FindRegion(int32 Id)
{
	for (FBrainRegion& R : Regions) { if (R.Id == Id) return &R; }
	return nullptr;
}

const FBrainRegion* FBrainGraph::FindRegion(int32 Id) const
{
	for (const FBrainRegion& R : Regions) { if (R.Id == Id) return &R; }
	return nullptr;
}

FBrainRegion* FBrainGraph::FindByRole(ERegionRole Role)
{
	for (FBrainRegion& R : Regions) { if (R.Role == Role) return &R; }
	return nullptr;
}

const FBrainRegion* FBrainGraph::FindByRole(ERegionRole Role) const
{
	for (const FBrainRegion& R : Regions) { if (R.Role == Role) return &R; }
	return nullptr;
}

FRegionLink* FBrainGraph::FindLink(int32 Id)
{
	for (FRegionLink& L : Links) { if (L.Id == Id) return &L; }
	return nullptr;
}

FRegionLink* FBrainGraph::AddLink(int32 Source, int32 Target, int32 SrcCh, int32 DstCh, float Weight)
{
	if (Source == Target || !FindRegion(Source) || !FindRegion(Target)) { return nullptr; }
	if (HasLink(Source, Target, SrcCh, DstCh)) { return nullptr; }

	FRegionLink L;
	L.Id = NextId++;
	L.Source = Source;
	L.Target = Target;
	L.SourceChannel = FMath::Clamp(SrcCh, 0, MaxChannels - 1);
	L.TargetChannel = FMath::Clamp(DstCh, 0, MaxChannels - 1);
	L.Weight = Weight;
	Links.Add(L);
	return &Links.Last();
}

bool FBrainGraph::RemoveLink(int32 Id)
{
	return Links.RemoveAll([Id](const FRegionLink& L) { return L.Id == Id; }) > 0;
}

bool FBrainGraph::HasLink(int32 Source, int32 Target, int32 SrcCh, int32 DstCh) const
{
	for (const FRegionLink& L : Links)
	{
		if (L.Source == Source && L.Target == Target && L.SourceChannel == SrcCh && L.TargetChannel == DstCh)
		{
			return true;
		}
	}
	return false;
}

void FBrainGraph::Tick(FRandomStream& Rng, int32 OpenRegionId)
{
	// 1. gather region inputs: external sensory + incoming links
	for (FBrainRegion& R : Regions)
	{
		if (!R.bAwake) { continue; }
		for (int32 c = 0; c < MaxChannels; ++c)
		{
			R.In[c] = RegionIsSensory(R.Role) ? SensoryIn[(int32)R.Role][c] : 0.f;
		}
	}
	for (FRegionLink& L : Links)
	{
		FBrainRegion* Src = FindRegion(L.Source);
		FBrainRegion* Dst = FindRegion(L.Target);
		if (!Src || !Dst || !Src->bAwake || !Dst->bAwake) { continue; }

		const float Signal = Src->Out[L.SourceChannel] * L.Weight;
		Dst->In[L.TargetChannel] = FMath::Clamp(Dst->In[L.TargetChannel] + Signal, -1.5f, 1.5f);
		L.LastSignal = Signal;
		L.Traffic = FMath::Lerp(L.Traffic, FMath::Abs(Signal), 0.06f);
	}

	// 2. tick each awake region (LOD: full sim only for the open one)
	for (FBrainRegion& R : Regions)
	{
		if (!R.bAwake)
		{
			R.Activation = 0.f;
			continue;
		}

		FMemory::Memcpy(R.Net.InputValues, R.In, sizeof(R.In));

		if (R.Id == OpenRegionId || R.Net.Compiled.Num() == 0)
		{
			R.Net.Tick(Rng);
		}
		else
		{
			R.Net.EvaluateCompiled(Rng);
		}

		FMemory::Memcpy(R.Out, R.Net.OutputValues, sizeof(R.Out));

		float Agg = 0.f;
		for (int32 c = 0; c < MaxChannels; ++c) { Agg += FMath::Abs(R.Out[c]); }
		R.Activation = FMath::Lerp(R.Activation, FMath::Clamp(Agg / MaxChannels * 1.6f, 0.f, 1.f), 0.25f);
		R.ActivityDuringTrial += R.Activation;
	}
}

void FBrainGraph::CompileDirty(FRandomStream& Rng, int32 OpenRegionId)
{
	for (FBrainRegion& R : Regions)
	{
		if (R.bAwake && R.Id != OpenRegionId && R.Net.bCompiledDirty)
		{
			R.Net.Compile(Rng);
		}
	}
}

float FBrainGraph::GlobalStability() const
{
	float Sum = 0.f;
	int32 Count = 0;
	for (const FBrainRegion& R : Regions)
	{
		if (R.bAwake) { Sum += R.Net.Stability; Count++; }
	}
	return Count > 0 ? Sum / Count : 0.7f;
}

float FBrainGraph::GlobalEnergyUse() const
{
	float Sum = 0.f;
	for (const FBrainRegion& R : Regions)
	{
		if (R.bAwake) { Sum += R.Net.EnergyUse; }
	}
	return Sum;
}

float FBrainGraph::SignalEfficiency() const
{
	// useful output per unit energy, squashed to 0..1
	float OutSum = 0.f;
	float Energy = 0.01f;
	int32 Count = 0;
	for (const FBrainRegion& R : Regions)
	{
		if (!R.bAwake) { continue; }
		for (int32 c = 0; c < MaxChannels; ++c) { OutSum += FMath::Abs(R.Out[c]); }
		Energy += R.Net.EnergyUse;
		Count++;
	}
	if (Count == 0) { return 0.5f; }
	const float Ratio = OutSum / Energy;
	return FMath::Clamp(Ratio / (Ratio + 1.2f) + 0.18f, 0.f, 0.99f);
}

int32 FBrainGraph::AwakeCount() const
{
	int32 N = 0;
	for (const FBrainRegion& R : Regions) { if (R.bAwake) { N++; } }
	return N;
}

void FBrainGraph::ResetTrialAccumulators()
{
	for (FBrainRegion& R : Regions) { R.ActivityDuringTrial = 0.f; }
}

void FBrainGraph::ResetAllState()
{
	for (FBrainRegion& R : Regions)
	{
		R.Net.ResetState();
		R.Activation = 0.f;
		for (int32 c = 0; c < MaxChannels; ++c) { R.In[c] = 0.f; R.Out[c] = 0.f; }
	}
	FMemory::Memzero(SensoryIn, sizeof(SensoryIn));
}

void BuildStarterBrain(FBrainGraph& Brain, FRandomStream& Rng)
{
	Brain.Regions.Reset();
	Brain.Links.Reset();
	Brain.NextId = 1;

	// brain-silhouette layout in a 1920x1080-ish virtual space (canvas pans/zooms anyway)
	struct FLayout { ERegionRole Role; FVector2D Pos; };
	const FLayout Layouts[] =
	{
		{ ERegionRole::Vision,           FVector2D(1560, 620) },  // occipital - back
		{ ERegionRole::Hearing,          FVector2D(1260, 760) },  // temporal
		{ ERegionRole::TouchPain,        FVector2D(1080, 330) },  // parietal
		{ ERegionRole::Motor,            FVector2D(830, 300) },   // motor strip
		{ ERegionRole::Balance,          FVector2D(1470, 880) },  // cerebellum
		{ ERegionRole::HungerThirst,     FVector2D(870, 700) },   // hypothalamus
		{ ERegionRole::ThreatDetection,  FVector2D(1010, 800) },  // amygdala
		{ ERegionRole::Memory,           FVector2D(1180, 560) },  // hippocampus
		{ ERegionRole::Language,         FVector2D(620, 640) },   // Broca-ish
		{ ERegionRole::Emotion,          FVector2D(760, 520) },   // limbic
		{ ERegionRole::SocialCognition,  FVector2D(430, 500) },   // prefrontal
		{ ERegionRole::Planning,         FVector2D(330, 340) },   // frontal pole
		{ ERegionRole::Attention,        FVector2D(950, 520) },   // thalamus - center hub
		{ ERegionRole::RewardMotivation, FVector2D(640, 380) },   // striatum
		{ ERegionRole::SelfPreservation, FVector2D(1250, 940) },  // brainstem
	};

	const TCHAR* HiddenNames[] =
	{
		TEXT("Region 01"), TEXT("Region 02"), TEXT("Region 03"), TEXT("Region 04"),
		TEXT("Signal Hub"), TEXT("Region 06"), TEXT("Feedback Core"), TEXT("Region 08"),
		TEXT("Unknown Integrator"), TEXT("Region 10"), TEXT("Region 11"), TEXT("Latent Cluster"),
		TEXT("Region 13"), TEXT("Reward Echo"), TEXT("Deep Structure")
	};

	int32 NameIdx = 0;
	for (const FLayout& L : Layouts)
	{
		FBrainRegion R;
		R.Id = Brain.NextId++;
		R.Role = L.Role;
		R.Pos = L.Pos + FVector2D(Rng.FRandRange(-24.f, 24.f), Rng.FRandRange(-18.f, 18.f));
		R.HiddenLabel = HiddenNames[NameIdx++ % 15];
		R.bAwake = (RegionUnlockPhase(L.Role) == EForgePhase::Reflex);

		const int32 Complexity = Rng.RandRange(6, 10);
		BuildStarterNetwork(R.Net, Rng, Complexity);

		Brain.Regions.Add(MoveTemp(R));
	}

	// initial inter-region wiring between awake regions (sparse, seeded)
	TArray<int32> AwakeIds;
	for (const FBrainRegion& R : Brain.Regions)
	{
		if (R.bAwake) { AwakeIds.Add(R.Id); }
	}
	const int32 StartLinks = 5;
	for (int32 i = 0; i < StartLinks; ++i)
	{
		const int32 A = AwakeIds[Rng.RandRange(0, AwakeIds.Num() - 1)];
		const int32 B = AwakeIds[Rng.RandRange(0, AwakeIds.Num() - 1)];
		if (A != B)
		{
			Brain.AddLink(A, B, Rng.RandRange(0, MaxChannels - 1), Rng.RandRange(0, MaxChannels - 1), Rng.FRandRange(0.4f, 0.9f));
		}
	}
}
