#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"

/** A saved wiring motif (guide section 17). */
struct BRAINFORGE_API FSavedPattern
{
	int32 Id = 0;
	FString Name;                 // starts as "Unknown Pattern 07"
	bool bNamed = false;          // discovered/renamed
	TArray<FNeuralNode> Nodes;    // positions relative to centroid
	TArray<FNeuralConnection> Connections; // ids remapped 0..n
	float KnownStabilityEffect = 0.f;
	int32 UseCount = 0;
};

struct BRAINFORGE_API FPatternLibrary
{
	int32 SlotCap = 4;
	int32 NextId = 1;
	TArray<FSavedPattern> Patterns;

	bool HasSpace() const { return Patterns.Num() < SlotCap; }

	/** Capture the given nodes (and their internal connections) as a new pattern. */
	bool SaveFromSelection(const FNeuralNetwork& Net, const TArray<int32>& NodeIds, FString& OutError);

	/** Stamp a pattern into a network at the given position. Returns new node ids. */
	TArray<int32> Insert(FNeuralNetwork& Net, int32 PatternId, const FVector2D& Pos, FRandomStream& Rng);

	bool Remove(int32 PatternId);
	FSavedPattern* Find(int32 PatternId);
};
