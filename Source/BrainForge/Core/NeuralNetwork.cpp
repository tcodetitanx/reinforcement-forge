#include "NeuralNetwork.h"
#include "Math/RandomStream.h"

const TCHAR* FitnessDimName(EFitnessDim Dim)
{
	switch (Dim)
	{
	case EFitnessDim::Survival:      return TEXT("Survival");
	case EFitnessDim::Perception:    return TEXT("Perception");
	case EFitnessDim::Mobility:      return TEXT("Mobility");
	case EFitnessDim::Regulation:    return TEXT("Regulation");
	case EFitnessDim::Memory:        return TEXT("Memory");
	case EFitnessDim::Communication: return TEXT("Communication");
	case EFitnessDim::Social:        return TEXT("Social Function");
	case EFitnessDim::Planning:      return TEXT("Planning");
	case EFitnessDim::Adaptability:  return TEXT("Adaptability");
	default:                         return TEXT("Unknown");
	}
}

const TCHAR* PhaseName(EForgePhase Phase)
{
	switch (Phase)
	{
	case EForgePhase::Reflex:      return TEXT("Phase I - Reflex");
	case EForgePhase::Perception:  return TEXT("Phase II - Perception");
	case EForgePhase::MemoryPhase: return TEXT("Phase III - Memory");
	case EForgePhase::Social:      return TEXT("Phase IV - Social Intelligence");
	case EForgePhase::Language:    return TEXT("Phase V - Language");
	case EForgePhase::Cognition:   return TEXT("Phase VI - Human Cognition");
	default:                       return TEXT("Unknown Phase");
	}
}

const TCHAR* NodeTypeName(ENodeType Type)
{
	switch (Type)
	{
	case ENodeType::Input:      return TEXT("Input");
	case ENodeType::Relay:      return TEXT("Relay");
	case ENodeType::Threshold:  return TEXT("Threshold");
	case ENodeType::Memory:     return TEXT("Memory");
	case ENodeType::Oscillator: return TEXT("Oscillator");
	case ENodeType::Inhibitor:  return TEXT("Inhibitor");
	case ENodeType::Reward:     return TEXT("Reward");
	case ENodeType::Output:     return TEXT("Output");
	default:                    return TEXT("Unknown");
	}
}

const TCHAR* ConnTypeName(EConnType Type)
{
	switch (Type)
	{
	case EConnType::Excitatory: return TEXT("Excitatory");
	case EConnType::Inhibitory: return TEXT("Inhibitory");
	case EConnType::Modulatory: return TEXT("Modulatory");
	case EConnType::Memory:     return TEXT("Memory");
	case EConnType::Unstable:   return TEXT("Unstable");
	case EConnType::Delayed:    return TEXT("Delayed");
	default:                    return TEXT("Unknown");
	}
}

FNeuralNode* FNeuralNetwork::FindNode(int32 Id)
{
	for (FNeuralNode& N : Nodes) { if (N.Id == Id) return &N; }
	return nullptr;
}

const FNeuralNode* FNeuralNetwork::FindNode(int32 Id) const
{
	for (const FNeuralNode& N : Nodes) { if (N.Id == Id) return &N; }
	return nullptr;
}

FNeuralConnection* FNeuralNetwork::FindConnection(int32 Id)
{
	for (FNeuralConnection& C : Connections) { if (C.Id == Id) return &C; }
	return nullptr;
}

int32 FNeuralNetwork::NodeIndex(int32 Id) const
{
	for (int32 i = 0; i < Nodes.Num(); ++i) { if (Nodes[i].Id == Id) return i; }
	return INDEX_NONE;
}

