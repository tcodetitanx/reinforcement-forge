#include "TrialSystem.h"
#include "Math/RandomStream.h"

// ---------------------------------------------------------------- fitness

FFitnessState::FFitnessState()
{
	for (int32 i = 0; i < FitnessDimCount(); ++i)
	{
		Dims[i] = 0.03f;
		DimKnown[i] = false;
	}
}

float FFitnessState::Overall() const
{
	// geometric mean prevents ignoring critical systems (guide section 11)
	double Product = 1.0;
	for (int32 i = 0; i < FitnessDimCount(); ++i)
	{
		Product *= FMath::Max(0.01f, Dims[i]);
	}
	const float GeoMean = (float)FMath::Pow(Product, 1.0 / FitnessDimCount());
	const float Stab = 0.55f + 0.45f * StabilityMod;
	const float Eff = 0.65f + 0.35f * EfficiencyMod;
	return FMath::Clamp(GeoMean * Stab * Eff, 0.f, 1.f);
}

void FFitnessState::ApplyTrialScore(EFitnessDim Dim, float Score01, float Weight, float LearningRate)
{
	float& D = Dims[(int32)Dim];
	const float Target = FMath::Clamp(Score01, 0.f, 1.f);
	D = FMath::Clamp(D + (Target - D) * LearningRate * Weight, 0.01f, 1.f);
}

int32 FFitnessState::KnownDimCount() const
{
	int32 N = 0;
	for (int32 i = 0; i < FitnessDimCount(); ++i) { if (DimKnown[i]) { N++; } }
	return N;
}

// ---------------------------------------------------------------- agent

void FHiddenAgent::Reset(FRandomStream& Rng)
{
	*this = FHiddenAgent();
	PosX = Rng.FRandRange(-0.25f, 0.25f);
	Hunger = Rng.FRandRange(0.1f, 0.4f);
	BodyTemp = Rng.FRandRange(0.4f, 0.6f);
}

// ---------------------------------------------------------------- defs

static const FTrialDef GTrialDefs[] =
{
	{ ETrialType::ApproachReward,    TEXT("Approach Reward"),      TEXT("Unknown Trial 03"), TEXT("A rewarding stimulus appears nearby. Move toward it and act."),          EForgePhase::Reflex,      140, EFitnessDim::Survival,      EFitnessDim::Mobility },
	{ ETrialType::AvoidThreat,       TEXT("Avoid Threat"),         TEXT("Unknown Trial 07"), TEXT("A damaging stimulus approaches. Withdraw before contact."),               EForgePhase::Reflex,      140, EFitnessDim::Survival,      EFitnessDim::Mobility },
	{ ETrialType::MaintainEnergy,    TEXT("Maintain Energy"),      TEXT("Unknown Trial 11"), TEXT("Resources are scarce. Spend as little as possible while staying responsive."), EForgePhase::Reflex,  160, EFitnessDim::Regulation,    EFitnessDim::Survival },
	{ ETrialType::NavigateObstacle,  TEXT("Navigate Obstacle"),    TEXT("Unknown Trial 15"), TEXT("An obstruction blocks the path. Steer around it without falling."),        EForgePhase::Perception,  170, EFitnessDim::Mobility,      EFitnessDim::Perception },
	{ ETrialType::RememberDirection, TEXT("Remember Direction"),   TEXT("Unknown Trial 19"), TEXT("A cue flashes, then vanishes. Later, move the way it pointed."),          EForgePhase::MemoryPhase, 190, EFitnessDim::Memory,        EFitnessDim::Planning },
	{ ETrialType::FindWarmth,        TEXT("Find Warmth"),          TEXT("Unknown Trial 22"), TEXT("Body temperature is falling. Locate and reach the warm zone."),           EForgePhase::Reflex,      160, EFitnessDim::Regulation,    EFitnessDim::Perception },
	{ ETrialType::RecognizeFood,     TEXT("Recognize Food"),       TEXT("Unknown Trial 26"), TEXT("Two objects: one nourishing, one harmful. Consume the right one."),       EForgePhase::Perception,  160, EFitnessDim::Perception,    EFitnessDim::Regulation },
	{ ETrialType::EscapePredator,    TEXT("Escape Predator"),      TEXT("Unknown Trial 31"), TEXT("A predator locks on. Sprint, evade, survive."),                           EForgePhase::Perception,  180, EFitnessDim::Survival,      EFitnessDim::Perception },
	{ ETrialType::InterpretVoice,    TEXT("Interpret a Voice"),    TEXT("Unknown Trial 35"), TEXT("A structured sound arrives. Respond with a matching pattern."),           EForgePhase::Language,    180, EFitnessDim::Communication, EFitnessDim::Memory },
	{ ETrialType::SocialBond,        TEXT("Respond to Another"),   TEXT("Unknown Trial 38"), TEXT("Another mind signals distress or greeting. React appropriately."),        EForgePhase::Social,      170, EFitnessDim::Social,        EFitnessDim::Communication },
	{ ETrialType::UseTool,           TEXT("Use a Simple Tool"),    TEXT("Unknown Trial 42"), TEXT("The reward is out of reach. Something nearby can extend your grasp."),    EForgePhase::Cognition,   200, EFitnessDim::Planning,      EFitnessDim::Mobility },
	{ ETrialType::PlanWinter,        TEXT("Plan Through Winter"),  TEXT("Unknown Trial 47"), TEXT("Cold is coming. Store energy now or starve later."),                      EForgePhase::Cognition,   220, EFitnessDim::Planning,      EFitnessDim::Regulation },
};
static_assert(UE_ARRAY_COUNT(GTrialDefs) == (int32)ETrialType::COUNT, "Trial def table mismatch");

