#include "SaveSystem.h"
#include "ForgeSession.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ------------------------------------------------------------- settings

static FString SettingsPath()
{
	return FPaths::ProjectSavedDir() / TEXT("ForgeSettings.json");
}

void FForgeSettings::Save() const
{
	TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetNumberField(TEXT("MasterVolume"), MasterVolume);
	J->SetNumberField(TEXT("SfxVolume"), SfxVolume);
	J->SetNumberField(TEXT("MusicVolume"), MusicVolume);
	J->SetNumberField(TEXT("WindowMode"), WindowMode);
	J->SetNumberField(TEXT("ResolutionX"), ResolutionX);
	J->SetNumberField(TEXT("ResolutionY"), ResolutionY);
	J->SetNumberField(TEXT("QualityLevel"), QualityLevel);
	J->SetBoolField(TEXT("Autosave"), bAutosave);
	J->SetBoolField(TEXT("TutorialHints"), bShowTutorialHints);
	J->SetBoolField(TEXT("ReducedFlicker"), bReducedFlicker);

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(J, Writer);
	FFileHelper::SaveStringToFile(Out, *SettingsPath());
}

void FForgeSettings::Load()
{
	FString In;
	if (!FFileHelper::LoadFileToString(In, *SettingsPath())) { return; }
	TSharedPtr<FJsonObject> J;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(In);
	if (!FJsonSerializer::Deserialize(Reader, J) || !J.IsValid()) { return; }

	MasterVolume = J->GetNumberField(TEXT("MasterVolume"));
	SfxVolume = J->GetNumberField(TEXT("SfxVolume"));
	MusicVolume = J->GetNumberField(TEXT("MusicVolume"));
	WindowMode = (int32)J->GetNumberField(TEXT("WindowMode"));
	ResolutionX = (int32)J->GetNumberField(TEXT("ResolutionX"));
	ResolutionY = (int32)J->GetNumberField(TEXT("ResolutionY"));
	QualityLevel = (int32)J->GetNumberField(TEXT("QualityLevel"));
	J->TryGetBoolField(TEXT("Autosave"), bAutosave);
	J->TryGetBoolField(TEXT("TutorialHints"), bShowTutorialHints);
	J->TryGetBoolField(TEXT("ReducedFlicker"), bReducedFlicker);
}

// ------------------------------------------------------------- session json

namespace
{
	TSharedRef<FJsonObject> NodeToJson(const FNeuralNode& N)
	{
		TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetNumberField(TEXT("Id"), N.Id);
		J->SetNumberField(TEXT("Type"), (int32)N.Type);
		J->SetNumberField(TEXT("X"), N.Pos.X);
		J->SetNumberField(TEXT("Y"), N.Pos.Y);
		J->SetNumberField(TEXT("Bias"), N.Bias);
		J->SetNumberField(TEXT("Threshold"), N.Threshold);
		J->SetNumberField(TEXT("Decay"), N.Decay);
		J->SetNumberField(TEXT("Noise"), N.Noise);
		J->SetNumberField(TEXT("Plasticity"), N.Plasticity);
		J->SetNumberField(TEXT("EnergyCost"), N.EnergyCost);
		J->SetBoolField(TEXT("Locked"), N.bLocked);
		return J;
	}

	FNeuralNode NodeFromJson(const TSharedPtr<FJsonObject>& J)
	{
		FNeuralNode N;
		N.Id = (int32)J->GetNumberField(TEXT("Id"));
		N.Type = (ENodeType)(int32)J->GetNumberField(TEXT("Type"));
		N.Pos.X = J->GetNumberField(TEXT("X"));
		N.Pos.Y = J->GetNumberField(TEXT("Y"));
		N.Bias = J->GetNumberField(TEXT("Bias"));
		N.Threshold = J->GetNumberField(TEXT("Threshold"));
		N.Decay = J->GetNumberField(TEXT("Decay"));
		N.Noise = J->GetNumberField(TEXT("Noise"));
		N.Plasticity = J->GetNumberField(TEXT("Plasticity"));
		N.EnergyCost = J->GetNumberField(TEXT("EnergyCost"));
		N.bLocked = J->GetBoolField(TEXT("Locked"));
		return N;
	}