FNeuralNode& FNeuralNetwork::AddNode(ENodeType Type, const FVector2D& Pos, FRandomStream& Rng)
{
	FNeuralNode N;
	N.Id = NextId++;
	N.Type = Type;
	N.Pos = Pos;
	N.Bias = Rng.FRandRange(-0.15f, 0.25f);
	N.Threshold = Rng.FRandRange(0.25f, 0.6f);
	N.Decay = Rng.FRandRange(0.05f, 0.2f);
	N.Noise = Rng.FRandRange(0.01f, 0.08f);
	N.Plasticity = Rng.FRandRange(0.3f, 0.8f);
	N.EnergyCost = Rng.FRandRange(0.06f, 0.2f);

	switch (Type)
	{
	case ENodeType::Memory:
		N.Decay = Rng.FRandRange(0.005f, 0.03f);
		N.EnergyCost *= 1.6f;
		break;
	case ENodeType::Oscillator:
		N.OscPhase = Rng.FRandRange(0.f, 6.28f);
		break;
	case ENodeType::Inhibitor:
		N.Bias = Rng.FRandRange(0.f, 0.3f);
		break;
	case ENodeType::Reward:
		N.EnergyCost *= 2.f;
		N.Threshold = Rng.FRandRange(0.4f, 0.7f);
		break;
	default:
		break;
	}

	bCompiledDirty = true;
	Nodes.Add(N);
	AssignChannels();
	return Nodes.Last();
}

FNeuralConnection* FNeuralNetwork::AddConnection(int32 Source, int32 Target, EConnType Type, float Weight)
{
	if (Source == Target || !FindNode(Source) || !FindNode(Target) || HasConnection(Source, Target))
	{
		return nullptr;
	}
	FNeuralConnection C;
	C.Id = NextId++;
	C.Source = Source;
	C.Target = Target;
	C.Type = Type;
	C.Weight = FMath::Clamp(Weight, -2.f, 2.f);
	C.Delay = (Type == EConnType::Delayed) ? 0.4f : 0.f;
	const int32 DelayTicks = FMath::Clamp(FMath::RoundToInt(C.Delay / SimDT), 0, 15);
	C.DelayBuffer.SetNumZeroed(DelayTicks + 1);
	bCompiledDirty = true;
	Connections.Add(C);
	return &Connections.Last();
}

bool FNeuralNetwork::RemoveNode(int32 Id)
{
	const int32 Idx = NodeIndex(Id);
	if (Idx == INDEX_NONE) { return false; }
	Nodes.RemoveAt(Idx);
	Connections.RemoveAll([Id](const FNeuralConnection& C) { return C.Source == Id || C.Target == Id; });
	AssignChannels();
	bCompiledDirty = true;
	return true;
}

bool FNeuralNetwork::RemoveConnection(int32 Id)
{
	const int32 Removed = Connections.RemoveAll([Id](const FNeuralConnection& C) { return C.Id == Id; });
	if (Removed > 0) { bCompiledDirty = true; }
	return Removed > 0;
}

bool FNeuralNetwork::HasConnection(int32 Source, int32 Target) const
{
	for (const FNeuralConnection& C : Connections)
	{
		if (C.Source == Source && C.Target == Target) { return true; }
	}
	return false;
}

void FNeuralNetwork::AssignChannels()
{
	int32 InCh = 0, OutCh = 0;
	for (FNeuralNode& N : Nodes)
	{
		if (N.Type == ENodeType::Input)       { N.Channel = InCh++ % MaxChannels; }
		else if (N.Type == ENodeType::Output) { N.Channel = OutCh++ % MaxChannels; }
		else                                  { N.Channel = INDEX_NONE; }
	}
}

float FNeuralNetwork::TotalActivation() const
{
	float Sum = 0.f;
	for (const FNeuralNode& N : Nodes) { Sum += FMath::Abs(N.Activation); }
	return Sum;
}

static float ApplyActivationFn(ENodeType Type, float X)
{
	switch (Type)
	{
	case ENodeType::Threshold:  return X > 0.f ? FMath::Min(1.f, X) : 0.f;
	case ENodeType::Inhibitor:  return FMath::Clamp(X, 0.f, 1.f);
	case ENodeType::Relay:      return FMath::Clamp(X, -1.f, 1.f);
	default:                    return FMath::Clamp(2.f / (1.f + FMath::Exp(-2.f * X)) - 1.f, -1.f, 1.f); // tanh-ish
	}
}

