#include "ForgeSession.h"

void FForgeSession::NewGame(int32 InSeed, const FString& InLineageName)
{
	Seed = InSeed;
	LineageName = InLineageName.IsEmpty() ? TEXT("Lineage Alpha") : InLineageName;
	Rng.Initialize(Seed);

	Brain = FBrainGraph();
	Fitness = FFitnessState();
	Discovery = FDiscoverySystem();
	Patterns = FPatternLibrary();
	Runner = FTrialRunner();

	BuildStarterBrain(Brain, Rng);

	Phase = EForgePhase::Reflex;
	Generation = 1;
	TrialCounter = 0;
	TrialsThisGeneration = 0;
	Energy = 250.f;
	EnergyCap = 500.f;
	FitnessScore = 0.f;
	BestRun = 0.f;
	ScoreRate = 0.f;
	Speed = 1;
	bAutoRun = true;
	BatchRemaining = 0;
	OpenRegionId = INDEX_NONE;
	LearningRateMult = 1.f;
	LearningBoostTrialsLeft = 0;
	bFreeAwaken = false;
	PermStabilityBonus = 0.f;
	bWon = false;
	bEverWon = false;
	PendingOffers.Reset();
	LowStabilityStreak = 0;
	History.Reset();
	FitnessCurve.Reset();
	RecentScores.Reset();
	ArchiveLog.Reset();
	MutationCount = 0;
	PruneCount = 0;
	EventQueue.Reset();
	TickAccumulator = 0.f;
	InterTrialPause = 0.5f;

	ArchiveLog.Add(FString::Printf(TEXT("Genesis. Seed %d. Four dormant reflexes stir in the dark."), Seed));
	PushEvent(EForgeSound::Awaken, TEXT("Evolution begins. Signals mean nothing yet."), true);
}

float FForgeSession::DisplayFitness() const
{
	// hidden dims rarely hit 1.0; scale so a superbly balanced brain can show 100%
	return FMath::Min(100.f, Fitness.Overall() * 118.f);
}

FNeuralNetwork* FForgeSession::OpenNetwork()
{
	FBrainRegion* R = OpenRegion();
	return R ? &R->Net : nullptr;
}

FBrainRegion* FForgeSession::OpenRegion()
{
	return OpenRegionId != INDEX_NONE ? Brain.FindRegion(OpenRegionId) : nullptr;
}

void FForgeSession::PushEvent(EForgeSound Sound, const FString& Text, bool bImportant)
{
	FForgeEvent E;
	E.Sound = Sound;
	E.Text = Text;
	E.bImportant = bImportant;
	EventQueue.Add(E);
}

bool FForgeSession::Spend(float Cost)
{
	if (Energy < Cost)
	{
		PushEvent(EForgeSound::EnergyLow, FString::Printf(TEXT("Insufficient energy (%d needed)"), FMath::RoundToInt(Cost)));
		return false;
	}
	Energy -= Cost;
	return true;
}

namespace
{
	struct FTutorialStep
	{
		const TCHAR* Objective;
		const TCHAR* DoneName;
	};
	const FTutorialStep GTutorialSteps[FForgeSession::NumTutorialSteps] =
	{
		{ TEXT("Watch the brain work: let 5 trials run. Notice which regions light up."), TEXT("Observation") },
		{ TEXT("Press M to MUTATE. Most mutations fail - evolution is patient."),         TEXT("First mutation") },
		{ TEXT("Wire a pathway: hold Ctrl and drag from one region to another."),         TEXT("First wiring") },
		{ TEXT("Double-click a lit region to enter the neurons inside it."),              TEXT("Going deeper") },
		{ TEXT("Press P to PRUNE dead pathways. A quiet brain is a cheap brain."),        TEXT("First pruning") },
		{ TEXT("Press Q to send a REWARD PULSE - it strengthens active paths."),          TEXT("Reinforcement") },
		{ TEXT("Reach Generation 2. Every 25 trials, the lineage adapts."),               TEXT("A new generation") },
	};
}

FString FForgeSession::TutorialObjective() const
{
	if (TutorialDone()) { return FString(); }
	return GTutorialSteps[TutorialStage].Objective;
}