const FTrialDef& GetTrialDef(ETrialType Type)
{
	return GTrialDefs[(int32)Type];
}

TArray<ETrialType> TrialsForPhase(EForgePhase Phase)
{
	TArray<ETrialType> Result;
	for (const FTrialDef& D : GTrialDefs)
	{
		if ((int32)D.PhaseReq <= (int32)Phase)
		{
			Result.Add(D.Type);
		}
	}
	return Result;
}

FString FTrialResult::VisibleSummary() const
{
	const FTrialDef& D = GetTrialDef(Type);
	const FString Name = bNameKnown ? D.RealName : D.HiddenName;
	const float Delta = FitnessAfter - FitnessBefore;
	return FString::Printf(TEXT("%s  |  Fitness %+.2f%%"), *Name, Delta);
}

// ---------------------------------------------------------------- runner

void FTrialRunner::Begin(ETrialType InType, int32 InTrialNumber, FBrainGraph& Brain, FRandomStream& Rng)
{
	bActive = true;
	Type = InType;
	Def = &GetTrialDef(InType);
	Tick = 0;
	TrialNumber = InTrialNumber;
	Accumulated = 0.f;
	ReachedAt = -1.f;

	AgentState.Reset(Rng);
	Brain.ResetTrialAccumulators();
	FMemory::Memzero(Brain.SensoryIn, sizeof(Brain.SensoryIn));

	TargetX = Rng.FRand() < 0.5f ? Rng.FRandRange(-0.9f, -0.45f) : Rng.FRandRange(0.45f, 0.9f);
	ThreatX = -TargetX + Rng.FRandRange(-0.2f, 0.2f);
	CueDirection = Rng.FRand() < 0.5f ? -1.f : 1.f;
	CueEndTick = Def->DurationTicks / 5;
	VoicePattern = Rng.FRandRange(0.3f, 0.9f);

	switch (Type)
	{
	case ETrialType::MaintainEnergy: AgentState.Energy = 0.55f; break;
	case ETrialType::FindWarmth:     AgentState.BodyTemp = 0.30f; break;
	case ETrialType::PlanWinter:     AgentState.Energy = 0.8f; break;
	case ETrialType::EscapePredator: AgentState.Fear = 0.4f; break;
	default: break;
	}
}