	TSharedRef<FJsonObject> ConnToJson(const FNeuralConnection& C)
	{
		TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetNumberField(TEXT("Id"), C.Id);
		J->SetNumberField(TEXT("Src"), C.Source);
		J->SetNumberField(TEXT("Dst"), C.Target);
		J->SetNumberField(TEXT("Type"), (int32)C.Type);
		J->SetNumberField(TEXT("Weight"), C.Weight);
		J->SetNumberField(TEXT("Delay"), C.Delay);
		J->SetNumberField(TEXT("Noise"), C.Noise);
		J->SetNumberField(TEXT("Plasticity"), C.Plasticity);
		J->SetNumberField(TEXT("Reinforced"), C.ReinforceCount);
		J->SetBoolField(TEXT("Locked"), C.bLocked);
		return J;
	}

	FNeuralConnection ConnFromJson(const TSharedPtr<FJsonObject>& J)
	{
		FNeuralConnection C;
		C.Id = (int32)J->GetNumberField(TEXT("Id"));
		C.Source = (int32)J->GetNumberField(TEXT("Src"));
		C.Target = (int32)J->GetNumberField(TEXT("Dst"));
		C.Type = (EConnType)(int32)J->GetNumberField(TEXT("Type"));
		C.Weight = J->GetNumberField(TEXT("Weight"));
		C.Delay = J->GetNumberField(TEXT("Delay"));
		C.Noise = J->GetNumberField(TEXT("Noise"));
		C.Plasticity = J->GetNumberField(TEXT("Plasticity"));
		C.ReinforceCount = (int32)J->GetNumberField(TEXT("Reinforced"));
		C.bLocked = J->GetBoolField(TEXT("Locked"));
		const int32 DelayTicks = FMath::Clamp(FMath::RoundToInt(C.Delay / FNeuralNetwork::SimDT), 0, 15);
		C.DelayBuffer.SetNumZeroed(DelayTicks + 1);
		return C;
	}

	TSharedRef<FJsonObject> NetToJson(const FNeuralNetwork& Net)
	{
		TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetNumberField(TEXT("NextId"), Net.NextId);
		TArray<TSharedPtr<FJsonValue>> Nodes, Conns;
		for (const FNeuralNode& N : Net.Nodes) { Nodes.Add(MakeShared<FJsonValueObject>(NodeToJson(N))); }
		for (const FNeuralConnection& C : Net.Connections) { Conns.Add(MakeShared<FJsonValueObject>(ConnToJson(C))); }
		J->SetArrayField(TEXT("Nodes"), Nodes);
		J->SetArrayField(TEXT("Conns"), Conns);
		return J;
	}

	void NetFromJson(FNeuralNetwork& Net, const TSharedPtr<FJsonObject>& J)
	{
		Net.Nodes.Reset();
		Net.Connections.Reset();
		Net.NextId = (int32)J->GetNumberField(TEXT("NextId"));
		for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("Nodes")))
		{
			Net.Nodes.Add(NodeFromJson(V->AsObject()));
		}
		for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("Conns")))
		{
			Net.Connections.Add(ConnFromJson(V->AsObject()));
		}
		Net.AssignChannels();
		Net.bCompiledDirty = true;
	}
}

FString ForgeSave::SaveDir()
{
	return FPaths::ProjectSavedDir() / TEXT("Lineages");
}

FString ForgeSave::SlotFileFor(const FString& LineageName)
{
	FString Clean;
	for (TCHAR C : LineageName)
	{
		Clean += (FChar::IsAlnum(C) || C == '_' || C == '-') ? C : TEXT('_');
	}
	if (Clean.IsEmpty()) { Clean = TEXT("lineage"); }
	return Clean.ToLower();
}

