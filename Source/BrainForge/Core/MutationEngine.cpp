#include "MutationEngine.h"
#include "Math/RandomStream.h"

namespace
{
	enum class EMutOp : uint8
	{
		ChangeWeight, AddConnection, RemoveConnection, ChangeThreshold,
		AddNode, RemoveNode, DuplicateMotif, InvertConnection, ChangeDelay, ChangeNoise
	};

	EMutOp PickOp(FRandomStream& Rng)
	{
		const float R = Rng.FRand();
		if (R < 0.30f) { return EMutOp::ChangeWeight; }
		if (R < 0.50f) { return EMutOp::AddConnection; }
		if (R < 0.62f) { return EMutOp::RemoveConnection; }
		if (R < 0.74f) { return EMutOp::ChangeThreshold; }
		if (R < 0.82f) { return EMutOp::AddNode; }
		if (R < 0.87f) { return EMutOp::RemoveNode; }
		if (R < 0.92f) { return EMutOp::DuplicateMotif; }
		if (R < 0.96f) { return EMutOp::InvertConnection; }
		if (R < 0.98f) { return EMutOp::ChangeDelay; }
		return EMutOp::ChangeNoise;
	}

	FNeuralConnection* RandomUnlockedConn(FNeuralNetwork& Net, FRandomStream& Rng)
	{
		TArray<int32> Candidates;
		for (int32 i = 0; i < Net.Connections.Num(); ++i)
		{
			if (!Net.Connections[i].bLocked) { Candidates.Add(i); }
		}
		if (Candidates.Num() == 0) { return nullptr; }
		return &Net.Connections[Candidates[Rng.RandRange(0, Candidates.Num() - 1)]];
	}

	FNeuralNode* RandomUnlockedHidden(FNeuralNetwork& Net, FRandomStream& Rng)
	{
		TArray<int32> Candidates;
		for (int32 i = 0; i < Net.Nodes.Num(); ++i)
		{
			const FNeuralNode& N = Net.Nodes[i];
			if (!N.bLocked && N.Type != ENodeType::Input && N.Type != ENodeType::Output) { Candidates.Add(i); }
		}
		if (Candidates.Num() == 0) { return nullptr; }
		return &Net.Nodes[Candidates[Rng.RandRange(0, Candidates.Num() - 1)]];
	}
}

FMutationPreview MutationEngine::Preview(const FNeuralNetwork& Net, float Intensity, float Knowledge, FRandomStream& Rng)
{
	FMutationPreview P;
	const int32 TrueCount = FMath::Max(1, FMath::RoundToInt(Intensity * 8.f));

	// early game: estimates are fuzzy (guide: "Early in the game, estimates should be poor")
	const int32 Fuzz = FMath::RoundToInt((1.f - Knowledge) * 3.f);
	P.NodesAffected = FMath::Max(1, TrueCount / 2 + Rng.RandRange(-Fuzz, Fuzz));
	P.ConnectionsChanged = FMath::Max(1, TrueCount + Rng.RandRange(-Fuzz, Fuzz));

	if (Knowledge < 0.25f)
	{
		P.EnergyEstimate = TEXT("Unknown");
		P.StabilityEstimate = TEXT("Unknown");
	}
	else if (Knowledge < 0.6f)
	{
		P.EnergyEstimate = FString::Printf(TEXT("%+d%% to %+d%%"), Rng.RandRange(-2, 6), Rng.RandRange(7, 14));
		P.StabilityEstimate = TEXT("Uncertain");
	}
	else
	{
		P.EnergyEstimate = FString::Printf(TEXT("~%+d%%"), Rng.RandRange(1, 8));
		P.StabilityEstimate = FString::Printf(TEXT("~%+d%%"), Rng.RandRange(-6, 3));
	}
	return P;
}

