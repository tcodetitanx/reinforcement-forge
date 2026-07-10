#include "PatternLibrary.h"
#include "Math/RandomStream.h"

bool FPatternLibrary::SaveFromSelection(const FNeuralNetwork& Net, const TArray<int32>& NodeIds, FString& OutError)
{
	if (NodeIds.Num() < 2)
	{
		OutError = TEXT("Select at least 2 nodes to capture a pattern.");
		return false;
	}
	if (NodeIds.Num() > 6)
	{
		OutError = TEXT("Patterns hold at most 6 nodes.");
		return false;
	}
	if (!HasSpace())
	{
		OutError = TEXT("Pattern memory is full.");
		return false;
	}

	FSavedPattern P;
	P.Id = NextId++;
	P.Name = FString::Printf(TEXT("Unknown Pattern %02d"), P.Id);

	FVector2D Centroid = FVector2D::ZeroVector;
	int32 Found = 0;
	for (int32 Id : NodeIds)
	{
		if (const FNeuralNode* N = Net.FindNode(Id))
		{
			Centroid += N->Pos;
			Found++;
		}
	}
	if (Found < 2)
	{
		OutError = TEXT("Selection no longer exists.");
		return false;
	}
	Centroid /= Found;

	TMap<int32, int32> Remap; // old id -> pattern-local id
	int32 LocalId = 0;
	for (int32 Id : NodeIds)
	{
		if (const FNeuralNode* N = Net.FindNode(Id))
		{
			FNeuralNode Copy = *N;
			Copy.Pos -= Centroid;
			Copy.Activation = 0.f;
			Copy.PrevActivation = 0.f;
			Copy.MemoryCharge = 0.f;
			Remap.Add(Id, LocalId);
			Copy.Id = LocalId++;
			P.Nodes.Add(Copy);
		}
	}
	for (const FNeuralConnection& C : Net.Connections)
	{
		const int32* Src = Remap.Find(C.Source);
		const int32* Dst = Remap.Find(C.Target);
		if (Src && Dst)
		{
			FNeuralConnection Copy = C;
			Copy.Id = P.Connections.Num();
			Copy.Source = *Src;
			Copy.Target = *Dst;
			Copy.DelayBuffer.Reset();
			Copy.LastSignal = 0.f;
			Copy.Traffic = 0.f;
			P.Connections.Add(Copy);
		}
	}

	Patterns.Add(MoveTemp(P));
	return true;
}

TArray<int32> FPatternLibrary::Insert(FNeuralNetwork& Net, int32 PatternId, const FVector2D& Pos, FRandomStream& Rng)
{
	TArray<int32> NewIds;
	FSavedPattern* P = Find(PatternId);
	if (!P || Net.Nodes.Num() + P->Nodes.Num() > 30) { return NewIds; }

	TMap<int32, int32> Remap; // pattern-local -> new net id
	for (const FNeuralNode& PN : P->Nodes)
	{
		// inputs/outputs of a pattern become relays inside the target network
		ENodeType T = PN.Type;
		if (T == ENodeType::Input || T == ENodeType::Output) { T = ENodeType::Relay; }

		FNeuralNode& NewN = Net.AddNode(T, Pos + PN.Pos, Rng);
		NewN.Bias = PN.Bias;
		NewN.Threshold = PN.Threshold;
		NewN.Decay = PN.Decay;
		NewN.Noise = PN.Noise;
		NewN.Plasticity = PN.Plasticity;
		Remap.Add(PN.Id, NewN.Id);
		NewIds.Add(NewN.Id);
	}
	for (const FNeuralConnection& PC : P->Connections)
	{
		const int32* Src = Remap.Find(PC.Source);
		const int32* Dst = Remap.Find(PC.Target);
		if (Src && Dst)
		{
			if (FNeuralConnection* C = Net.AddConnection(*Src, *Dst, PC.Type, PC.Weight))
			{
				C->Plasticity = PC.Plasticity;
			}
		}
	}

	P->UseCount++;
	if (!P->bNamed && P->UseCount >= 3)
	{
		// after repeated successful use the game names the motif
		const TCHAR* Names[] =
		{
			TEXT("Feedback Stabilizer"), TEXT("Threshold Gate"), TEXT("Reward Relay"),
			TEXT("Sparse Detector"), TEXT("Signal Amplifier"), TEXT("Memory Loop"),
			TEXT("Priority Inhibitor"), TEXT("Noise Filter"), TEXT("Oscillator Core")
		};
		P->Name = Names[Rng.RandRange(0, UE_ARRAY_COUNT(Names) - 1)];
		P->bNamed = true;
	}
	return NewIds;
}

bool FPatternLibrary::Remove(int32 PatternId)
{
	return Patterns.RemoveAll([PatternId](const FSavedPattern& P) { return P.Id == PatternId; }) > 0;
}

FSavedPattern* FPatternLibrary::Find(int32 PatternId)
{
	for (FSavedPattern& P : Patterns) { if (P.Id == PatternId) return &P; }
	return nullptr;
}