FString FForgeSession::TutorialProgress() const
{
	switch (TutorialStage)
	{
	case 0: return FString::Printf(TEXT("%d / 5 trials"), FMath::Min(TrialCounter, 5));
	case 6: return FString::Printf(TEXT("%d / %d trials this generation"), TrialsThisGeneration, TrialsPerGeneration);
	default: return FString();
	}
}

void FForgeSession::UpdateTutorial()
{
	if (TutorialDone()) { return; }

	bool bComplete = false;
	switch (TutorialStage)
	{
	case 0: bComplete = TrialCounter >= 5; break;
	case 1: bComplete = Discovery.MutateActions >= 1; break;
	case 2: bComplete = PlayerConnections >= 1; break;
	case 3: bComplete = bHasEnteredRegion; break;
	case 4: bComplete = Discovery.PruneActions >= 1; break;
	case 5: bComplete = Discovery.ReinforceActions >= 1; break;
	case 6: bComplete = Generation >= 2; break;
	default: break;
	}

	if (bComplete)
	{
		Energy = FMath::Min(EnergyCap, Energy + 40.f);
		PushEvent(EForgeSound::Discovery,
			FString::Printf(TEXT("First steps - %s complete (+40 energy)"), GTutorialSteps[TutorialStage].DoneName), true);
		TutorialStage++;
		if (TutorialDone())
		{
			PushEvent(EForgeSound::RewardChime, TEXT("You know enough now. The rest is discovery."), true);
		}
	}
}

void FForgeSession::Update(float DeltaSeconds)
{
	UpdateTutorial();

	// modal reward choice or victory screen halts the flow
	if (PendingOffers.Num() > 0 || Speed == 0)
	{
		return;
	}

	// score rate EMA
	ScoreRateTimer += DeltaSeconds;
	if (ScoreRateTimer > 1.f)
	{
		ScoreRate = FMath::Lerp(ScoreRate, ScoreRateAccum / ScoreRateTimer, 0.3f);
		ScoreRateAccum = 0.f;
		ScoreRateTimer = 0.f;
	}

	const float TicksPerSecond = 16.f * Speed;
	TickAccumulator += DeltaSeconds * TicksPerSecond;

	// clamp so a hitch doesn't fast-forward the world
	TickAccumulator = FMath::Min(TickAccumulator, TicksPerSecond * 0.5f);

	while (TickAccumulator >= 1.f)
	{
		TickAccumulator -= 1.f;

		if (Runner.IsActive())
		{
			if (!Runner.Step(Brain, Rng, OpenRegionId))
			{
				FinishTrial();
			}
		}
		else
		{
			// idle brain still hums quietly between trials
			Brain.Tick(Rng, OpenRegionId);

			InterTrialPause -= 1.f / TicksPerSecond;
			if (InterTrialPause <= 0.f && (bAutoRun || BatchRemaining > 0))
			{
				StartNextTrial();
			}
		}
	}
}

void FForgeSession::StartNextTrial()
{
	Brain.CompileDirty(Rng, OpenRegionId);

	const TArray<ETrialType> Available = TrialsForPhase(Phase);
	if (Available.Num() == 0) { return; }
	const ETrialType Type = Available[Rng.RandRange(0, Available.Num() - 1)];

	TrialCounter++;
	Runner.Begin(Type, TrialCounter, Brain, Rng);
}