bool ForgeSave::SaveSession(const FForgeSession& S, const FString& FileName)
{
	TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetNumberField(TEXT("Version"), 1);
	J->SetNumberField(TEXT("Seed"), S.Seed);
	J->SetStringField(TEXT("LineageName"), S.LineageName);
	J->SetNumberField(TEXT("Phase"), (int32)S.Phase);
	J->SetNumberField(TEXT("Generation"), S.Generation);
	J->SetNumberField(TEXT("TrialCounter"), S.TrialCounter);
	J->SetNumberField(TEXT("TrialsThisGen"), S.TrialsThisGeneration);
	J->SetNumberField(TEXT("Energy"), S.Energy);
	J->SetNumberField(TEXT("EnergyCap"), S.EnergyCap);
	J->SetNumberField(TEXT("FitnessScore"), S.FitnessScore);
	J->SetNumberField(TEXT("BestRun"), S.BestRun);
	J->SetNumberField(TEXT("LearningRateMult"), S.LearningRateMult);
	J->SetNumberField(TEXT("LearningBoostLeft"), S.LearningBoostTrialsLeft);
	J->SetBoolField(TEXT("FreeAwaken"), S.bFreeAwaken);
	J->SetNumberField(TEXT("PermStability"), S.PermStabilityBonus);
	J->SetBoolField(TEXT("EverWon"), S.bEverWon);
	J->SetNumberField(TEXT("TutorialStage"), S.TutorialStage);
	J->SetNumberField(TEXT("PlayerConnections"), S.PlayerConnections);
	J->SetBoolField(TEXT("EnteredRegion"), S.bHasEnteredRegion);
	J->SetStringField(TEXT("SavedAt"), FDateTime::UtcNow().ToIso8601());

	// fitness
	TArray<TSharedPtr<FJsonValue>> Dims, DimsKnown;
	for (int32 i = 0; i < FitnessDimCount(); ++i)
	{
		Dims.Add(MakeShared<FJsonValueNumber>(S.Fitness.Dims[i]));
		DimsKnown.Add(MakeShared<FJsonValueBoolean>(S.Fitness.DimKnown[i]));
	}
	J->SetArrayField(TEXT("FitnessDims"), Dims);
	J->SetArrayField(TEXT("FitnessDimsKnown"), DimsKnown);
	J->SetNumberField(TEXT("StabilityMod"), S.Fitness.StabilityMod);
	J->SetNumberField(TEXT("EfficiencyMod"), S.Fitness.EfficiencyMod);

	// brain regions
	TArray<TSharedPtr<FJsonValue>> Regions;
	for (const FBrainRegion& R : S.Brain.Regions)
	{
		TSharedRef<FJsonObject> RJ = MakeShared<FJsonObject>();
		RJ->SetNumberField(TEXT("Id"), R.Id);
		RJ->SetNumberField(TEXT("Role"), (int32)R.Role);
		RJ->SetStringField(TEXT("HiddenLabel"), R.HiddenLabel);
		RJ->SetNumberField(TEXT("X"), R.Pos.X);
		RJ->SetNumberField(TEXT("Y"), R.Pos.Y);
		RJ->SetBoolField(TEXT("Awake"), R.bAwake);
		RJ->SetNumberField(TEXT("Confidence"), R.DiscoveryConfidence);
		RJ->SetObjectField(TEXT("Net"), NetToJson(R.Net));
		Regions.Add(MakeShared<FJsonValueObject>(RJ));
	}
	J->SetArrayField(TEXT("Regions"), Regions);
	J->SetNumberField(TEXT("BrainNextId"), S.Brain.NextId);

	// region links
	TArray<TSharedPtr<FJsonValue>> Links;
	for (const FRegionLink& L : S.Brain.Links)
	{
		TSharedRef<FJsonObject> LJ = MakeShared<FJsonObject>();
		LJ->SetNumberField(TEXT("Id"), L.Id);
		LJ->SetNumberField(TEXT("Src"), L.Source);
		LJ->SetNumberField(TEXT("Dst"), L.Target);
		LJ->SetNumberField(TEXT("SrcCh"), L.SourceChannel);
		LJ->SetNumberField(TEXT("DstCh"), L.TargetChannel);
		LJ->SetNumberField(TEXT("Weight"), L.Weight);
		LJ->SetBoolField(TEXT("Locked"), L.bLocked);
		Links.Add(MakeShared<FJsonValueObject>(LJ));
	}
	J->SetArrayField(TEXT("Links"), Links);

	// discovery
	TSharedRef<FJsonObject> DJ = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Heur, TrialRuns, Triggered;
	for (const FHeuristic& H : S.Discovery.Heuristics)
	{
		TSharedRef<FJsonObject> HJ = MakeShared<FJsonObject>();
		HJ->SetNumberField(TEXT("Id"), H.Id);
		HJ->SetStringField(TEXT("Text"), H.Text);
		HJ->SetNumberField(TEXT("Confidence"), H.Confidence);
		HJ->SetNumberField(TEXT("Samples"), H.SampleSize);
		HJ->SetNumberField(TEXT("EffBonus"), H.EfficiencyBonus);
		HJ->SetNumberField(TEXT("StabBonus"), H.StabilityBonus);
		HJ->SetStringField(TEXT("Label"), H.BonusLabel);
		Heur.Add(MakeShared<FJsonValueObject>(HJ));
	}
	for (int32 i = 0; i < (int32)ETrialType::COUNT; ++i)
	{
		TrialRuns.Add(MakeShared<FJsonValueNumber>(S.Discovery.TrialTypeRuns[i]));
	}
	for (int32 Id : S.Discovery.TriggeredHeuristicIds)
	{
		Triggered.Add(MakeShared<FJsonValueNumber>(Id));
	}
	DJ->SetArrayField(TEXT("Heuristics"), Heur);
	DJ->SetArrayField(TEXT("TrialRuns"), TrialRuns);
	DJ->SetArrayField(TEXT("Triggered"), Triggered);
	DJ->SetNumberField(TEXT("PruneActions"), S.Discovery.PruneActions);
	DJ->SetNumberField(TEXT("ReinforceActions"), S.Discovery.ReinforceActions);
	DJ->SetNumberField(TEXT("LockActions"), S.Discovery.LockActions);
	DJ->SetNumberField(TEXT("MutateActions"), S.Discovery.MutateActions);
	J->SetObjectField(TEXT("Discovery"), DJ);

	// patterns
	TSharedRef<FJsonObject> PJ = MakeShared<FJsonObject>();
	PJ->SetNumberField(TEXT("SlotCap"), S.Patterns.SlotCap);
	PJ->SetNumberField(TEXT("NextId"), S.Patterns.NextId);
	TArray<TSharedPtr<FJsonValue>> Pats;
	for (const FSavedPattern& P : S.Patterns.Patterns)
	{
		TSharedRef<FJsonObject> PatJ = MakeShared<FJsonObject>();
		PatJ->SetNumberField(TEXT("Id"), P.Id);
		PatJ->SetStringField(TEXT("Name"), P.Name);
		PatJ->SetBoolField(TEXT("Named"), P.bNamed);
		PatJ->SetNumberField(TEXT("UseCount"), P.UseCount);
		TArray<TSharedPtr<FJsonValue>> PN, PC;
		for (const FNeuralNode& N : P.Nodes) { PN.Add(MakeShared<FJsonValueObject>(NodeToJson(N))); }
		for (const FNeuralConnection& C : P.Connections) { PC.Add(MakeShared<FJsonValueObject>(ConnToJson(C))); }
		PatJ->SetArrayField(TEXT("Nodes"), PN);
		PatJ->SetArrayField(TEXT("Conns"), PC);
		Pats.Add(MakeShared<FJsonValueObject>(PatJ));
	}
	PJ->SetArrayField(TEXT("Patterns"), Pats);
	J->SetObjectField(TEXT("PatternLib"), PJ);

	// history (fitness curve + archive log; trial details trimmed)
	TArray<TSharedPtr<FJsonValue>> Curve, Log, Scores;
	for (float V : S.FitnessCurve) { Curve.Add(MakeShared<FJsonValueNumber>(V)); }
	for (const FString& L : S.ArchiveLog) { Log.Add(MakeShared<FJsonValueString>(L)); }
	for (float V : S.RecentScores) { Scores.Add(MakeShared<FJsonValueNumber>(V)); }
	J->SetArrayField(TEXT("FitnessCurve"), Curve);
	J->SetArrayField(TEXT("ArchiveLog"), Log);
	J->SetArrayField(TEXT("RecentScores"), Scores);
	J->SetNumberField(TEXT("MutationCount"), S.MutationCount);
	J->SetNumberField(TEXT("PruneCount"), S.PruneCount);

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(J, Writer);

	IFileManager::Get().MakeDirectory(*SaveDir(), true);
	return FFileHelper::SaveStringToFile(Out, *(SaveDir() / FileName + TEXT(".json")));
}

