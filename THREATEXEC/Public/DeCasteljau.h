#pragma once

// ============================================================================
// DeCasteljau.h
//
// Header-only, allocation-free Bezier evaluation utilities for:
//   - double       (1D scalar)
//   - FVector2D    (2D)
//   - FVector      (3D)
//
// Features:
//   Safe Eval(t) with degenerate guards (0,1 control points)
//   Fully deterministic for identical inputs
//   No dynamic allocations inside hot loops
//   NaN protection where needed
//   De Casteljau levels builder for proof visualisation
//   Uniform arc-length sampling (2D/3D)
//   Suitable for real-time use in Unreal Engine editor
//
// Everything sits inside namespace TEBezier.
//
// ============================================================================

#include "CoreMinimal.h"

namespace TEBezier
{
	// ------------------------------------------------------------------------
	// Internal utility: component-wise LERP for double / FVector2D / FVector
	// ------------------------------------------------------------------------
	template<typename T>
	FORCEINLINE T Lerp(const T& A, const T& B, double Alpha)
	{
		// Generic fallback: component-wise scalar multiply is expected.
		return (T)((1.0 - Alpha) * A + Alpha * B);
	}

	template<>
	FORCEINLINE double Lerp<double>(const double& A, const double& B, double Alpha)
	{
		return A + (B - A) * Alpha;
	}

	template<>
	FORCEINLINE FVector2D Lerp<FVector2D>(const FVector2D& A, const FVector2D& B, double Alpha)
	{
		return FVector2D(
			A.X + (B.X - A.X) * Alpha,
			A.Y + (B.Y - A.Y) * Alpha
		);
	}

	template<>
	FORCEINLINE FVector Lerp<FVector>(const FVector& A, const FVector& B, double Alpha)
	{
		return FVector(
			A.X + (B.X - A.X) * Alpha,
			A.Y + (B.Y - A.Y) * Alpha,
			A.Z + (B.Z - A.Z) * Alpha
		);
	}

	// ------------------------------------------------------------------------
	// Type-specific helpers: distance and NaN checks for supported point types
	// ------------------------------------------------------------------------
	static FORCEINLINE double PointDistance(const double& A, const double& B)
	{
		return FMath::Abs(A - B);
	}

	static FORCEINLINE double PointDistance(const FVector2D& A, const FVector2D& B)
	{
		// FVector2D::Distance is available; explicit call to avoid sqrt duplication
		return FVector2D::Distance(A, B);
	}

	static FORCEINLINE double PointDistance(const FVector& A, const FVector& B)
	{
		return FVector::Distance(A, B);
	}

	static FORCEINLINE bool HasNaN(const double& A)
	{
		return FMath::IsNaN(A);
	}

	static FORCEINLINE bool HasNaN(const FVector2D& A)
	{
		return A.ContainsNaN();
	}

	static FORCEINLINE bool HasNaN(const FVector& A)
	{
		return A.ContainsNaN();
	}

	// ------------------------------------------------------------------------
	// SafeEval(T): handles degenerate cases without crashing.
	// ------------------------------------------------------------------------
	template<typename T>
	FORCEINLINE T SafeEval(const TArray<T>& Control, double TVal)
	{
		const int32 N = Control.Num();
		if (N <= 0)
		{
			return T(0); // degenerate: return zero
		}
		else if (N == 1)
		{
			return Control[0]; // degenerate: single point
		}

		const double t = FMath::Clamp(TVal, 0.0, 1.0);

		// Work buffer copied once outside loops
		T Work[64]; // supports curves up to degree 63 (overkill for editor)
		check(N <= 64);

		for (int32 i = 0; i < N; ++i)
		{
			Work[i] = Control[i];
		}

		for (int32 k = N - 1; k > 0; --k)
		{
			for (int32 i = 0; i < k; ++i)
			{
				Work[i] = Lerp<T>(Work[i], Work[i + 1], t);
			}
		}

		return Work[0];
	}

	// ------------------------------------------------------------------------
	// DeCasteljauEval: explicit name for clarity.
	// ------------------------------------------------------------------------
	template<typename T>
	FORCEINLINE T DeCasteljauEval(const TArray<T>& Control, double TVal)
	{
		return SafeEval<T>(Control, TVal);
	}

	// ------------------------------------------------------------------------
	// DeCasteljauLevels: returns array of levels used for proof.
	// Example for N control points:
	//   level 0 -> N items
	//   level 1 -> N-1 items
	//   ...
	//   level N-1 -> 1 item
	// ------------------------------------------------------------------------
	template<typename T>
	inline void DeCasteljauLevels(const TArray<T>& Control, double TVal, TArray<TArray<T>>& OutLevels)
	{
		OutLevels.Reset();

		const int32 N = Control.Num();
		if (N <= 0)
		{
			return;
		}

		const double t = FMath::Clamp(TVal, 0.0, 1.0);

		OutLevels.SetNum(N);
		OutLevels[0] = Control;

		for (int32 level = 1; level < N; ++level)
		{
			const int32 CountPrev = OutLevels[level - 1].Num();
			const int32 CountThis = CountPrev - 1;

			OutLevels[level].SetNum(CountThis);

			for (int32 i = 0; i < CountThis; ++i)
			{
				const T& A = OutLevels[level - 1][i];
				const T& B = OutLevels[level - 1][i + 1];
				const T P = Lerp<T>(A, B, t);
				OutLevels[level][i] = P;
			}
		}

		// (Optional) NaN scrub:
		for (auto& Level : OutLevels)
		{
			for (T& P : Level)
			{
				if (HasNaN(P))
				{
					P = T(0); // fallback
				}
			}
		}
	}