void FForgeSession::FinishTrial()
{
	FTrialResult R = Runner.Finalize(Brain, Fitness, Rng);
	R.bNameKnown = Discovery.TrialNameKnown(R.Type);

	// automatic background plasticity: success reinforces the paths that carried it
	const float LR = 0.05f * LearningRateMult;
	const float Reward = (R.Score01 - 0.42f) * 2.f;
	for (int32 RegionId : R.DominantRegions)
	{
		if (FBrainRegion* Reg = Brain.FindRegion(RegionId))
		{
			Reg->Net.Reinforce(Reward, LR, true);
		}
	}

	// adaptability grows from trial variety
	Fitness.ApplyTrialScore(EFitnessDim::Adaptability, FMath::Min(1.f, Discovery.Knowledge() + 0.3f), 0.4f, 0.04f);

	// perception grows whenever sensory regions carried a scoring trial
	for (int32 RegionId : R.DominantRegions)
	{
		if (const FBrainRegion* Reg = Brain.FindRegion(RegionId))
		{
			if (RegionIsSensory(Reg->Role))
			{
				Fitness.ApplyTrialScore(EFitnessDim::Perception, R.Score01, 0.3f, 0.05f);
			}
			if (Reg->Role == ERegionRole::SocialCognition || Reg->Role == ERegionRole::Emotion)
			{
				Fitness.ApplyTrialScore(EFitnessDim::Social, R.Score01, 0.3f, 0.05f);
			}
			if (Reg->Role == ERegionRole::Language)
			{
				Fitness.ApplyTrialScore(EFitnessDim::Communication, R.Score01, 0.3f, 0.05f);
			}
		}
	}

	// heuristic bonuses feed the fitness modifiers
	Fitness.EfficiencyMod = FMath::Clamp(Fitness.EfficiencyMod + Discovery.TotalEfficiencyBonus() * 0.01f, 0.f, 1.f);
	Fitness.StabilityMod = FMath::Clamp(Fitness.StabilityMod + (Discovery.TotalStabilityBonus() + PermStabilityBonus) * 0.01f, 0.02f, 1.f);

	// discoveries
	R.DiscoveryLines = Discovery.OnTrialComplete(Brain, R, Fitness, Rng);
	for (const FString& Line : R.DiscoveryLines)
	{
		ArchiveLog.Add(Line);
		PushEvent(EForgeSound::Discovery, Line, true);
	}

	// economy + score juice
	const float EnergyGain = 6.f + 26.f * R.Score01;
	Energy = FMath::Min(EnergyCap, Energy + EnergyGain);

	const float Points = R.Score01 * 10.f * (1.f + 0.12f * (Generation - 1));
	FitnessScore += Points;
	ScoreRateAccum += Points;
	BestRun = FMath::Max(BestRun, FitnessScore);

	// history
	History.Add(R);
	if (History.Num() > 400) { History.RemoveAt(0, History.Num() - 400); }
	FitnessCurve.Add(DisplayFitness());
	if (FitnessCurve.Num() > 500) { FitnessCurve.RemoveAt(0, FitnessCurve.Num() - 500); }
	RecentScores.Add(R.Score01);
	if (RecentScores.Num() > 24) { RecentScores.RemoveAt(0, RecentScores.Num() - 24); }

	// trial sound: calm, only meaningful moments get loud
	if (R.Score01 > 0.65f) { PushEvent(EForgeSound::TrialWin); }
	else if (R.Score01 < 0.12f) { PushEvent(EForgeSound::Inhibit); }
	else { PushEvent(EForgeSound::TrialSoft); }

	// instability consequences (guide section 25)
	if (Brain.GlobalStability() < 0.3f)
	{
		LowStabilityStreak++;
		if (LowStabilityStreak >= 3)
		{
			LowStabilityStreak = 0;
			for (int32 i = 0; i < FitnessDimCount(); ++i)
			{
				Fitness.Dims[i] = FMath::Max(0.01f, Fitness.Dims[i] * 0.94f);
			}
			ArchiveLog.Add(TEXT("Catastrophic feedback loop. The brain seized; some learning was lost."));
			PushEvent(EForgeSound::Failure, TEXT("Seizure-like instability! Fitness degraded."), true);
		}
	}
	else
	{
		LowStabilityStreak = 0;
	}

	if (LearningBoostTrialsLeft > 0 && --LearningBoostTrialsLeft == 0)
	{
		LearningRateMult = 1.f;
		PushEvent(EForgeSound::Inhibit, TEXT("Plasticity surge faded."));
	}

	if (BatchRemaining > 0) { BatchRemaining--; }

	// generation rollover
	TrialsThisGeneration++;
	if (TrialsThisGeneration >= TrialsPerGeneration)
	{
		TrialsThisGeneration = 0;
		Generation++;
		MakeGenerationOffers();
		PushEvent(EForgeSound::Generation, FString::Printf(TEXT("Generation %d. Choose an adaptation."), Generation), true);
		ArchiveLog.Add(FString::Printf(TEXT("Generation %d reached at fitness %.1f%%."), Generation, DisplayFitness()));
	}

	CheckPhaseAdvance();

	// victory (guide section 1: viable human brain)
	if (!bEverWon && DisplayFitness() >= 100.f)
	{
		bWon = true;
		bEverWon = true;
		PushEvent(EForgeSound::Victory, TEXT("A VIABLE HUMAN BRAIN. It thinks. It remembers. It plans. It is."), true);
		ArchiveLog.Add(TEXT("Total fitness 100%. The blind experiment opened its eyes."));
	}

	InterTrialPause = 1.2f;
}