bool ForgeSave::LoadSession(FForgeSession& S, const FString& FileName)
{
	FString In;
	if (!FFileHelper::LoadFileToString(In, *(SaveDir() / FileName + TEXT(".json")))) { return false; }

	TSharedPtr<FJsonObject> J;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(In);
	if (!FJsonSerializer::Deserialize(Reader, J) || !J.IsValid()) { return false; }

	// bootstrap a fresh session with the stored seed, then overwrite state
	S.NewGame((int32)J->GetNumberField(TEXT("Seed")), J->GetStringField(TEXT("LineageName")));

	S.Phase = (EForgePhase)(int32)J->GetNumberField(TEXT("Phase"));
	S.Generation = (int32)J->GetNumberField(TEXT("Generation"));
	S.TrialCounter = (int32)J->GetNumberField(TEXT("TrialCounter"));
	S.TrialsThisGeneration = (int32)J->GetNumberField(TEXT("TrialsThisGen"));
	S.Energy = J->GetNumberField(TEXT("Energy"));
	S.EnergyCap = J->GetNumberField(TEXT("EnergyCap"));
	S.FitnessScore = J->GetNumberField(TEXT("FitnessScore"));
	S.BestRun = J->GetNumberField(TEXT("BestRun"));
	S.LearningRateMult = J->GetNumberField(TEXT("LearningRateMult"));
	S.LearningBoostTrialsLeft = (int32)J->GetNumberField(TEXT("LearningBoostLeft"));
	S.bFreeAwaken = J->GetBoolField(TEXT("FreeAwaken"));
	S.PermStabilityBonus = J->GetNumberField(TEXT("PermStability"));
	J->TryGetBoolField(TEXT("EverWon"), S.bEverWon);
	{
		double Tmp;
		if (J->TryGetNumberField(TEXT("TutorialStage"), Tmp)) { S.TutorialStage = (int32)Tmp; }
		if (J->TryGetNumberField(TEXT("PlayerConnections"), Tmp)) { S.PlayerConnections = (int32)Tmp; }
	}
	J->TryGetBoolField(TEXT("EnteredRegion"), S.bHasEnteredRegion);

	const TArray<TSharedPtr<FJsonValue>>* Dims;
	if (J->TryGetArrayField(TEXT("FitnessDims"), Dims))
	{
		for (int32 i = 0; i < FitnessDimCount() && i < Dims->Num(); ++i)
		{
			S.Fitness.Dims[i] = (*Dims)[i]->AsNumber();
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* DimsKnown;
	if (J->TryGetArrayField(TEXT("FitnessDimsKnown"), DimsKnown))
	{
		for (int32 i = 0; i < FitnessDimCount() && i < DimsKnown->Num(); ++i)
		{
			S.Fitness.DimKnown[i] = (*DimsKnown)[i]->AsBool();
		}
	}
	S.Fitness.StabilityMod = J->GetNumberField(TEXT("StabilityMod"));
	S.Fitness.EfficiencyMod = J->GetNumberField(TEXT("EfficiencyMod"));

	// regions
	S.Brain.Regions.Reset();
	S.Brain.Links.Reset();
	S.Brain.NextId = (int32)J->GetNumberField(TEXT("BrainNextId"));
	for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("Regions")))
	{
		const TSharedPtr<FJsonObject> RJ = V->AsObject();
		FBrainRegion R;
		R.Id = (int32)RJ->GetNumberField(TEXT("Id"));
		R.Role = (ERegionRole)(int32)RJ->GetNumberField(TEXT("Role"));
		R.HiddenLabel = RJ->GetStringField(TEXT("HiddenLabel"));
		R.Pos.X = RJ->GetNumberField(TEXT("X"));
		R.Pos.Y = RJ->GetNumberField(TEXT("Y"));
		R.bAwake = RJ->GetBoolField(TEXT("Awake"));
		R.DiscoveryConfidence = RJ->GetNumberField(TEXT("Confidence"));
		NetFromJson(R.Net, RJ->GetObjectField(TEXT("Net")));
		S.Brain.Regions.Add(MoveTemp(R));
	}
	for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("Links")))
	{
		const TSharedPtr<FJsonObject> LJ = V->AsObject();
		FRegionLink L;
		L.Id = (int32)LJ->GetNumberField(TEXT("Id"));
		L.Source = (int32)LJ->GetNumberField(TEXT("Src"));
		L.Target = (int32)LJ->GetNumberField(TEXT("Dst"));
		L.SourceChannel = (int32)LJ->GetNumberField(TEXT("SrcCh"));
		L.TargetChannel = (int32)LJ->GetNumberField(TEXT("DstCh"));
		L.Weight = LJ->GetNumberField(TEXT("Weight"));
		L.bLocked = LJ->GetBoolField(TEXT("Locked"));
		S.Brain.Links.Add(L);
	}

	// discovery
	const TSharedPtr<FJsonObject> DJ = J->GetObjectField(TEXT("Discovery"));
	S.Discovery.Heuristics.Reset();
	for (const TSharedPtr<FJsonValue>& V : DJ->GetArrayField(TEXT("Heuristics")))
	{
		const TSharedPtr<FJsonObject> HJ = V->AsObject();
		FHeuristic H;
		H.Id = (int32)HJ->GetNumberField(TEXT("Id"));
		H.Text = HJ->GetStringField(TEXT("Text"));
		H.Confidence = HJ->GetNumberField(TEXT("Confidence"));
		H.SampleSize = (int32)HJ->GetNumberField(TEXT("Samples"));
		H.EfficiencyBonus = HJ->GetNumberField(TEXT("EffBonus"));
		H.StabilityBonus = HJ->GetNumberField(TEXT("StabBonus"));
		H.BonusLabel = HJ->GetStringField(TEXT("Label"));
		S.Discovery.Heuristics.Add(H);
	}
	{
		const TArray<TSharedPtr<FJsonValue>>& Runs = DJ->GetArrayField(TEXT("TrialRuns"));
		for (int32 i = 0; i < (int32)ETrialType::COUNT && i < Runs.Num(); ++i)
		{
			S.Discovery.TrialTypeRuns[i] = (int32)Runs[i]->AsNumber();
		}
		for (const TSharedPtr<FJsonValue>& V : DJ->GetArrayField(TEXT("Triggered")))
		{
			S.Discovery.TriggeredHeuristicIds.Add((int32)V->AsNumber());
		}
	}
	S.Discovery.PruneActions = (int32)DJ->GetNumberField(TEXT("PruneActions"));
	S.Discovery.ReinforceActions = (int32)DJ->GetNumberField(TEXT("ReinforceActions"));
	S.Discovery.LockActions = (int32)DJ->GetNumberField(TEXT("LockActions"));
	S.Discovery.MutateActions = (int32)DJ->GetNumberField(TEXT("MutateActions"));

	// patterns
	const TSharedPtr<FJsonObject> PJ = J->GetObjectField(TEXT("PatternLib"));
	S.Patterns.SlotCap = (int32)PJ->GetNumberField(TEXT("SlotCap"));
	S.Patterns.NextId = (int32)PJ->GetNumberField(TEXT("NextId"));
	S.Patterns.Patterns.Reset();
	for (const TSharedPtr<FJsonValue>& V : PJ->GetArrayField(TEXT("Patterns")))
	{
		const TSharedPtr<FJsonObject> PatJ = V->AsObject();
		FSavedPattern P;
		P.Id = (int32)PatJ->GetNumberField(TEXT("Id"));
		P.Name = PatJ->GetStringField(TEXT("Name"));
		P.bNamed = PatJ->GetBoolField(TEXT("Named"));
		P.UseCount = (int32)PatJ->GetNumberField(TEXT("UseCount"));
		for (const TSharedPtr<FJsonValue>& NV : PatJ->GetArrayField(TEXT("Nodes"))) { P.Nodes.Add(NodeFromJson(NV->AsObject())); }
		for (const TSharedPtr<FJsonValue>& CV : PatJ->GetArrayField(TEXT("Conns"))) { P.Connections.Add(ConnFromJson(CV->AsObject())); }
		S.Patterns.Patterns.Add(MoveTemp(P));
	}

	// history
	S.FitnessCurve.Reset();
	for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("FitnessCurve"))) { S.FitnessCurve.Add(V->AsNumber()); }
	S.ArchiveLog.Reset();
	for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("ArchiveLog"))) { S.ArchiveLog.Add(V->AsString()); }
	S.RecentScores.Reset();
	for (const TSharedPtr<FJsonValue>& V : J->GetArrayField(TEXT("RecentScores"))) { S.RecentScores.Add(V->AsNumber()); }
	S.MutationCount = (int32)J->GetNumberField(TEXT("MutationCount"));
	S.PruneCount = (int32)J->GetNumberField(TEXT("PruneCount"));

	S.EventQueue.Reset();
	S.PendingOffers.Reset();
	S.bWon = false;
	S.OpenRegionId = INDEX_NONE;
	S.Speed = 1;

	return true;
}