void FTrialRunner::WriteSensoryInputs(FBrainGraph& Brain, FRandomStream& Rng)
{
	const float T = (float)Tick / Def->DurationTicks;
	auto Sense = [&Brain](ERegionRole Role, int32 Ch, float V)
	{
		Brain.SensoryIn[(int32)Role][Ch] = FMath::Clamp(V, -1.f, 1.f);
	};

	// baseline interoception always present
	Sense(ERegionRole::HungerThirst, 0, AgentState.Hunger);
	Sense(ERegionRole::HungerThirst, 1, 1.f - AgentState.Energy);
	Sense(ERegionRole::HungerThirst, 2, FMath::Abs(AgentState.BodyTemp - 0.5f) * 2.f);
	Sense(ERegionRole::TouchPain, 0, AgentState.Pain);
	Sense(ERegionRole::Balance, 0, 1.f - AgentState.Balance);
	Sense(ERegionRole::Balance, 1, FMath::Clamp(FMath::Abs(AgentState.VelX) * 2.f, 0.f, 1.f));

	const float RelTarget = TargetX - AgentState.PosX;   // + means target to the right
	const float RelThreat = ThreatX - AgentState.PosX;

	switch (Type)
	{
	case ETrialType::ApproachReward:
	case ETrialType::RecognizeFood:
	case ETrialType::UseTool:
		Sense(ERegionRole::Vision, 0, FMath::Max(0.f, RelTarget));          // target right
		Sense(ERegionRole::Vision, 1, FMath::Max(0.f, -RelTarget));         // target left
		Sense(ERegionRole::Vision, 2, 1.f - FMath::Abs(RelTarget));         // proximity
		if (Type == ETrialType::RecognizeFood)
		{
			Sense(ERegionRole::Vision, 3, VoicePattern);                     // texture cue: >0.6 = safe
			Sense(ERegionRole::HungerThirst, 3, FMath::Max(0.f, RelThreat)); // decoy scent
		}
		break;

	case ETrialType::AvoidThreat:
	case ETrialType::EscapePredator:
	{
		// threat approaches the agent over time
		const float Chase = (Type == ETrialType::EscapePredator) ? 0.55f : 0.25f;
		ThreatX = FMath::Lerp(ThreatX, AgentState.PosX, Chase * 0.02f);
		const float Prox = 1.f - FMath::Clamp(FMath::Abs(ThreatX - AgentState.PosX), 0.f, 1.f);
		Sense(ERegionRole::Vision, 0, FMath::Max(0.f, ThreatX - AgentState.PosX));
		Sense(ERegionRole::Vision, 1, FMath::Max(0.f, AgentState.PosX - ThreatX));
		Sense(ERegionRole::ThreatDetection, 0, Prox);
		Sense(ERegionRole::Hearing, 0, Prox * Rng.FRandRange(0.6f, 1.f));
		break;
	}

	case ETrialType::MaintainEnergy:
		// sparse random pings the brain should NOT overreact to
		if (Tick % 40 < 4) { Sense(ERegionRole::Hearing, 1, 0.7f); }
		break;

	case ETrialType::NavigateObstacle:
	{
		const float ObstacleX = FMath::Sin(T * 6.28f) * 0.5f;
		Sense(ERegionRole::Vision, 0, FMath::Max(0.f, ObstacleX - AgentState.PosX));
		Sense(ERegionRole::Vision, 1, FMath::Max(0.f, AgentState.PosX - ObstacleX));
		Sense(ERegionRole::Vision, 2, 1.f - FMath::Abs(ObstacleX - AgentState.PosX));
		Sense(ERegionRole::Vision, 3, FMath::Max(0.f, RelTarget));
		break;
	}

	case ETrialType::RememberDirection:
		if (Tick < CueEndTick)
		{
			Sense(ERegionRole::Vision, CueDirection > 0.f ? 0 : 1, 1.f);
			Sense(ERegionRole::Attention, 0, 1.f);
		}
		break;

	case ETrialType::FindWarmth:
	{
		AgentState.BodyTemp = FMath::Max(0.f, AgentState.BodyTemp - 0.0012f);
		const float Warm = 1.f - FMath::Clamp(FMath::Abs(RelTarget), 0.f, 1.f);
		Sense(ERegionRole::TouchPain, 1, Warm);                              // warmth gradient
		Sense(ERegionRole::TouchPain, 2, FMath::Max(0.f, RelTarget));
		Sense(ERegionRole::TouchPain, 3, FMath::Max(0.f, -RelTarget));
		if (Warm > 0.8f) { AgentState.BodyTemp = FMath::Min(1.f, AgentState.BodyTemp + 0.004f); }
		break;
	}

	case ETrialType::InterpretVoice:
	{
		const float Carrier = (FMath::Sin(Tick * VoicePattern * 0.8f) * 0.5f + 0.5f);
		Sense(ERegionRole::Hearing, 0, Carrier);
		Sense(ERegionRole::Hearing, 1, VoicePattern);
		break;
	}

	case ETrialType::SocialBond:
	{
		const bool bDistress = VoicePattern > 0.6f;
		Sense(ERegionRole::Hearing, 0, bDistress ? 0.9f : 0.4f);
		Sense(ERegionRole::Vision, 2, 0.6f);
		Sense(ERegionRole::Hearing, 2, bDistress ? Rng.FRandRange(0.5f, 1.f) : 0.2f);
		break;
	}

	case ETrialType::PlanWinter:
	{
		const float ColdSoon = FMath::Clamp((T - 0.4f) * 2.5f, 0.f, 1.f);
		AgentState.BodyTemp = FMath::Max(0.f, 0.5f - ColdSoon * 0.35f);
		Sense(ERegionRole::Vision, 2, 1.f - T);                              // visible food early
		Sense(ERegionRole::TouchPain, 1, ColdSoon);
		break;
	}

	default:
		break;
	}
}