void FNeuralNetwork::Tick(FRandomStream& Rng)
{
	// 1. store previous activations
	for (FNeuralNode& N : Nodes)
	{
		N.PrevActivation = N.Activation;
	}

	// 2. compute reward broadcast (plasticity modulation) from reward nodes
	float RewardBroadcast = 0.f;
	for (const FNeuralNode& N : Nodes)
	{
		if (N.Type == ENodeType::Reward) { RewardBroadcast += N.PrevActivation; }
	}

	// 3. gather inputs through connections (with delay buffers)
	TArray<float, TInlineAllocator<64>> InputSum;
	TArray<float, TInlineAllocator<64>> ModSum;
	InputSum.SetNumZeroed(Nodes.Num());
	ModSum.SetNumZeroed(Nodes.Num());

	TMap<int32, int32> IdToIdx;
	IdToIdx.Reserve(Nodes.Num());
	for (int32 i = 0; i < Nodes.Num(); ++i) { IdToIdx.Add(Nodes[i].Id, i); }

	for (FNeuralConnection& C : Connections)
	{
		const int32* SrcIdx = IdToIdx.Find(C.Source);
		const int32* DstIdx = IdToIdx.Find(C.Target);
		if (!SrcIdx || !DstIdx) { continue; }

		float Src = Nodes[*SrcIdx].PrevActivation;

		// delay ring buffer
		if (C.DelayBuffer.Num() > 1)
		{
			C.DelayBuffer[C.BufHead] = Src;
			C.BufHead = (C.BufHead + 1) % C.DelayBuffer.Num();
			Src = C.DelayBuffer[C.BufHead];
		}

		float Signal = Src * C.Weight;
		Signal += Rng.FRandRange(-C.Noise, C.Noise);

		switch (C.Type)
		{
		case EConnType::Inhibitory:
			Signal = -FMath::Abs(Signal);
			break;
		case EConnType::Unstable:
			if (Rng.FRand() < 0.18f) { Signal = 0.f; }
			else if (Rng.FRand() < 0.06f) { Signal *= 2.5f; }
			break;
		case EConnType::Memory:
			// memory connections transmit a slow blend of past signal
			C.LastSignal = FMath::Lerp(C.LastSignal, Signal, 0.15f);
			Signal = C.LastSignal;
			break;
		case EConnType::Modulatory:
			ModSum[*DstIdx] += Signal;
			C.LastSignal = Signal;
			C.Traffic = FMath::Lerp(C.Traffic, FMath::Abs(Signal), 0.08f);
			continue; // does not feed activation
		default:
			break;
		}

		C.LastSignal = Signal;
		C.Traffic = FMath::Lerp(C.Traffic, FMath::Abs(Signal), 0.08f);
		InputSum[*DstIdx] += Signal;
	}

	// 4. update nodes
	float Energy = 0.f;
	float Activity = 0.f;
	float Jerk = 0.f; // sum of activation change - instability measure

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		FNeuralNode& N = Nodes[i];
		float Total = InputSum[i] + N.Bias + Rng.FRandRange(-N.Noise, N.Noise);

		switch (N.Type)
		{
		case ENodeType::Input:
			N.Activation = FMath::Clamp((N.Channel >= 0 ? InputValues[N.Channel] : 0.f) + Rng.FRandRange(-N.Noise, N.Noise), -1.f, 1.f);
			break;

		case ENodeType::Oscillator:
			N.OscPhase += SimDT * (2.f + 4.f * FMath::Clamp(Total, 0.f, 1.f));
			N.Activation = (FMath::Sin(N.OscPhase) * 0.5f + 0.5f) * FMath::Clamp(0.4f + Total, 0.f, 1.f);
			break;

		case ENodeType::Memory:
			if (Total >= N.Threshold)
			{
				N.MemoryCharge = FMath::Clamp(N.MemoryCharge + Total * 0.35f, 0.f, 1.5f);
			}
			N.MemoryCharge *= (1.f - N.Decay);
			N.Activation = FMath::Clamp(N.MemoryCharge, 0.f, 1.f);
			break;

		case ENodeType::Inhibitor:
			N.Activation = (Total >= N.Threshold) ? ApplyActivationFn(N.Type, Total) : 0.f;
			break;

		default:
			if (Total >= N.Threshold)
			{
				N.Activation = ApplyActivationFn(N.Type, Total);
			}
			else
			{
				N.Activation *= (1.f - N.Decay * 3.f);
				if (FMath::Abs(N.Activation) < 0.01f) { N.Activation = 0.f; }
			}
			break;
		}

		if (N.Type != ENodeType::Memory && N.Type != ENodeType::Input)
		{
			N.Activation *= (1.f - N.Decay * 0.5f);
		}

		// modulatory input + reward broadcast raise effective plasticity transiently
		const float PlasticityBoost = FMath::Clamp(ModSum[i] + RewardBroadcast * 0.3f, -0.5f, 1.f);
		N.Plasticity = FMath::Clamp(N.Plasticity + PlasticityBoost * 0.002f, 0.05f, 1.f);

		if (FMath::Abs(N.Activation) > 0.55f) { N.PulseTime = 0.f; }
		else { N.PulseTime += SimDT; }

		N.AvgActivity = FMath::Lerp(N.AvgActivity, FMath::Abs(N.Activation), 0.05f);

		Energy += N.EnergyCost * FMath::Abs(N.Activation);
		Activity += FMath::Abs(N.Activation);
		Jerk += FMath::Abs(N.Activation - N.PrevActivation);
	}

	// 5. outputs
	for (int32 c = 0; c < MaxChannels; ++c) { OutputValues[c] = 0.f; }
	int32 OutCounts[MaxChannels] = {0, 0, 0, 0};
	for (const FNeuralNode& N : Nodes)
	{
		if (N.Type == ENodeType::Output && N.Channel >= 0)
		{
			OutputValues[N.Channel] += N.Activation;
			OutCounts[N.Channel]++;
		}
	}
	for (int32 c = 0; c < MaxChannels; ++c)
	{
		if (OutCounts[c] > 0) { OutputValues[c] = FMath::Clamp(OutputValues[c] / OutCounts[c], -1.f, 1.f); }
	}

	// 6. stats
	const int32 NumN = FMath::Max(1, Nodes.Num());
	EnergyUse = FMath::Lerp(EnergyUse, Energy, 0.05f);
	ActivityLevel = FMath::Lerp(ActivityLevel, Activity / NumN, 0.05f);
	const float Instability = FMath::Clamp(Jerk / NumN * 2.2f, 0.f, 1.f);
	Stability = FMath::Clamp(FMath::Lerp(Stability, 1.f - Instability, 0.03f), 0.02f, 1.f);
}