void FForgeSession::MakeGenerationOffers()
{
	PendingOffers.Reset();

	struct FPool { int32 Id; const TCHAR* Title; const TCHAR* Desc; };
	TArray<FPool> Pool =
	{
		{ 1, TEXT("Metabolic Reserve"),  TEXT("+100 max energy, and refill to full.") },
		{ 2, TEXT("Pattern Slot"),       TEXT("+1 pattern memory slot.") },
		{ 3, TEXT("Insight Probe"),      TEXT("Reveal the probable function of an unknown region.") },
		{ 4, TEXT("Stability Bath"),     TEXT("Permanently +4% stability modifier.") },
		{ 5, TEXT("Plasticity Surge"),   TEXT("Learning rate +60% for the next 50 trials.") },
		{ 6, TEXT("Developmental Leap"), TEXT("The next region awakening is free.") },
	};

	// pick 3 distinct offers
	for (int32 i = 0; i < 3 && Pool.Num() > 0; ++i)
	{
		const int32 Idx = Rng.RandRange(0, Pool.Num() - 1);
		FGenerationOffer O;
		O.Id = Pool[Idx].Id;
		O.Title = Pool[Idx].Title;
		O.Desc = Pool[Idx].Desc;
		PendingOffers.Add(O);
		Pool.RemoveAt(Idx);
	}
}

void FForgeSession::ChooseGenerationReward(int32 OfferId)
{
	bool bFound = false;
	for (const FGenerationOffer& O : PendingOffers)
	{
		if (O.Id == OfferId) { bFound = true; break; }
	}
	if (!bFound) { return; }

	switch (OfferId)
	{
	case 1:
		EnergyCap += 100.f;
		Energy = EnergyCap;
		ArchiveLog.Add(TEXT("Adaptation: metabolic reserve expanded."));
		break;
	case 2:
		Patterns.SlotCap++;
		ArchiveLog.Add(TEXT("Adaptation: pattern memory expanded."));
		break;
	case 3:
	{
		FBrainRegion* Best = nullptr;
		for (FBrainRegion& R : Brain.Regions)
		{
			if (R.bAwake && !R.NameKnown() && (!Best || R.DiscoveryConfidence > Best->DiscoveryConfidence))
			{
				Best = &R;
			}
		}
		if (Best)
		{
			Best->DiscoveryConfidence = FMath::Max(Best->DiscoveryConfidence, 0.55f);
			const FString Line = FString::Printf(TEXT("Probe result: %s is probably %s."), *Best->HiddenLabel, RegionRoleName(Best->Role));
			ArchiveLog.Add(Line);
			PushEvent(EForgeSound::Discovery, Line, true);
		}
		else
		{
			Energy = FMath::Min(EnergyCap, Energy + 80.f);
		}
		break;
	}
	case 4:
		PermStabilityBonus += 4.f;
		ArchiveLog.Add(TEXT("Adaptation: baseline stability improved."));
		break;
	case 5:
		LearningRateMult = 1.6f;
		LearningBoostTrialsLeft = 50;
		ArchiveLog.Add(TEXT("Adaptation: plasticity surge active."));
		break;
	case 6:
		bFreeAwaken = true;
		ArchiveLog.Add(TEXT("Adaptation: developmental leap stored."));
		break;
	default:
		break;
	}

	PendingOffers.Reset();
	PushEvent(EForgeSound::RewardChime);
}