void FTrialRunner::ApplyMotorOutputs(FBrainGraph& Brain, FRandomStream& Rng)
{
	// motor output channels: 0 = move right, 1 = move left, 2 = act (reach/eat/speak), 3 = rest/brace
	float MoveR = 0.f, MoveL = 0.f, Act = 0.f, Rest = 0.f;
	if (const FBrainRegion* Motor = Brain.FindByRole(ERegionRole::Motor))
	{
		if (Motor->bAwake)
		{
			MoveR = FMath::Max(0.f, Motor->Out[0]);
			MoveL = FMath::Max(0.f, Motor->Out[1]);
			Act = FMath::Max(0.f, Motor->Out[2]);
			Rest = FMath::Max(0.f, Motor->Out[3]);
		}
	}

	float Speak = 0.f;
	if (const FBrainRegion* Lang = Brain.FindByRole(ERegionRole::Language))
	{
		if (Lang->bAwake) { Speak = FMath::Max(0.f, Lang->Out[0]); }
	}
	float SocialOut = 0.f;
	if (const FBrainRegion* Soc = Brain.FindByRole(ERegionRole::SocialCognition))
	{
		if (Soc->bAwake) { SocialOut = FMath::Max(0.f, Soc->Out[0]); }
	}

	const float Drive = MoveR - MoveL;
	if (FMath::Abs(Drive) > 0.12f && AgentState.FirstReactionTick < 0)
	{
		AgentState.FirstReactionTick = Tick;
	}

	// physics
	AgentState.VelX = FMath::Clamp(AgentState.VelX + Drive * 0.02f, -0.06f, 0.06f);
	AgentState.VelX *= 0.9f;
	AgentState.PosX = FMath::Clamp(AgentState.PosX + AgentState.VelX, -1.f, 1.f);

	// balance penalty for violent motion
	const float Violence = FMath::Abs(Drive);
	AgentState.Balance = FMath::Clamp(AgentState.Balance + 0.01f - Violence * 0.02f, 0.f, 1.f);

	// energy costs
	const float Effort = Violence * 0.004f + Act * 0.003f + Speak * 0.002f + 0.0006f;
	const float Recovery = Rest * 0.002f;
	AgentState.Energy = FMath::Clamp(AgentState.Energy - Effort + Recovery, 0.f, 1.f);
	AgentState.EnergySpent += Effort;

	// scenario consequences
	const float DistTarget = FMath::Abs(TargetX - AgentState.PosX);
	const float DistThreat = FMath::Abs(ThreatX - AgentState.PosX);

	switch (Type)
	{
	case ETrialType::ApproachReward:
		if (DistTarget < 0.15f && Act > 0.3f && ReachedAt < 0.f) { ReachedAt = (float)Tick; }
		break;

	case ETrialType::AvoidThreat:
	case ETrialType::EscapePredator:
		if (DistThreat < 0.12f)
		{
			AgentState.Pain = FMath::Min(1.f, AgentState.Pain + 0.05f);
			AgentState.PainTaken += 0.05f;
			AgentState.Health = FMath::Max(0.f, AgentState.Health - 0.01f);
		}
		else
		{
			AgentState.Pain *= 0.96f;
		}
		break;

	case ETrialType::RecognizeFood:
		if (Act > 0.4f && ReachedAt < 0.f)
		{
			if (DistTarget < 0.2f) { ReachedAt = (float)Tick; }                       // correct food
			else if (DistThreat < 0.2f) { AgentState.PainTaken += 0.5f; ReachedAt = (float)Tick; Accumulated = -1.f; } // poison
		}
		break;

	case ETrialType::NavigateObstacle:
	{
		const float ObstacleX = FMath::Sin((float)Tick / Def->DurationTicks * 6.28f) * 0.5f;
		if (FMath::Abs(ObstacleX - AgentState.PosX) < 0.1f)
		{
			AgentState.Balance = FMath::Max(0.f, AgentState.Balance - 0.03f);
			AgentState.PainTaken += 0.01f;
		}
		if (DistTarget < 0.15f && ReachedAt < 0.f) { ReachedAt = (float)Tick; }
		break;
	}

	case ETrialType::RememberDirection:
		if (Tick > CueEndTick * 2)
		{
			// moving in the remembered direction accumulates score
			Accumulated += FMath::Clamp(AgentState.VelX * CueDirection * 8.f, -0.02f, 0.03f);
		}
		break;

	case ETrialType::FindWarmth:
		if (DistTarget < 0.18f) { Accumulated += 0.012f; }
		break;

	case ETrialType::MaintainEnergy:
		// reward staying calm: activity is the enemy
		Accumulated += (Violence < 0.05f && Act < 0.1f) ? 0.008f : -0.004f;
		break;

	case ETrialType::InterpretVoice:
	{
		// score echoing the pattern: speak intensity should track VoicePattern
		const float Match = 1.f - FMath::Abs(Speak - VoicePattern);
		if (Speak > 0.1f) { Accumulated += FMath::Clamp(Match * 0.012f, 0.f, 0.012f); AgentState.Comprehension = FMath::Max(AgentState.Comprehension, Match); }
		break;
	}

	case ETrialType::SocialBond:
	{
		const bool bDistress = VoicePattern > 0.6f;
		const float Appropriate = bDistress ? SocialOut : (1.f - FMath::Abs(SocialOut - 0.3f));
		Accumulated += FMath::Clamp(Appropriate * 0.01f, 0.f, 0.01f);
		AgentState.SocialBond = FMath::Clamp(AgentState.SocialBond + Appropriate * 0.01f, 0.f, 1.f);
		break;
	}

	case ETrialType::UseTool:
		// requires sustained act output while NEAR but not AT target (using the stick)
		if (DistTarget > 0.2f && DistTarget < 0.5f && Act > 0.4f) { Accumulated += 0.012f; }
		if (Accumulated > 0.5f && DistTarget < 0.55f && ReachedAt < 0.f) { ReachedAt = (float)Tick; }
		break;

	case ETrialType::PlanWinter:
	{
		const float T = (float)Tick / Def->DurationTicks;
		if (T < 0.4f && Act > 0.3f) { Accumulated += 0.015f; }               // storing food early
		if (T > 0.6f) { AgentState.Energy = FMath::Max(0.f, AgentState.Energy - 0.003f + Accumulated * 0.004f); }
		break;
	}

	default:
		break;
	}

	AgentState.Hunger = FMath::Min(1.f, AgentState.Hunger + 0.0008f);
	AgentState.Fear = FMath::Clamp(AgentState.Fear + (DistThreat < 0.3f ? 0.01f : -0.008f), 0.f, 1.f);
}