void FNeuralNetwork::EvaluateCompiled(FRandomStream& Rng)
{
	if (Compiled.Num() == 0)
	{
		for (int32 c = 0; c < MaxChannels; ++c) { OutputValues[c] = 0.f; }
		return;
	}

	// inverse-distance blend of the two nearest samples
	int32 BestA = 0, BestB = 0;
	float DA = FLT_MAX, DB = FLT_MAX;
	for (int32 s = 0; s < Compiled.Num(); ++s)
	{
		float D = 0.f;
		for (int32 c = 0; c < MaxChannels; ++c)
		{
			const float Diff = Compiled[s].In[c] - InputValues[c];
			D += Diff * Diff;
		}
		if (D < DA) { DB = DA; BestB = BestA; DA = D; BestA = s; }
		else if (D < DB) { DB = D; BestB = s; }
	}

	const float WA = 1.f / (DA + 0.01f);
	const float WB = 1.f / (DB + 0.01f);
	const float NoiseAmt = (1.f - Stability) * 0.08f;
	for (int32 c = 0; c < MaxChannels; ++c)
	{
		OutputValues[c] = (Compiled[BestA].Out[c] * WA + Compiled[BestB].Out[c] * WB) / (WA + WB);
		OutputValues[c] = FMath::Clamp(OutputValues[c] + Rng.FRandRange(-NoiseAmt, NoiseAmt), -1.f, 1.f);
	}
}

