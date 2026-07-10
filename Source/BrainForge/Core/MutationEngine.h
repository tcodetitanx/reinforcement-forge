#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"

/** Preview shown before committing a mutation (guide section 15). Estimates are unreliable early. */
struct BRAINFORGE_API FMutationPreview
{
	int32 NodesAffected = 0;
	int32 ConnectionsChanged = 0;
	FString EnergyEstimate;
	FString StabilityEstimate;
};

namespace MutationEngine
{
	/** Generate a (possibly inaccurate) preview. Knowledge 0..1 sharpens the estimates. */
	BRAINFORGE_API FMutationPreview Preview(const FNeuralNetwork& Net, float Intensity, float Knowledge, FRandomStream& Rng);

	/**
	 * Apply weighted random mutations (guide section 32).
	 * Returns human-readable lines describing what happened (vague on purpose).
	 */
	BRAINFORGE_API TArray<FString> Mutate(FNeuralNetwork& Net, float Intensity, FRandomStream& Rng, int32* OutChangedCount = nullptr);

	/** Duplicate the most reinforced motif (2-3 nodes) elsewhere in the network. */
	BRAINFORGE_API bool DuplicateMotif(FNeuralNetwork& Net, FRandomStream& Rng);
}