bool FTrialRunner::Step(FBrainGraph& Brain, FRandomStream& Rng, int32 OpenRegionId)
{
	if (!bActive || !Def) { return false; }

	WriteSensoryInputs(Brain, Rng);
	Brain.Tick(Rng, OpenRegionId);
	ApplyMotorOutputs(Brain, Rng);

	Tick++;
	return Tick < Def->DurationTicks;
}

float FTrialRunner::EvaluateScore(FBrainGraph& Brain) const
{
	float Score = 0.f;
	const float Duration = (float)Def->DurationTicks;

	switch (Type)
	{
	case ETrialType::ApproachReward:
	case ETrialType::NavigateObstacle:
	case ETrialType::UseTool:
	{
		if (ReachedAt >= 0.f)
		{
			Score = 0.6f + 0.4f * (1.f - ReachedAt / Duration);
		}
		else
		{
			Score = 0.25f * (1.f - FMath::Abs(TargetX - AgentState.PosX));
		}
		if (Type == ETrialType::NavigateObstacle)
		{
			Score *= FMath::Lerp(0.5f, 1.f, AgentState.Balance);
		}
		break;
	}

	case ETrialType::AvoidThreat:
	case ETrialType::EscapePredator:
	{
		Score = FMath::Clamp(1.f - AgentState.PainTaken * 1.4f, 0.f, 1.f);
		if (AgentState.FirstReactionTick >= 0)
		{
			Score = FMath::Min(1.f, Score + 0.15f * (1.f - AgentState.FirstReactionTick / Duration));
		}
		break;
	}

	case ETrialType::MaintainEnergy:
		Score = FMath::Clamp(Accumulated + AgentState.Energy * 0.4f, 0.f, 1.f);
		break;

	case ETrialType::RememberDirection:
	case ETrialType::FindWarmth:
	case ETrialType::InterpretVoice:
	case ETrialType::SocialBond:
	case ETrialType::PlanWinter:
		Score = FMath::Clamp(Accumulated, 0.f, 1.f);
		if (Type == ETrialType::FindWarmth) { Score = FMath::Clamp(Score + (AgentState.BodyTemp - 0.3f), 0.f, 1.f); }
		if (Type == ETrialType::PlanWinter) { Score = FMath::Clamp(Score * 0.6f + AgentState.Energy * 0.4f, 0.f, 1.f); }
		break;

	case ETrialType::RecognizeFood:
		Score = (Accumulated < 0.f) ? 0.05f : (ReachedAt >= 0.f ? 0.85f : 0.2f);
		break;

	default:
		break;
	}

	// energy efficiency bonus/penalty (guide: efficiency matters)
	const float EffFactor = FMath::Clamp(1.f - AgentState.EnergySpent * 0.5f, 0.7f, 1.f);
	return FMath::Clamp(Score * EffFactor, 0.f, 1.f);
}