void FForgeSession::CheckPhaseAdvance()
{
	const float F = DisplayFitness();
	EForgePhase NewPhase = Phase;
	if (F >= 65.f)      { NewPhase = EForgePhase::Cognition; }
	else if (F >= 50.f) { NewPhase = EForgePhase::Language; }
	else if (F >= 35.f) { NewPhase = EForgePhase::Social; }
	else if (F >= 20.f) { NewPhase = EForgePhase::MemoryPhase; }
	else if (F >= 8.f)  { NewPhase = EForgePhase::Perception; }

	if ((int32)NewPhase > (int32)Phase)
	{
		Phase = NewPhase;
		const FString Line = FString::Printf(TEXT("%s unlocked. New regions can awaken; new pressures apply."), PhaseName(Phase));
		ArchiveLog.Add(Line);
		PushEvent(EForgeSound::Awaken, Line, true);
	}
}

// ------------------------------------------------------------- actions

bool FForgeSession::ActionRunBatch()
{
	if (!Spend(ForgeCost::RunHundred)) { return false; }
	BatchRemaining = 100;
	bAutoRun = true;
	if (Speed == 0) { Speed = 4; }
	PushEvent(EForgeSound::Click, TEXT("Batch: 100 trials queued."));
	return true;
}

bool FForgeSession::ActionRandomize()
{
	if (!Spend(ForgeCost::Randomize)) { return false; }
	if (FNeuralNetwork* Net = OpenNetwork())
	{
		Net->Randomize(Rng, 0.35f);
	}
	else
	{
		for (FBrainRegion& R : Brain.Regions)
		{
			if (R.bAwake) { R.Net.Randomize(Rng, 0.2f); }
		}
	}
	MutationCount++;
	PushEvent(EForgeSound::Mutate, TEXT("Weights scrambled. Something is different now."));
	return true;
}

bool FForgeSession::ActionMutate(bool bCluster)
{
	const float Cost = bCluster ? ForgeCost::MutateCluster : ForgeCost::Mutate;
	if (!Spend(Cost)) { return false; }

	const float Intensity = bCluster ? 0.9f : 0.4f;
	int32 Changed = 0;

	if (FNeuralNetwork* Net = OpenNetwork())
	{
		MutationEngine::Mutate(*Net, Intensity, Rng, &Changed);
	}
	else
	{
		// whole-brain: mutate a random awake region
		TArray<FBrainRegion*> Awake;
		for (FBrainRegion& R : Brain.Regions) { if (R.bAwake) { Awake.Add(&R); } }
		if (Awake.Num() > 0)
		{
			FBrainRegion* Target = Awake[Rng.RandRange(0, Awake.Num() - 1)];
			MutationEngine::Mutate(Target->Net, Intensity, Rng, &Changed);
		}
	}

	MutationCount++;
	Discovery.MutateActions++;
	ArchiveLog.Add(FString::Printf(TEXT("Mutation applied (%d structures affected)."), Changed));
	PushEvent(EForgeSound::Mutate, FString::Printf(TEXT("Mutation: %d structures affected."), Changed));
	return true;
}

bool FForgeSession::ActionPrune()
{
	if (!Spend(ForgeCost::Prune)) { return false; }

	int32 Removed = 0;
	if (FNeuralNetwork* Net = OpenNetwork())
	{
		Removed = Net->PruneWeakest(4, 0.05f);
	}
	else
	{
		for (FBrainRegion& R : Brain.Regions)
		{
			if (R.bAwake) { Removed += R.Net.PruneWeakest(2, 0.04f); }
		}
	}

	PruneCount++;
	Discovery.PruneActions++;
	PushEvent(EForgeSound::Prune, FString::Printf(TEXT("Pruned %d weak connections."), Removed));
	return true;
}

bool FForgeSession::ActionRewardPulse()
{
	if (!Spend(ForgeCost::RewardPulse)) { return false; }

	const float LR = 0.12f * LearningRateMult;
	if (FNeuralNetwork* Net = OpenNetwork())
	{
		Net->Reinforce(1.f, LR, true);
	}
	else
	{
		for (FBrainRegion& R : Brain.Regions)
		{
			if (R.bAwake) { R.Net.Reinforce(1.f, LR * 0.7f, true); }
		}
	}

	Discovery.ReinforceActions++;
	PushEvent(EForgeSound::RewardChime, TEXT("Reward pulse: active paths strengthened."));
	return true;
}