	// ------------------------------------------------------------------------
	// Parameter-uniform sampling (simple sample by t)
	// ------------------------------------------------------------------------
	template<typename T>
	inline void SampleUniform(const TArray<T>& Control, int32 NumSamples, TArray<T>& Out)
	{
		Out.Reset();

		const int32 N = FMath::Clamp(NumSamples, 1, 8192);
		Out.Reserve(N);

		if (Control.Num() == 0)
		{
			return;
		}

		if (Control.Num() == 1)
		{
			Out.Init(Control[0], N);
			return;
		}

		for (int32 i = 0; i < N; ++i)
		{
			const double t = (N == 1) ? 0.0 : (double)i / (double)(N - 1);
			Out.Add(DeCasteljauEval<T>(Control, t));
		}
	}

	// ------------------------------------------------------------------------
	// Arc-length sampling helpers
	// Private helper: Computes dense points & cumulative distances.
	// Used for 2D and 3D curves.
	// ------------------------------------------------------------------------
	template<typename T>
	inline bool BuildDenseArcSamples(const TArray<T>& Control, int32 DenseCount,
		TArray<T>& DensePts, TArray<double>& CumDist)
	{
		const int32 Dense = FMath::Max(DenseCount, 2);

		const int32 C = Control.Num();
		if (C < 2)
		{
			return false;
		}

		DensePts.Reset(Dense);
		CumDist.SetNum(Dense);

		for (int32 i = 0; i < Dense; ++i)
		{
			const double t = (Dense == 1) ? 0.0 : (double)i / (double)(Dense - 1);
			T P = DeCasteljauEval<T>(Control, t);

			// NaN protection using type-safe helper
			if (HasNaN(P))
			{
				P = T(0);
			}

			DensePts.Add(P);

			if (i == 0)
			{
				CumDist[0] = 0.0;
			}
			else
			{
				// Use type-specific distance helper (no casts)
				const double d = PointDistance(
					DensePts[i - 1],
					DensePts[i]
				);
				CumDist[i] = CumDist[i - 1] + d;
			}
		}

		return true;
	}

	// ------------------------------------------------------------------------
	// UniformArcLengthSample (dimension agnostic)
	//
	// Strategy:
	//   1) Dense pre-sample -> DensePts[] and CumDist[]
	//   2) Interpolate distance targets
	//   3) Binary search in CumDist[]
	//   4) Lerp t between bracketed indices -> DeCasteljauEval
	// ------------------------------------------------------------------------

	template<typename T>
	inline void UniformArcLengthSample(
		const TArray<T>& Control,
		int32 TargetCount,
		TArray<T>& Out)
	{
		Out.Reset();

		if (Control.Num() < 2)
		{
			return;
		}

		const int32 Count = FMath::Clamp(TargetCount, 2, 4096);
		const int32 Dense = FMath::Max(Count * 8, 64);

		TArray<T> DensePts;
		TArray<double> Cum;

		if (!BuildDenseArcSamples<T>(Control, Dense, DensePts, Cum))
		{
			return;
		}

		const double TotalLength = Cum.Last();
		if (TotalLength <= KINDA_SMALL_NUMBER)
		{
			// Degenerate: all points same � collapse to ends
			Out.Add(DensePts[0]);
			Out.Add(DensePts.Last());
			return;
		}

		Out.Reserve(Count);

		for (int32 j = 0; j < Count; ++j)
		{
			const double d = TotalLength * (double)j / (double)(Count - 1);

			// Binary search upper bound
			int32 lo = 0;
			int32 hi = Dense - 1;
			while (lo < hi)
			{
				const int32 mid = (lo + hi) >> 1;
				if (Cum[mid] < d) lo = mid + 1;
				else hi = mid;
			}
			const int32 idx = FMath::Clamp(lo, 1, Dense - 1);

			const int32 i0 = idx - 1;
			const int32 i1 = idx;

			const double seg = FMath::Max(Cum[i1] - Cum[i0], SMALL_NUMBER);
			const double a = (d - Cum[i0]) / seg;

			// Convert index-space blend to t-space
			const double t0 = (double)i0 / (double)(Dense - 1);
			const double t1 = (double)i1 / (double)(Dense - 1);
			const double t = FMath::Lerp(t0, t1, a);

			T P = DeCasteljauEval<T>(Control, t);

			if (HasNaN(P))
			{
				P = T(0);
			}

			Out.Add(P);
		}
	}
}