TArray<FSaveSlotInfo> ForgeSave::ListSlots()
{
	TArray<FSaveSlotInfo> Result;
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(SaveDir() / TEXT("*.json")), true, false);

	for (const FString& File : Files)
	{
		FString In;
		if (!FFileHelper::LoadFileToString(In, *(SaveDir() / File))) { continue; }
		TSharedPtr<FJsonObject> J;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(In);
		if (!FJsonSerializer::Deserialize(Reader, J) || !J.IsValid()) { continue; }

		FSaveSlotInfo Info;
		Info.FileName = FPaths::GetBaseFilename(File);
		Info.LineageName = J->GetStringField(TEXT("LineageName"));
		Info.Seed = (int32)J->GetNumberField(TEXT("Seed"));
		Info.Generation = (int32)J->GetNumberField(TEXT("Generation"));
		Info.TrialCount = (int32)J->GetNumberField(TEXT("TrialCounter"));

		// derive display fitness from stored dims
		FFitnessState F;
		const TArray<TSharedPtr<FJsonValue>>* Dims;
		if (J->TryGetArrayField(TEXT("FitnessDims"), Dims))
		{
			for (int32 i = 0; i < FitnessDimCount() && i < Dims->Num(); ++i) { F.Dims[i] = (*Dims)[i]->AsNumber(); }
		}
		F.StabilityMod = J->GetNumberField(TEXT("StabilityMod"));
		F.EfficiencyMod = J->GetNumberField(TEXT("EfficiencyMod"));
		Info.Fitness = FMath::Min(100.f, F.Overall() * 118.f);

		FString SavedAt;
		if (J->TryGetStringField(TEXT("SavedAt"), SavedAt))
		{
			FDateTime::ParseIso8601(*SavedAt, Info.Timestamp);
		}
		Result.Add(Info);
	}

	Result.Sort([](const FSaveSlotInfo& A, const FSaveSlotInfo& B) { return A.Timestamp > B.Timestamp; });
	return Result;
}

bool ForgeSave::DeleteSlot(const FString& FileName)
{
	return IFileManager::Get().Delete(*(SaveDir() / FileName + TEXT(".json")));
}
