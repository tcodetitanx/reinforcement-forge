#pragma once

#include "CoreMinimal.h"
#include "ForgeTypes.h"

struct FRandomStream;

/** Maximum aggregate input/output channels a region exposes to its parent graph. */
static constexpr int32 MaxChannels = 4;

/** One neuron inside a subnetwork (guide section 5.3). */
struct BRAINFORGE_API FNeuralNode
{
	int32 Id = 0;
	ENodeType Type = ENodeType::Relay;
	FVector2D Pos = FVector2D::ZeroVector;

	float Bias = 0.f;
	float Threshold = 0.4f;
	float Decay = 0.1f;
	float Noise = 0.05f;
	float Plasticity = 0.5f;
	float EnergyCost = 0.1f;
	bool bLocked = false;

	// runtime
	float Activation = 0.f;
	float PrevActivation = 0.f;
	float OscPhase = 0.f;
	float MemoryCharge = 0.f;
	float PulseTime = 100.f;      // seconds since last strong activation (viz)
	float AvgActivity = 0.f;      // EMA of activation, for discovery/pruning stats

	// discovery metadata
	FString DiscoveredFunction;
	float Confidence = 0.f;

	// channel index for Input/Output nodes
	int32 Channel = INDEX_NONE;
};

/** One synapse (guide section 5.4). */
struct BRAINFORGE_API FNeuralConnection
{
	int32 Id = 0;
	int32 Source = 0;
	int32 Target = 0;
	EConnType Type = EConnType::Excitatory;

	float Weight = 0.5f;          // clamped -2..2 (sign carried by type for inhibitory)
	float Delay = 0.f;            // sim seconds
	float Noise = 0.02f;
	float Plasticity = 0.5f;
	int32 ReinforceCount = 0;
	bool bLocked = false;

	// runtime
	TArray<float> DelayBuffer;
	int32 BufHead = 0;
	float LastSignal = 0.f;       // signal delivered last tick (viz pulses)
	float Traffic = 0.f;          // EMA of |signal|
	float PulsePhase = 0.f;       // viz: traveling pulse position seed
};

/** Aggregate result of compiling a closed subnetwork (guide section 8). */
struct BRAINFORGE_API FCompiledSample
{
	float In[MaxChannels] = {0, 0, 0, 0};
	float Out[MaxChannels] = {0, 0, 0, 0};
};

/**
 * A self-contained symbolic neural network. Used both for region subnetworks.
 * Tick model follows guide section 6.
 */
struct BRAINFORGE_API FNeuralNetwork
{
	int32 NextId = 1;
	TArray<FNeuralNode> Nodes;
	TArray<FNeuralConnection> Connections;

	float InputValues[MaxChannels] = {0, 0, 0, 0};
	float OutputValues[MaxChannels] = {0, 0, 0, 0};

	// derived stats (EMA)
	float Stability = 0.7f;
	float EnergyUse = 0.f;
	float ActivityLevel = 0.f;

	// compiled approximation used while this network's region is closed
	TArray<FCompiledSample> Compiled;
	bool bCompiledDirty = true;

	static constexpr float SimDT = 0.1f;

	FNeuralNode* FindNode(int32 Id);
	const FNeuralNode* FindNode(int32 Id) const;
	FNeuralConnection* FindConnection(int32 Id);
	int32 NodeIndex(int32 Id) const;

	FNeuralNode& AddNode(ENodeType Type, const FVector2D& Pos, FRandomStream& Rng);
	FNeuralConnection* AddConnection(int32 Source, int32 Target, EConnType Type, float Weight);
	bool RemoveNode(int32 Id);
	bool RemoveConnection(int32 Id);
	bool HasConnection(int32 Source, int32 Target) const;

	/** Advance one sim tick. */
	void Tick(FRandomStream& Rng);

	/** Evaluate the compiled approximation for the current InputValues. */
	void EvaluateCompiled(FRandomStream& Rng);

	/** Re-sample the network response table (run offline against test inputs). */
	void Compile(FRandomStream& Rng);

	/** Reset runtime activations. */
	void ResetState();

	/** Hebbian-style reinforcement of recently-active paths (guide section 16). */
	void Reinforce(float Reward, float LearningRate, bool bOnlyPositiveTraffic);

	/** Remove the weakest (lowest traffic * |weight|) unlocked connections. Returns removed count. */
	int32 PruneWeakest(int32 MaxCount, float TrafficThreshold);

	/** Randomize all unlocked weights/params slightly. */
	void Randomize(FRandomStream& Rng, float Intensity);

	/** Ensure Input/Output nodes have sane channel assignments. */
	void AssignChannels();

	/** Total absolute activation - cheap liveness metric. */
	float TotalActivation() const;
};

/** Build a random starter subnetwork appropriate for a region. */
BRAINFORGE_API void BuildStarterNetwork(FNeuralNetwork& Net, FRandomStream& Rng, int32 Complexity);