bool FForgeSession::ActionLock(const TArray<int32>& NodeIds, const TArray<int32>& ConnIds)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net || (NodeIds.Num() == 0 && ConnIds.Num() == 0)) { return false; }
	if (!Spend(ForgeCost::LockPattern)) { return false; }

	int32 Count = 0;
	for (int32 Id : NodeIds)
	{
		if (FNeuralNode* N = Net->FindNode(Id)) { N->bLocked = !N->bLocked; Count++; }
	}
	for (int32 Id : ConnIds)
	{
		if (FNeuralConnection* C = Net->FindConnection(Id)) { C->bLocked = !C->bLocked; Count++; }
	}

	Discovery.LockActions++;
	PushEvent(EForgeSound::Lock, FString::Printf(TEXT("Toggled lock on %d structures."), Count));
	return Count > 0;
}

bool FForgeSession::ActionAddNode(ENodeType Type, const FVector2D& Pos)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net || Net->Nodes.Num() >= 30) { return false; }
	if (!Spend(ForgeCost::AddNode)) { return false; }

	Net->AddNode(Type, Pos, Rng);
	PushEvent(EForgeSound::Click, FString::Printf(TEXT("%s node grown."), NodeTypeName(Type)));
	return true;
}

bool FForgeSession::ActionConnect(int32 SrcNodeId, int32 DstNodeId, EConnType Type)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }
	if (Net->HasConnection(SrcNodeId, DstNodeId) || SrcNodeId == DstNodeId) { return false; }
	if (!Spend(ForgeCost::Connect)) { return false; }

	if (Net->AddConnection(SrcNodeId, DstNodeId, Type, Rng.FRandRange(0.4f, 0.9f)))
	{
		PlayerConnections++;
		PushEvent(EForgeSound::Connect);
		return true;
	}
	Energy += ForgeCost::Connect; // refund
	return false;
}

bool FForgeSession::ActionDisconnect(int32 ConnId)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }
	if (!Spend(ForgeCost::Disconnect)) { return false; }

	if (Net->RemoveConnection(ConnId))
	{
		PushEvent(EForgeSound::Disconnect);
		return true;
	}
	Energy += ForgeCost::Disconnect;
	return false;
}

bool FForgeSession::ActionBoost(int32 ConnId, float Delta)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }
	FNeuralConnection* C = Net->FindConnection(ConnId);
	if (!C || C->bLocked) { return false; }
	if (!Spend(Delta > 0.f ? ForgeCost::Boost : ForgeCost::Dampen)) { return false; }

	C->Weight = FMath::Clamp(C->Weight + Delta, -2.f, 2.f);
	Net->bCompiledDirty = true;
	PushEvent(Delta > 0.f ? EForgeSound::Click : EForgeSound::Inhibit);
	return true;
}

bool FForgeSession::ActionDuplicateMotif()
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }
	if (!Spend(ForgeCost::DuplicateMotif)) { return false; }

	if (MutationEngine::DuplicateMotif(*Net, Rng))
	{
		PushEvent(EForgeSound::Mutate, TEXT("Most reinforced motif duplicated."));
		return true;
	}
	Energy += ForgeCost::DuplicateMotif;
	PushEvent(EForgeSound::Inhibit, TEXT("No motif worth duplicating."));
	return false;
}

bool FForgeSession::ActionAutoTest()
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }
	if (!Spend(ForgeCost::AutoTest)) { return false; }

	Net->Compile(Rng);
	const FString Report = FString::Printf(TEXT("Auto-test: stability %.0f%%, energy draw %.1f, %d response samples."),
		Net->Stability * 100.f, Net->EnergyUse, Net->Compiled.Num());
	ArchiveLog.Add(Report);
	PushEvent(EForgeSound::MemoryEcho, Report);
	return true;
}

bool FForgeSession::ActionAwakenRegion(int32 RegionId)
{
	FBrainRegion* R = Brain.FindRegion(RegionId);
	if (!R || R->bAwake) { return false; }
	if ((int32)RegionUnlockPhase(R->Role) > (int32)Phase)
	{
		PushEvent(EForgeSound::Inhibit, TEXT("This structure is not developmentally ready."));
		return false;
	}

	if (bFreeAwaken)
	{
		bFreeAwaken = false;
	}
	else if (!Spend(ForgeCost::AwakenRegion))
	{
		return false;
	}

	R->bAwake = true;
	R->Net.bCompiledDirty = true;
	ArchiveLog.Add(FString::Printf(TEXT("A dormant structure lit up: %s."), *R->HiddenLabel));
	PushEvent(EForgeSound::Awaken, FString::Printf(TEXT("%s awakened."), *R->DisplayName()), true);
	return true;
}