void FNeuralNetwork::Compile(FRandomStream& Rng)
{
	// preserve live state
	FNeuralNetwork Backup;
	TArray<float> SavedAct;
	SavedAct.Reserve(Nodes.Num());
	for (const FNeuralNode& N : Nodes) { SavedAct.Add(N.Activation); }
	float SavedIn[MaxChannels];
	FMemory::Memcpy(SavedIn, InputValues, sizeof(SavedIn));

	Compiled.Reset();
	const int32 NumSamples = 12;
	const int32 SettleTicks = 24;

	for (int32 s = 0; s < NumSamples; ++s)
	{
		FCompiledSample Sample;
		for (int32 c = 0; c < MaxChannels; ++c)
		{
			Sample.In[c] = (s == 0) ? 0.f : Rng.FRandRange(-0.2f, 1.f);
			InputValues[c] = Sample.In[c];
		}
		ResetState();
		float Acc[MaxChannels] = {0, 0, 0, 0};
		for (int32 t = 0; t < SettleTicks; ++t)
		{
			Tick(Rng);
			if (t >= SettleTicks / 2)
			{
				for (int32 c = 0; c < MaxChannels; ++c) { Acc[c] += OutputValues[c]; }
			}
		}
		for (int32 c = 0; c < MaxChannels; ++c)
		{
			Sample.Out[c] = Acc[c] / (SettleTicks / 2);
		}
		Compiled.Add(Sample);
	}

	// restore live state
	FMemory::Memcpy(InputValues, SavedIn, sizeof(SavedIn));
	for (int32 i = 0; i < Nodes.Num() && i < SavedAct.Num(); ++i) { Nodes[i].Activation = SavedAct[i]; }
	bCompiledDirty = false;
}

void FNeuralNetwork::ResetState()
{
	for (FNeuralNode& N : Nodes)
	{
		N.Activation = 0.f;
		N.PrevActivation = 0.f;
		N.MemoryCharge = 0.f;
	}
	for (FNeuralConnection& C : Connections)
	{
		for (float& V : C.DelayBuffer) { V = 0.f; }
		C.LastSignal = 0.f;
	}
}

void FNeuralNetwork::Reinforce(float Reward, float LearningRate, bool bOnlyPositiveTraffic)
{
	for (FNeuralConnection& C : Connections)
	{
		if (C.bLocked) { continue; }
		if (bOnlyPositiveTraffic && C.Traffic < 0.05f) { continue; }

		const FNeuralNode* Src = FindNode(C.Source);
		const FNeuralNode* Dst = FindNode(C.Target);
		if (!Src || !Dst) { continue; }

		const bool bSrcActive = Src->AvgActivity > 0.15f;
		const bool bDstActive = Dst->AvgActivity > 0.15f;

		if (bSrcActive && bDstActive && Reward > 0.f)
		{
			C.Weight += LearningRate * Reward * C.Plasticity;
			C.ReinforceCount++;
		}
		else if (bSrcActive && !bDstActive && Reward < 0.f)
		{
			C.Weight -= LearningRate * FMath::Abs(Reward) * C.Plasticity;
		}
		C.Weight = FMath::Clamp(C.Weight, -2.f, 2.f);
	}
	bCompiledDirty = true;
}

int32 FNeuralNetwork::PruneWeakest(int32 MaxCount, float TrafficThreshold)
{
	TArray<TPair<float, int32>> Ranked; // score, conn id
	for (const FNeuralConnection& C : Connections)
	{
		if (C.bLocked) { continue; }
		const float Score = C.Traffic * FMath::Abs(C.Weight);
		if (Score < TrafficThreshold)
		{
			Ranked.Emplace(Score, C.Id);
		}
	}
	Ranked.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; });

	int32 Removed = 0;
	for (int32 i = 0; i < Ranked.Num() && Removed < MaxCount; ++i)
	{
		// never orphan the graph completely
		if (Connections.Num() <= 2) { break; }
		if (RemoveConnection(Ranked[i].Value)) { Removed++; }
	}
	return Removed;
}