TArray<FString> MutationEngine::Mutate(FNeuralNetwork& Net, float Intensity, FRandomStream& Rng, int32* OutChangedCount)
{
	TArray<FString> Lines;
	const int32 Count = FMath::Max(1, FMath::RoundToInt(Intensity * 8.f));
	int32 Changed = 0;

	for (int32 i = 0; i < Count; ++i)
	{
		switch (PickOp(Rng))
		{
		case EMutOp::ChangeWeight:
			if (FNeuralConnection* C = RandomUnlockedConn(Net, Rng))
			{
				C->Weight = FMath::Clamp(C->Weight + Rng.FRandRange(-0.5f, 0.5f), -2.f, 2.f);
				Lines.Add(TEXT("A synaptic weight drifted."));
				Changed++;
			}
			break;

		case EMutOp::AddConnection:
			if (Net.Nodes.Num() >= 2)
			{
				const int32 A = Net.Nodes[Rng.RandRange(0, Net.Nodes.Num() - 1)].Id;
				const int32 B = Net.Nodes[Rng.RandRange(0, Net.Nodes.Num() - 1)].Id;
				const EConnType T = Rng.FRand() < 0.7f ? EConnType::Excitatory
					: (Rng.FRand() < 0.5f ? EConnType::Inhibitory : EConnType::Unstable);
				if (Net.AddConnection(A, B, T, Rng.FRandRange(-0.8f, 1.f)))
				{
					Lines.Add(TEXT("A new pathway sprouted."));
					Changed++;
				}
			}
			break;

		case EMutOp::RemoveConnection:
			if (Net.Connections.Num() > 3)
			{
				if (FNeuralConnection* C = RandomUnlockedConn(Net, Rng))
				{
					Net.RemoveConnection(C->Id);
					Lines.Add(TEXT("A pathway withered."));
					Changed++;
				}
			}
			break;

		case EMutOp::ChangeThreshold:
			if (FNeuralNode* N = RandomUnlockedHidden(Net, Rng))
			{
				N->Threshold = FMath::Clamp(N->Threshold + Rng.FRandRange(-0.25f, 0.25f), 0.05f, 0.95f);
				Lines.Add(TEXT("A firing threshold shifted."));
				Changed++;
			}
			break;

		case EMutOp::AddNode:
			if (Net.Nodes.Num() < 26)
			{
				const float R = Rng.FRand();
				ENodeType T = R < 0.4f ? ENodeType::Relay : (R < 0.6f ? ENodeType::Threshold : (R < 0.75f ? ENodeType::Memory : (R < 0.9f ? ENodeType::Inhibitor : ENodeType::Oscillator)));
				const FNeuralNode& NewN = Net.AddNode(T, FVector2D(Rng.FRandRange(200.f, 720.f), Rng.FRandRange(80.f, 520.f)), Rng);
				// wire it in loosely
				if (Net.Nodes.Num() > 2)
				{
					const int32 Other = Net.Nodes[Rng.RandRange(0, Net.Nodes.Num() - 2)].Id;
					Net.AddConnection(Other, NewN.Id, EConnType::Excitatory, Rng.FRandRange(0.3f, 0.9f));
				}
				Lines.Add(TEXT("A new cell body formed."));
				Changed++;
			}
			break;

		case EMutOp::RemoveNode:
			if (Net.Nodes.Num() > 10)
			{
				if (FNeuralNode* N = RandomUnlockedHidden(Net, Rng))
				{
					Net.RemoveNode(N->Id);
					Lines.Add(TEXT("A cell died."));
					Changed++;
				}
			}
			break;

		case EMutOp::DuplicateMotif:
			if (DuplicateMotif(Net, Rng))
			{
				Lines.Add(TEXT("A motif copied itself."));
				Changed++;
			}
			break;

		case EMutOp::InvertConnection:
			if (FNeuralConnection* C = RandomUnlockedConn(Net, Rng))
			{
				C->Type = (C->Type == EConnType::Inhibitory) ? EConnType::Excitatory : EConnType::Inhibitory;
				Lines.Add(TEXT("A pathway inverted its influence."));
				Changed++;
			}
			break;

		case EMutOp::ChangeDelay:
			if (FNeuralConnection* C = RandomUnlockedConn(Net, Rng))
			{
				const int32 NewTicks = Rng.RandRange(0, 8);
				C->Delay = NewTicks * FNeuralNetwork::SimDT;
				C->DelayBuffer.SetNumZeroed(NewTicks + 1);
				C->BufHead = 0;
				Lines.Add(TEXT("Signal timing changed somewhere."));
				Changed++;
			}
			break;

		case EMutOp::ChangeNoise:
			if (FNeuralNode* N = RandomUnlockedHidden(Net, Rng))
			{
				N->Noise = FMath::Clamp(N->Noise + Rng.FRandRange(-0.05f, 0.05f), 0.f, 0.25f);
				Lines.Add(TEXT("Background noise fluctuated."));
				Changed++;
			}
			break;
		}
	}

	Net.bCompiledDirty = true;
	if (OutChangedCount) { *OutChangedCount = Changed; }
	return Lines;
}

bool MutationEngine::DuplicateMotif(FNeuralNetwork& Net, FRandomStream& Rng)
{
	// find the most reinforced connection; copy it plus its endpoints' local params
	const FNeuralConnection* Best = nullptr;
	for (const FNeuralConnection& C : Net.Connections)
	{
		if (!Best || C.ReinforceCount > Best->ReinforceCount) { Best = &C; }
	}
	if (!Best || Net.Nodes.Num() >= 24) { return false; }

	const FNeuralNode* SrcN = Net.FindNode(Best->Source);
	const FNeuralNode* DstN = Net.FindNode(Best->Target);
	if (!SrcN || !DstN) { return false; }

	const FVector2D Offset(Rng.FRandRange(-120.f, 120.f), Rng.FRandRange(60.f, 140.f));
	const ENodeType SrcT = SrcN->Type == ENodeType::Input ? ENodeType::Relay : SrcN->Type;
	const ENodeType DstT = DstN->Type == ENodeType::Output ? ENodeType::Relay : DstN->Type;
	const float Weight = Best->Weight;
	const EConnType ConnT = Best->Type;
	const FVector2D APos = SrcN->Pos + Offset;
	const FVector2D BPos = DstN->Pos + Offset;

	FNeuralNode& A = Net.AddNode(SrcT, APos, Rng);
	const int32 AId = A.Id;
	FNeuralNode& B = Net.AddNode(DstT, BPos, Rng);
	const int32 BId = B.Id;
	Net.AddConnection(AId, BId, ConnT, Weight);

	// splice the copy into the network
	if (Net.Nodes.Num() > 4)
	{
		const int32 Anchor = Net.Nodes[Rng.RandRange(0, Net.Nodes.Num() - 3)].Id;
		Net.AddConnection(Anchor, AId, EConnType::Excitatory, Rng.FRandRange(0.3f, 0.8f));
	}
	return true;
}
