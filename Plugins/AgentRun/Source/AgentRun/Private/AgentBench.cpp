#include "AgentBench.h"

FAgentBenchResult AgentBench::Summarize(TConstArrayView<float> FrameSeconds, float HitchSeconds)
{
	FAgentBenchResult Result;
	Result.Frames = FrameSeconds.Num();
	if (Result.Frames == 0)
	{
		return Result;
	}
	TArray<float> Sorted(FrameSeconds.GetData(), FrameSeconds.Num());
	Sorted.Sort();
	double Total = 0.0;
	for (const float Seconds : Sorted)
	{
		Total += Seconds;
		Result.Hitches += Seconds > HitchSeconds ? 1 : 0;
	}
	const double Average = Total / Result.Frames;
	const double Worst = Sorted.Last();
	const double Slow = Sorted[FMath::Clamp(FMath::CeilToInt32(Result.Frames * 0.99) - 1, 0, Result.Frames - 1)];
	Result.AverageMs = Average * 1000.0;
	Result.WorstMs = Worst * 1000.0;
	Result.AverageFps = Average > 0.0 ? 1.0 / Average : 0.0;
	Result.Low1Fps = Slow > 0.0 ? 1.0 / Slow : 0.0;
	Result.MinFps = Worst > 0.0 ? 1.0 / Worst : 0.0;
	return Result;
}

FString AgentBench::CsvHeader()
{
	return TEXT("label,frames,avg_fps,low1_fps,min_fps,avg_ms,worst_ms,hitches,game_ms,render_ms,gpu_ms");
}

FString AgentBench::CsvRow(const FString& Label, const FAgentBenchResult& Result, double GameMs, double RenderMs, double GpuMs)
{
	// Labels come from script tokens, so they hold no commas or quotes.
	return FString::Printf(TEXT("%s,%d,%.1f,%.1f,%.1f,%.2f,%.2f,%d,%.2f,%.2f,%.2f"), *Label, Result.Frames, Result.AverageFps, Result.Low1Fps,
		Result.MinFps, Result.AverageMs, Result.WorstMs, Result.Hitches, GameMs, RenderMs, GpuMs);
}