void FNeuralNetwork::Randomize(FRandomStream& Rng, float Intensity)
{
	for (FNeuralConnection& C : Connections)
	{
		if (C.bLocked) { continue; }
		C.Weight = FMath::Clamp(C.Weight + Rng.FRandRange(-Intensity, Intensity), -2.f, 2.f);
	}
	for (FNeuralNode& N : Nodes)
	{
		if (N.bLocked) { continue; }
		N.Threshold = FMath::Clamp(N.Threshold + Rng.FRandRange(-Intensity, Intensity) * 0.4f, 0.05f, 0.95f);
		N.Bias = FMath::Clamp(N.Bias + Rng.FRandRange(-Intensity, Intensity) * 0.3f, -0.6f, 0.6f);
	}
	bCompiledDirty = true;
}

void BuildStarterNetwork(FNeuralNetwork& Net, FRandomStream& Rng, int32 Complexity)
{
	Net.Nodes.Reset();
	Net.Connections.Reset();
	Net.NextId = 1;

	const float W = 900.f, H = 560.f;

	// input column
	TArray<int32> InputIds, HiddenIds, OutputIds;
	const int32 NumIn = MaxChannels;
	const int32 NumOut = MaxChannels;
	const int32 NumHidden = FMath::Clamp(Complexity, 4, 14);

	for (int32 i = 0; i < NumIn; ++i)
	{
		FNeuralNode& N = Net.AddNode(ENodeType::Input, FVector2D(60.f, 80.f + i * (H - 120.f) / (NumIn - 1)), Rng);
		InputIds.Add(N.Id);
	}
	for (int32 i = 0; i < NumHidden; ++i)
	{
		const float R = Rng.FRand();
		ENodeType T = ENodeType::Relay;
		if (R < 0.22f)      { T = ENodeType::Threshold; }
		else if (R < 0.36f) { T = ENodeType::Memory; }
		else if (R < 0.46f) { T = ENodeType::Inhibitor; }
		else if (R < 0.54f) { T = ENodeType::Oscillator; }
		else if (R < 0.60f) { T = ENodeType::Reward; }

		FNeuralNode& N = Net.AddNode(T, FVector2D(Rng.FRandRange(200.f, W - 200.f), Rng.FRandRange(60.f, H - 40.f)), Rng);
		HiddenIds.Add(N.Id);
	}
	for (int32 i = 0; i < NumOut; ++i)
	{
		FNeuralNode& N = Net.AddNode(ENodeType::Output, FVector2D(W - 60.f, 80.f + i * (H - 120.f) / (NumOut - 1)), Rng);
		OutputIds.Add(N.Id);
	}

	auto RandomConnType = [&Rng]() -> EConnType
	{
		const float R = Rng.FRand();
		if (R < 0.62f) { return EConnType::Excitatory; }
		if (R < 0.76f) { return EConnType::Inhibitory; }
		if (R < 0.84f) { return EConnType::Memory; }
		if (R < 0.90f) { return EConnType::Modulatory; }
		if (R < 0.96f) { return EConnType::Delayed; }
		return EConnType::Unstable;
	};

	// wire inputs -> hidden
	for (int32 InId : InputIds)
	{
		const int32 Fan = Rng.RandRange(1, 2);
		for (int32 f = 0; f < Fan; ++f)
		{
			Net.AddConnection(InId, HiddenIds[Rng.RandRange(0, HiddenIds.Num() - 1)], RandomConnType(), Rng.FRandRange(0.3f, 1.1f));
		}
	}
	// hidden -> hidden sparse
	const int32 CrossLinks = NumHidden;
	for (int32 i = 0; i < CrossLinks; ++i)
	{
		Net.AddConnection(
			HiddenIds[Rng.RandRange(0, HiddenIds.Num() - 1)],
			HiddenIds[Rng.RandRange(0, HiddenIds.Num() - 1)],
			RandomConnType(), Rng.FRandRange(-0.6f, 1.f));
	}
	// hidden -> outputs
	for (int32 OutId : OutputIds)
	{
		const int32 Fan = Rng.RandRange(1, 2);
		for (int32 f = 0; f < Fan; ++f)
		{
			Net.AddConnection(HiddenIds[Rng.RandRange(0, HiddenIds.Num() - 1)], OutId, RandomConnType(), Rng.FRandRange(0.3f, 1.f));
		}
	}

	Net.AssignChannels();
	Net.bCompiledDirty = true;
}
