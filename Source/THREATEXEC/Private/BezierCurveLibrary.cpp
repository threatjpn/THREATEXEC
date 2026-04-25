// ============================================================================
// BezierCurveLibrary.cpp
// Implements shared Bézier utility functions exposed through the blueprint function library.
//
// Notes:
// - Comments in this file are documentation-only and do not alter behaviour.
// - Function signatures, ordering, and implementation logic are preserved.
// ============================================================================

#include "BezierCurveLibrary.h"

void UBezierCurveLibrary::BuildArcLengthLUT(const TArray<FVector>& Pts, TArray<double>& OutCumulative)
{
	OutCumulative.Reset();
	if (Pts.Num() == 0) return;

	OutCumulative.Reserve(Pts.Num());
	double L = 0.0;
	OutCumulative.Add(0.0);
	for (int32 i = 1; i < Pts.Num(); ++i)
	{
		L += FVector::Distance(Pts[i-1], Pts[i]);
		OutCumulative.Add(L);
	}
}

static int32 LowerBoundDouble(const TArray<double>& A, double x)
{
	int32 lo = 0, hi = A.Num();
	while (lo < hi)
	{
		int32 mid = (lo + hi) >> 1;
		if (A[mid] < x) lo = mid + 1; else hi = mid;
	}
	return lo;
}

void UBezierCurveLibrary::UniformResampleByArc(const TArray<FVector>& Pts, int32 Count, TArray<FVector>& OutPts)
{
	OutPts.Reset();
	const int32 N = Pts.Num();
	if (N == 0 || Count <= 0) return;
	if (N == 1 || Count == 1) { OutPts.Add(Pts[0]); return; }

	TArray<double> Cum; BuildArcLengthLUT(Pts, Cum);
	const double Total = Cum.Last();
	if (Total <= SMALL_NUMBER)
	{
		OutPts.Add(Pts[0]); return;
	}

	OutPts.Reserve(Count);
	for (int32 k = 0; k < Count; ++k)
	{
		const double target = (Count == 1) ? 0.0 : (Total * (double)k / (double)(Count - 1));
		int32 j = LowerBoundDouble(Cum, target);
		j = FMath::Clamp(j, 1, N - 1);
		const double segLen = Cum[j] - Cum[j-1];
		const double a = (segLen > SMALL_NUMBER) ? (target - Cum[j-1]) / segLen : 0.0;
		OutPts.Add(FMath::Lerp(Pts[j-1], Pts[j], (float)a));
	}
}

uint32 UBezierCurveLibrary::HashControl(const TArray<FVector2D>& C)
{
	uint32 H = 2166136261u; // FNV-1a
	for (const FVector2D& P : C)
	{
		auto mix = [&](double v){
			uint32 u; FMemory::Memcpy(&u, &v, sizeof(uint32)); // low 32 bits fine for hashing
			H ^= u; H *= 16777619u;
		};
		mix(P.X); mix(P.Y);
	}
	return H;
}

uint32 UBezierCurveLibrary::HashControl3D(const TArray<FVector>& C)
{
	uint32 H = 2166136261u;
	for (const FVector& P : C)
	{
		auto mix = [&](double v){
			uint32 u; FMemory::Memcpy(&u, &v, sizeof(uint32));
			H ^= u; H *= 16777619u;
		};
		mix(P.X); mix(P.Y); mix(P.Z);
	}
	return H;
}
