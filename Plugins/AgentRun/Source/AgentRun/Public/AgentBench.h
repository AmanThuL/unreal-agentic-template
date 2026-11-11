#pragma once

#include "CoreMinimal.h"

/** Frame statistics of one bench: what a player feels (the lows and hitches), not just the average. */
struct FAgentBenchResult
{
	int32 Frames = 0;
	double AverageFps = 0.0;
	/** The 1% low: the frame rate of the 99th percentile frame time. */
	double Low1Fps = 0.0;
	/** The frame rate of the slowest frame. */
	double MinFps = 0.0;
	double AverageMs = 0.0;
	double WorstMs = 0.0;
	/** Frames slower than the hitch threshold. */
	int32 Hitches = 0;
};

/** Bench math, kept free of the engine loop so it is testable without a world. */
namespace AgentBench
{
	/** Summarizes frame times in seconds. HitchSeconds defaults to 25 ms (under 40 fps). */
	AGENTRUN_API FAgentBenchResult Summarize(TConstArrayView<float> FrameSeconds, float HitchSeconds = 0.025f);

	/** CSV header and row for Saved/AgentRuns/bench.csv. Thread times are averages in milliseconds. */
	AGENTRUN_API FString CsvHeader();
	AGENTRUN_API FString CsvRow(const FString& Label, const FAgentBenchResult& Result, double GameMs, double RenderMs, double GpuMs);
}