bool FForgeSession::ActionSavePattern(const TArray<int32>& NodeIds)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }

	FString Err;
	if (Patterns.SaveFromSelection(*Net, NodeIds, Err))
	{
		PushEvent(EForgeSound::Lock, TEXT("Pattern captured to memory."));
		return true;
	}
	PushEvent(EForgeSound::Inhibit, Err);
	return false;
}

bool FForgeSession::ActionInsertPattern(int32 PatternId, const FVector2D& Pos)
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net) { return false; }
	if (!Spend(ForgeCost::InsertPattern)) { return false; }

	const TArray<int32> NewIds = Patterns.Insert(*Net, PatternId, Pos, Rng);
	if (NewIds.Num() > 0)
	{
		PushEvent(EForgeSound::Connect, TEXT("Pattern stamped into the network."));
		return true;
	}
	Energy += ForgeCost::InsertPattern;
	return false;
}

bool FForgeSession::ActionConnectRegions(int32 SrcRegionId, int32 DstRegionId)
{
	if (!Spend(ForgeCost::Connect)) { return false; }

	const FBrainRegion* Src = Brain.FindRegion(SrcRegionId);
	const FBrainRegion* Dst = Brain.FindRegion(DstRegionId);
	if (!Src || !Dst || !Src->bAwake || !Dst->bAwake)
	{
		Energy += ForgeCost::Connect;
		return false;
	}

	// pick the least-used channel pair
	int32 SrcCh = Rng.RandRange(0, MaxChannels - 1);
	int32 DstCh = Rng.RandRange(0, MaxChannels - 1);
	for (int32 Attempt = 0; Attempt < 6 && Brain.HasLink(SrcRegionId, DstRegionId, SrcCh, DstCh); ++Attempt)
	{
		SrcCh = Rng.RandRange(0, MaxChannels - 1);
		DstCh = Rng.RandRange(0, MaxChannels - 1);
	}

	if (Brain.AddLink(SrcRegionId, DstRegionId, SrcCh, DstCh, Rng.FRandRange(0.5f, 0.9f)))
	{
		PlayerConnections++;
		PushEvent(EForgeSound::Connect);
		return true;
	}
	Energy += ForgeCost::Connect;
	return false;
}

bool FForgeSession::ActionDisconnectRegionLink(int32 LinkId)
{
	if (!Spend(ForgeCost::Disconnect)) { return false; }
	if (Brain.RemoveLink(LinkId))
	{
		PushEvent(EForgeSound::Disconnect);
		return true;
	}
	Energy += ForgeCost::Disconnect;
	return false;
}

void FForgeSession::EnterRegion(int32 RegionId)
{
	FBrainRegion* R = Brain.FindRegion(RegionId);
	if (R && R->bAwake)
	{
		OpenRegionId = RegionId;
		bHasEnteredRegion = true;
		PushEvent(EForgeSound::MemoryEcho);
	}
}

void FForgeSession::ExitRegion()
{
	if (OpenRegionId != INDEX_NONE)
	{
		if (FNeuralNetwork* Net = OpenNetwork())
		{
			Net->bCompiledDirty = true; // recompile after edits (guide section 8)
		}
		OpenRegionId = INDEX_NONE;
		PushEvent(EForgeSound::Click);
	}
}

FMutationPreview FForgeSession::GetMutationPreview()
{
	FNeuralNetwork* Net = OpenNetwork();
	if (!Net)
	{
		FBrainRegion* First = nullptr;
		for (FBrainRegion& R : Brain.Regions) { if (R.bAwake) { First = &R; break; } }
		if (!First) { return FMutationPreview(); }
		Net = &First->Net;
	}
	return MutationEngine::Preview(*Net, 0.4f, Knowledge(), Rng);
}