FTrialResult FTrialRunner::Finalize(FBrainGraph& Brain, FFitnessState& Fitness, FRandomStream& Rng)
{
	FTrialResult R;
	R.TrialNumber = TrialNumber;
	R.Type = Type;
	R.FitnessBefore = Fitness.OverallPercent();
	R.Score01 = EvaluateScore(Brain);

	const float StabBefore = Fitness.StabilityMod;

	// learning rate scales with score contrast so both success and failure teach
	const float LR = 0.09f;
	Fitness.ApplyTrialScore(Def->PrimaryDim, R.Score01, 1.0f, LR);
	Fitness.ApplyTrialScore(Def->SecondaryDim, R.Score01, 0.55f, LR);

	// every trial slightly trains adaptability toward mid via variety (handled by session)
	Fitness.StabilityMod = FMath::Lerp(Fitness.StabilityMod, Brain.GlobalStability(), 0.15f);
	Fitness.EfficiencyMod = FMath::Lerp(Fitness.EfficiencyMod, Brain.SignalEfficiency(), 0.15f);

	R.FitnessAfter = Fitness.OverallPercent();
	R.StabilityDelta = (Fitness.StabilityMod - StabBefore) * 100.f;
	R.EnergyDeltaPct = (AgentState.EnergySpent - 0.25f) * 100.f;

	// dominant regions = highest per-trial activity
	TArray<TPair<float, int32>> Ranked;
	for (const FBrainRegion& Reg : Brain.Regions)
	{
		if (Reg.bAwake && Reg.ActivityDuringTrial > 1.f)
		{
			Ranked.Emplace(Reg.ActivityDuringTrial, Reg.Id);
		}
	}
	Ranked.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key > B.Key; });
	for (int32 i = 0; i < FMath::Min(3, Ranked.Num()); ++i)
	{
		R.DominantRegions.Add(Ranked[i].Value);
	}

	bActive = false;
	return R;
}
