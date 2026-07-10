#include "DiscoverySystem.h"
#include "BrainGraph.h"
#include "TrialSystem.h"
#include "Math/RandomStream.h"

float FDiscoverySystem::Knowledge() const
{
	int32 NamedTrials = 0;
	for (int32 i = 0; i < (int32)ETrialType::COUNT; ++i)
	{
		if (TrialTypeRuns[i] >= 5) { NamedTrials++; }
	}
	const float FromTrials = (float)NamedTrials / (int32)ETrialType::COUNT;
	const float FromHeuristics = FMath::Min(1.f, Heuristics.Num() / 12.f);
	return FMath::Clamp(FromTrials * 0.5f + FromHeuristics * 0.5f, 0.f, 1.f);
}

float FDiscoverySystem::TotalEfficiencyBonus() const
{
	float Sum = 0.f;
	for (const FHeuristic& H : Heuristics) { Sum += H.EfficiencyBonus; }
	return Sum;
}

float FDiscoverySystem::TotalStabilityBonus() const
{
	float Sum = 0.f;
	for (const FHeuristic& H : Heuristics) { Sum += H.StabilityBonus; }
	return Sum;
}

TArray<FString> FDiscoverySystem::OnTrialComplete(FBrainGraph& Brain, const FTrialResult& Result, FFitnessState& Fitness, FRandomStream& Rng)
{
	TArray<FString> Lines;

	// ---- trial name discovery
	int32& Runs = TrialTypeRuns[(int32)Result.Type];
	Runs++;
	if (Runs == 5)
	{
		const FTrialDef& D = GetTrialDef(Result.Type);
		Lines.Add(FString::Printf(TEXT("Probable scenario identified: %s (confidence %d%%)"), D.RealName, Rng.RandRange(61, 82)));
	}

	// ---- region function discovery: dominant regions during scoring trials gain confidence
	for (int32 RegionId : Result.DominantRegions)
	{
		if (FBrainRegion* R = Brain.FindRegion(RegionId))
		{
			const float Before = R->DiscoveryConfidence;
			const float Gain = 0.02f + 0.05f * Result.Score01;
			R->DiscoveryConfidence = FMath::Min(1.f, R->DiscoveryConfidence + Gain);

			if (Before < 0.5f && R->DiscoveryConfidence >= 0.5f)
			{
				Lines.Add(FString::Printf(TEXT("%s -> probable function: %s (confidence %d%%)"),
					*R->HiddenLabel, RegionRoleName(R->Role), FMath::RoundToInt(R->DiscoveryConfidence * 100)));
			}
			else if (Before < 0.8f && R->DiscoveryConfidence >= 0.8f)
			{
				Lines.Add(FString::Printf(TEXT("Function confirmed: %s"), RegionRoleName(R->Role)));
			}
		}
	}

	// ---- fitness dimension reveal (guide section 12)
	const FTrialDef& Def = GetTrialDef(Result.Type);
	const int32 PrimIdx = (int32)Def.PrimaryDim;
	if (!Fitness.DimKnown[PrimIdx] && Fitness.Dims[PrimIdx] > 0.15f)
	{
		Fitness.DimKnown[PrimIdx] = true;
		Lines.Add(FString::Printf(TEXT("Fitness contributor identified: %s"), FitnessDimName(Def.PrimaryDim)));
	}

	// ---- stability streak tracking
	const float Stab = Brain.GlobalStability();
	StabilityRiseStreak = (Stab > PrevStability + 0.002f) ? StabilityRiseStreak + 1 : 0;
	PrevStability = Stab;

	// ---- heuristic triggers (each fires once)
	auto TryAdd = [&](int32 Id, bool bCondition, const TCHAR* Text, float EffBonus, float StabBonus, const TCHAR* Label)
	{
		if (!bCondition || TriggeredHeuristicIds.Contains(Id)) { return; }
		// discovery is probabilistic - insight arrives, it isn't dispensed
		if (Rng.FRand() > 0.45f) { return; }
		TriggeredHeuristicIds.Add(Id);
		FHeuristic H;
		H.Id = Id;
		H.Text = Text;
		H.Confidence = Rng.FRandRange(0.55f, 0.9f);
		H.SampleSize = Rng.RandRange(6, 30);
		H.EfficiencyBonus = EffBonus;
		H.StabilityBonus = StabBonus;
		H.BonusLabel = Label;
		Heuristics.Add(H);
		Lines.Add(FString::Printf(TEXT("New heuristic: %s  [%s]"), Text, Label));
	};

	const float Efficiency = Brain.SignalEfficiency();

	TryAdd(1, PruneActions >= 3 && StabilityRiseStreak >= 2,
		TEXT("Loop suppression improves stability"), 0.f, 0.03f, TEXT("+9% stability"));
	TryAdd(2, ReinforceActions >= 4 && Fitness.Dims[(int32)EFitnessDim::Survival] > 0.2f,
		TEXT("Reward relay chains amplify learning"), 0.04f, 0.f, TEXT("+12% efficiency"));
	TryAdd(3, Efficiency > 0.55f,
		TEXT("Threshold gates prevent signal waste"), 0.027f, 0.f, TEXT("+8% efficiency"));
	TryAdd(4, Result.Score01 > 0.6f && Result.Type == ETrialType::ApproachReward,
		TEXT("Shorter signal paths increase efficiency"), 0.02f, 0.f, TEXT("+6% efficiency"));
	TryAdd(5, Stab < 0.35f,
		TEXT("Excessive feedback causes seizure-like instability"), 0.f, 0.02f, TEXT("insight"));
	TryAdd(6, Fitness.Dims[(int32)EFitnessDim::Mobility] > 0.3f,
		TEXT("This motor cluster fires before successful movement"), 0.015f, 0.f, TEXT("+4% efficiency"));
	TryAdd(7, LockActions >= 2,
		TEXT("Locked patterns resist mutation damage"), 0.f, 0.025f, TEXT("+7% stability"));
	TryAdd(8, MutateActions >= 5 && Result.Score01 > 0.5f,
		TEXT("Most mutations fail; the survivors compound"), 0.02f, 0.f, TEXT("+6% efficiency"));
	TryAdd(9, Fitness.Dims[(int32)EFitnessDim::Memory] > 0.25f,
		TEXT("Memory charge persists between trials"), 0.f, 0.02f, TEXT("+5% stability"));
	TryAdd(10, Fitness.Dims[(int32)EFitnessDim::Social] > 0.25f,
		TEXT("High social-region activation correlates with group survival"), 0.02f, 0.f, TEXT("+6% efficiency"));
	TryAdd(11, Result.Type == ETrialType::MaintainEnergy && Result.Score01 > 0.55f,
		TEXT("A quiet brain is a cheap brain"), 0.03f, 0.f, TEXT("+9% efficiency"));
	TryAdd(12, Brain.AwakeCount() >= 8,
		TEXT("Hub regions reduce total wiring length"), 0.02f, 0.01f, TEXT("+5% both"));
	TryAdd(13, Result.Type == ETrialType::RememberDirection && Result.Score01 > 0.5f,
		TEXT("Oscillator loops can hold a direction"), 0.f, 0.02f, TEXT("+5% stability"));
	TryAdd(14, Fitness.OverallPercent() > 40.f,
		TEXT("Balanced systems outperform specialists"), 0.02f, 0.02f, TEXT("+6% both"));

	return Lines;
}
