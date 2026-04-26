// ============================================================================
// BezierCurve3DActor.cpp
// Implements the 3D Bézier actor, including control-point management, spline synchronisation, visualisation, and export workflows.
//
// Notes:
// - Comments in this file are documentation-only and do not alter behaviour.
// - Function signatures, ordering, and implementation logic are preserved.
// ============================================================================

#include "BezierCurve3DActor.h"

#include "BezierEditSubsystem.h"

#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "DeCasteljau.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/PlatformFilemanager.h"
#include "THREATEXEC_FileUtils.h"
#include "Algo/Reverse.h"

namespace
{
	static void TE_DrawRuntimeLine3D(const UObject* Owner, UWorld* World, const FVector& Start, const FVector& End, const FLinearColor& Color, uint8 DepthPriority, float Thickness)
	{
		if (!Owner || !World)
		{
			return;
		}

		FLinearColor RuntimeColor = Color;
		const float RuntimeAlpha = FMath::Clamp(RuntimeColor.A, 0.0f, 1.0f);
		RuntimeColor.A = RuntimeAlpha;
		DrawDebugLine(
			World,
			Start,
			End,
			RuntimeColor.ToFColor(true),
			false,
			0.0f,
			DepthPriority,
			Thickness * RuntimeAlpha);
	}


	static uint8 TE_ResolveDepthPriority(const EBezierVisualPriority InPriority)
	{
		switch (InPriority)
		{
		case EBezierVisualPriority::World: return SDPG_World;
		case EBezierVisualPriority::Foreground:
		case EBezierVisualPriority::Overlay:
		default: return SDPG_Foreground;
		}
	}

	static int32 TE_ResolveSortPriority(const EBezierVisualPriority InPriority, const int32 InBias)
	{
		const int32 BasePriority = (InPriority == EBezierVisualPriority::Overlay) ? 2000 : (InPriority == EBezierVisualPriority::Foreground ? 1000 : 0);
		return BasePriority + InBias;
	}

	static void TE_DrawRuntimePoint3D(const UObject* Owner, UWorld* World, const FVector& Position, float PointSize, const FLinearColor& Color, uint8 DepthPriority)
	{
		if (!Owner || !World)
		{
			return;
		}

		FLinearColor RuntimeColor = Color;
		const float RuntimeAlpha = FMath::Clamp(RuntimeColor.A, 0.0f, 1.0f);
		RuntimeColor.A = RuntimeAlpha;
		DrawDebugPoint(
			World,
			Position,
			PointSize * RuntimeAlpha,
			RuntimeColor.ToFColor(true),
			false,
			0.0f,
			DepthPriority);
	}
}

static void SetInstanceColorRGB3D(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const FLinearColor& C, float Alpha)
{
	if (!ISM) return;
	if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return;
	if (ISM->NumCustomDataFloats < 4) ISM->NumCustomDataFloats = 4;

	ISM->SetCustomDataValue(InstanceIndex, 0, C.R, false);
	ISM->SetCustomDataValue(InstanceIndex, 1, C.G, false);
	ISM->SetCustomDataValue(InstanceIndex, 2, C.B, false);
	ISM->SetCustomDataValue(InstanceIndex, 3, Alpha, true);
}

ABezierCurve3DActor::ABezierCurve3DActor()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(Root);

	ControlPointISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ControlPoints"));
	ControlPointISM->SetupAttachment(Root);
	ControlPointISM->NumCustomDataFloats = 3;
	ControlPointISM->SetCullDistances(0, 0);
	ControlPointISM->bNeverDistanceCull = true;
	ControlPointISM->LDMaxDrawDistance = 0.0f;
	ControlPointISM->DetailMode = DM_Low;
	ControlPointISM->SetCollisionResponseToAllChannels(ECR_Ignore);
	ControlPointISM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ControlPointISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		ControlPointISM->SetStaticMesh(CubeMesh.Object);
	}

	StripMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StripMesh"));
	StripMeshComponent->SetupAttachment(Root);
	StripMeshComponent->bNeverDistanceCull = true;
	StripMeshComponent->LDMaxDrawDistance = 0.0f;
	StripMeshComponent->DetailMode = DM_Low;
	StripMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CubeStripISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CubeStrip"));
	CubeStripISM->SetupAttachment(Root);
	CubeStripISM->SetCullDistances(0, 0);
	CubeStripISM->bNeverDistanceCull = true;
	CubeStripISM->LDMaxDrawDistance = 0.0f;
	CubeStripISM->DetailMode = DM_Low;
	CubeStripISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (CubeMesh.Succeeded())
	{
		CubeStripISM->SetStaticMesh(CubeMesh.Object);
	}

	if (GridSizeCycleValues.Num() == 0)
	{
		GridSizeCycleValues = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	}
}

void ABezierCurve3DActor::BeginPlay()
{
	Super::BeginPlay();

	if (UBezierEditSubsystem* Sub = GetWorld()->GetSubsystem<UBezierEditSubsystem>())
	{
		Sub->RegisterEditable(this);
	}

	EnsureSpline();
	ReadSplineToControl();
	InitialControl = Control;

	RefreshControlPointVisuals();
	ApplyRuntimeEditVisibility();
}

void ABezierCurve3DActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		if (UBezierEditSubsystem* Sub = W->GetSubsystem<UBezierEditSubsystem>())
		{
			Sub->UnregisterEditable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ABezierCurve3DActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureSpline();
	if (Control.Num() == 0)
	{
		Control = { FVector(0,0,0), FVector(1,0,0) };
		WriteControlToSpline();
	}

	RefreshControlPointVisuals();
	ApplyRuntimeEditVisibility();
}

#if WITH_EDITOR
bool ABezierCurve3DActor::ShouldTickIfViewportsOnly() const { return true; }
#endif

void ABezierCurve3DActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ReadSplineToControl();

	UpdateVisualFadeTargets();
	UpdateVisualFade(DeltaSeconds);

	if (bShowStripMesh)
	{
		UpdateStripMesh();
	}

	UpdateStripPulseOpacity();

	ApplyRuntimeEditVisibility();
	UpdateControlPointPulse();

	if (!GetWorld()) return;

	const FTransform Xf = GetActorTransform();
	const float DebugPulseT = (FMath::Sin(GetWorld()->GetTimeSeconds() * DebugPulseSpeed) + 1.0f) * 0.5f;
	const float GridPulseT = (FMath::Sin(GetWorld()->GetTimeSeconds() * GridPulseSpeed) + 1.0f) * 0.5f;
	const float DebugAlphaMin = FMath::Min(DebugPulseMinAlpha, DebugPulseMaxAlpha);
	const float DebugAlphaMax = FMath::Max(DebugPulseMinAlpha, DebugPulseMaxAlpha);
	const float PulseAlpha = bPulseDebugLines
		? FMath::Lerp(DebugAlphaMin, DebugAlphaMax, DebugPulseT)
		: 1.0f;
	const float FinalDebugAlpha = FMath::Clamp(PulseAlpha, 0.0f, 1.0f);
	const float DebugThicknessMin = FMath::Min(DebugPulseMinThickness, DebugPulseMaxThickness);
	const float DebugThicknessMax = FMath::Max(DebugPulseMinThickness, DebugPulseMaxThickness);
	const float DebugThickness = bPulseDebugLines
		? FMath::Lerp(DebugThicknessMin, DebugThicknessMax, DebugPulseT)
		: DebugPulseMaxThickness;
	const float GridAlphaMin = FMath::Min(GridPulseMinAlpha, GridPulseMaxAlpha);
	const float GridAlphaMax = FMath::Max(GridPulseMinAlpha, GridPulseMaxAlpha);
	const float GridPulseAlpha = bPulseGrid
		? FMath::Lerp(GridAlphaMin, GridAlphaMax, GridPulseT)
		: 1.0f;
	const float GridThicknessMin = FMath::Min(GridPulseMinThickness, GridPulseMaxThickness);
	const float GridThicknessMax = FMath::Max(GridPulseMinThickness, GridPulseMaxThickness);
	const float GridThickness = bPulseGrid
		? FMath::Lerp(GridThicknessMin, GridThicknessMax, GridPulseT)
		: GridPulseMaxThickness;
	const float FinalDebugThickness = FMath::Max(0.01f, DebugThickness * DebugThicknessScale);
	const float FinalGridThickness = FMath::Max(0.01f, GridThickness * GridThicknessScale);
	const uint8 DebugDepthPriority = TE_ResolveDepthPriority(VisualPriority);

	if (bEditMode && bShowControlPolygon && Control.Num() >= 2)
	{
		for (int32 i = 0; i + 1 < Control.Num(); ++i)
		{
			TE_DrawRuntimeLine3D(
				this,
				GetWorld(),
				Xf.TransformPosition(Control[i] * Scale),
				Xf.TransformPosition(Control[i + 1] * Scale),
				DebugLineColor.CopyWithNewOpacity(FMath::Clamp(DebugLineColor.A * FinalDebugAlpha, 0.0f, 1.0f)),
				DebugDepthPriority,
				FinalDebugThickness);
		}
	}

	if (bEditMode && bShowLevelsAtT && Control.Num() >= 2)
	{
		TArray<TArray<FVector>> Levels;
		TEBezier::DeCasteljauLevels<FVector>(Control, FMath::Clamp(ProofT, 0.0, 1.0), Levels);
		for (int32 L = 0; L < Levels.Num(); ++L)
		{
			const FLinearColor LevelColor = (L == Levels.Num() - 1) ? DebugResultColor : DebugLevelColor;
			for (int32 i = 0; i + 1 < Levels[L].Num(); ++i)
			{
				const float LevelAlpha = FMath::Clamp(LevelColor.A * FinalDebugAlpha, 0.0f, 1.0f);
				TE_DrawRuntimeLine3D(
					this,
					GetWorld(),
					Xf.TransformPosition(Levels[L][i] * Scale),
					Xf.TransformPosition(Levels[L][i + 1] * Scale),
					LevelColor.CopyWithNewOpacity(LevelAlpha),
					DebugDepthPriority,
					FinalDebugThickness);
			}
		}
	}

	if (bEditMode && bShowSamplePoints && Control.Num() >= 2)
	{
		TArray<FVector> Samples;
		const int32 SampleCount = FMath::Clamp(StripSegments, 2, 512);
		SampleCurvePoints(SampleCount, Samples);
		for (const FVector& Sample : Samples)
		{
			TE_DrawRuntimePoint3D(
				this,
				GetWorld(),
				Xf.TransformPosition(Sample * Scale),
				6.0f,
				DebugSamplePointColor.CopyWithNewOpacity(FMath::Clamp(DebugSamplePointColor.A * FinalDebugAlpha, 0.0f, 1.0f)),
				DebugDepthPriority);
		}
	}

	if (bEditMode && (bShowGrid || bSnapToGrid))
	{
		const float G = FMath::Max(0.01f, GridSizeCm);
		const int32 HalfCells = FMath::Max(1, FMath::RoundToInt(GridExtentCm / G));
		const float Extent = G * HalfCells;
		const float FinalAlpha = FMath::Clamp(GridColor.A * GridBaseAlpha * GridPulseAlpha, 0.0f, 1.0f);
		const FLinearColor GridLineColor(
			FMath::Clamp(GridColor.R, 0.0f, 1.0f),
			FMath::Clamp(GridColor.G, 0.0f, 1.0f),
			FMath::Clamp(GridColor.B, 0.0f, 1.0f),
			FinalAlpha
		);
		const FVector WorldGridOrigin = GridOriginWorld;
		const auto ToWorld = [&Xf, &WorldGridOrigin](const FVector& LocalPoint)
		{
			return Xf.TransformVector(LocalPoint) + WorldGridOrigin;
		};
		for (int32 i = -HalfCells; i <= HalfCells; ++i)
		{
			const float Offset = i * G;

			if (bShowGridXY)
			{
				const FVector A(-Extent, Offset, 0.0f);
				const FVector B(Extent, Offset, 0.0f);
				TE_DrawRuntimeLine3D(this, GetWorld(), ToWorld(A), ToWorld(B), GridLineColor, DebugDepthPriority, FinalGridThickness);

				const FVector C(Offset, -Extent, 0.0f);
				const FVector D(Offset, Extent, 0.0f);
				TE_DrawRuntimeLine3D(this, GetWorld(), ToWorld(C), ToWorld(D), GridLineColor, DebugDepthPriority, FinalGridThickness);
			}

			if (bShowGridXZ)
			{
				const FVector E(-Extent, 0.0f, Offset);
				const FVector F(Extent, 0.0f, Offset);
				TE_DrawRuntimeLine3D(this, GetWorld(), ToWorld(E), ToWorld(F), GridLineColor, DebugDepthPriority, FinalGridThickness);

				const FVector G0(Offset, 0.0f, -Extent);
				const FVector H(Offset, 0.0f, Extent);
				TE_DrawRuntimeLine3D(this, GetWorld(), ToWorld(G0), ToWorld(H), GridLineColor, DebugDepthPriority, FinalGridThickness);
			}

			if (bShowGridYZ)
			{
				const FVector I(0.0f, -Extent, Offset);
				const FVector J(0.0f, Extent, Offset);
				TE_DrawRuntimeLine3D(this, GetWorld(), ToWorld(I), ToWorld(J), GridLineColor, DebugDepthPriority, FinalGridThickness);

				const FVector K(0.0f, Offset, -Extent);
				const FVector L(0.0f, Offset, Extent);
				TE_DrawRuntimeLine3D(this, GetWorld(), ToWorld(K), ToWorld(L), GridLineColor, DebugDepthPriority, FinalGridThickness);
			}
		}
	}
}

void ABezierCurve3DActor::ApplyRuntimeEditVisibility()
{
	// Whole actor
	SetActorHiddenInGame(!bActorVisibleInGame);

	// Visuals only show if enabled, and either in edit mode or not hiding when not editing
	const bool bAllowFade = bEnableVisualFade && (ControlPointFadeAlpha > KINDA_SMALL_NUMBER || StripFadeAlpha > KINDA_SMALL_NUMBER);
	const bool bCanShowControlPointVisuals = bActorVisibleInGame && bEnableRuntimeEditing && (bEditMode || !bHideVisualsWhenNotEditing || bAllowFade);
	const bool bCanShowStripVisuals = bActorVisibleInGame;

	const bool bShowCP = bCanShowControlPointVisuals && bShowControlPoints;
	const bool bShowStrip = bCanShowStripVisuals && bShowStripMesh;
	const bool bShowCPVisual = ControlPointFadeAlpha > KINDA_SMALL_NUMBER;
	const bool bShowStripVisual = StripFadeAlpha > KINDA_SMALL_NUMBER;

	if (ControlPointISM)
	{
		const bool bVisible = bShowCP && bShowCPVisual;
		ControlPointISM->SetHiddenInGame(!bVisible);
		ControlPointISM->SetVisibility(bVisible, true);

		// Important: do NOT block mouse traces unless editing and CP are visible
		ControlPointISM->SetCollisionEnabled((bVisible && bEditMode) ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		ControlPointISM->SetDepthPriorityGroup((ESceneDepthPriorityGroup)TE_ResolveDepthPriority(VisualPriority));
		ControlPointISM->TranslucencySortPriority = TE_ResolveSortPriority(VisualPriority, VisualPriorityBias);
		ControlPointISM->MarkRenderStateDirty();
	}

	if (StripMeshComponent)
	{
		const bool bShowProc = bShowStrip && bShowStripVisual && !bUseCubeStrip;
		StripMeshComponent->SetHiddenInGame(!bShowProc);
		StripMeshComponent->SetVisibility(bShowProc, true);
		StripMeshComponent->SetDepthPriorityGroup((ESceneDepthPriorityGroup)TE_ResolveDepthPriority(VisualPriority));
		StripMeshComponent->TranslucencySortPriority = TE_ResolveSortPriority(VisualPriority, VisualPriorityBias);
		StripMeshComponent->MarkRenderStateDirty();
	}

	if (CubeStripISM)
	{
		const bool bShowCube = bShowStrip && bShowStripVisual && bUseCubeStrip;
		CubeStripISM->SetHiddenInGame(!bShowCube);
		CubeStripISM->SetVisibility(bShowCube, true);
		CubeStripISM->SetDepthPriorityGroup((ESceneDepthPriorityGroup)TE_ResolveDepthPriority(VisualPriority));
		CubeStripISM->TranslucencySortPriority = TE_ResolveSortPriority(VisualPriority, VisualPriorityBias);
		CubeStripISM->MarkRenderStateDirty();
	}
}

void ABezierCurve3DActor::UpdateVisualFadeTargets()
{
	const bool bCanShowControlPointVisuals = bActorVisibleInGame && bEnableRuntimeEditing && (bEditMode || !bHideVisualsWhenNotEditing);
	const bool bCanShowStripVisuals = bActorVisibleInGame;
	TargetControlPointFade = (bCanShowControlPointVisuals && bShowControlPoints) ? 1.0f : 0.0f;
	TargetStripFade = (bCanShowStripVisuals && bShowStripMesh) ? 1.0f : 0.0f;

	if (!bEnableVisualFade)
	{
		ControlPointFadeAlpha = TargetControlPointFade;
		StripFadeAlpha = TargetStripFade;
	}
}

void ABezierCurve3DActor::UpdateVisualFade(float DeltaSeconds)
{
	if (!bEnableVisualFade)
	{
		return;
	}

	ControlPointFadeAlpha = FMath::FInterpTo(ControlPointFadeAlpha, TargetControlPointFade, DeltaSeconds, VisualFadeSpeed);
	StripFadeAlpha = FMath::FInterpTo(StripFadeAlpha, TargetStripFade, DeltaSeconds, VisualFadeSpeed);
}

void ABezierCurve3DActor::UpdateControlPointInstanceColors()
{
	if (!ControlPointISM) return;

	const float Alpha = GetControlPointPulseOpacity();
	const int32 Count = ControlPointISM->GetInstanceCount();
	for (int32 i = 0; i < Count; ++i)
	{
		FLinearColor C = ControlPointColor;
		if (bSelectAllControlPoints)
		{
			C = ControlPointSelectedColor;
		}
		else if (i == SelectedControlPointIndex)
		{
			C = ControlPointSelectedColor;
		}
		else if (i == HoveredControlPointIndex)
		{
			C = ControlPointHoverColor;
		}

		SetInstanceColorRGB3D(ControlPointISM, i, C, Alpha);
	}

	ControlPointISM->MarkRenderStateDirty();
}

void ABezierCurve3DActor::UpdateControlPointInstanceScale(float InScale)
{
	if (!ControlPointISM) return;

	const int32 Count = ControlPointISM->GetInstanceCount();
	for (int32 i = 0; i < Count; ++i)
	{
		FTransform Xf;
		if (!ControlPointISM->GetInstanceTransform(i, Xf, false)) continue;
		Xf.SetScale3D(FVector(InScale));
		ControlPointISM->UpdateInstanceTransform(i, Xf, false, i == Count - 1, true);
	}
}

float ABezierCurve3DActor::GetControlPointPulseScale() const
{
	const float BaseScale = ControlPointVisualScale;
	if (!bPulseControlPoints || !GetWorld())
	{
		return BaseScale;
	}

	const float PulseAlpha = (FMath::Sin(GetWorld()->GetTimeSeconds() * ControlPointPulseSpeed) + 1.0f) * 0.5f;
	const float PulseScale = FMath::Lerp(ControlPointPulseMinScale, ControlPointPulseMaxScale, PulseAlpha);
	return BaseScale * PulseScale;
}

void ABezierCurve3DActor::UpdateControlPointPulse()
{
	if (!ControlPointISM) return;

	const float TargetScale = GetControlPointPulseScale();
	const float TargetOpacity = GetControlPointPulseOpacity();
	if (FMath::IsNearlyEqual(TargetScale, CachedControlPointScale))
	{
		if (!FMath::IsNearlyEqual(TargetOpacity, CachedControlPointOpacity))
		{
			UpdateControlPointInstanceColors();
			CachedControlPointOpacity = TargetOpacity;
		}
	}
	else
	{
		UpdateControlPointInstanceScale(TargetScale);
		CachedControlPointScale = TargetScale;
		if (!FMath::IsNearlyEqual(TargetOpacity, CachedControlPointOpacity))
		{
			UpdateControlPointInstanceColors();
			CachedControlPointOpacity = TargetOpacity;
		}
	}
}

float ABezierCurve3DActor::GetStripPulseAlpha() const
{
	if (!bPulseStrip || !GetWorld())
	{
		return 1.0f;
	}

	return (FMath::Sin(GetWorld()->GetTimeSeconds() * StripPulseSpeed) + 1.0f) * 0.5f;
}

float ABezierCurve3DActor::GetControlPointPulseOpacity() const
{
	const float FadeAlpha = bEnableVisualFade ? ControlPointFadeAlpha : 1.0f;
	if (!bPulseControlPoints || !GetWorld())
	{
		return FMath::Clamp(FadeAlpha, 0.0f, 1.0f);
	}

	const float Alpha = (FMath::Sin(GetWorld()->GetTimeSeconds() * ControlPointPulseSpeed) + 1.0f) * 0.5f;
	const float PulseAlpha = FMath::Lerp(
		FMath::Min(ControlPointPulseMinAlpha, ControlPointPulseMaxAlpha),
		FMath::Max(ControlPointPulseMinAlpha, ControlPointPulseMaxAlpha),
		Alpha);
	return FMath::Clamp(FadeAlpha * PulseAlpha, 0.0f, 1.0f);
}

float ABezierCurve3DActor::GetStripWidthForRender() const
{
	const float PulseScale = bPulseStrip && GetWorld()
		? FMath::Lerp(StripPulseMinWidth, StripPulseMaxWidth, GetStripPulseAlpha())
		: 1.0f;
	return StripWidth * PulseScale;
}

float ABezierCurve3DActor::GetStripThicknessForRender() const
{
	const float PulseScale = bPulseStrip && GetWorld()
		? FMath::Lerp(StripPulseMinThickness, StripPulseMaxThickness, GetStripPulseAlpha())
		: 1.0f;
	return StripThickness * PulseScale;
}

float ABezierCurve3DActor::GetStripPulseOpacity() const
{
	const float FadeAlpha = bEnableVisualFade ? StripFadeAlpha : 1.0f;
	if (!bPulseStrip || !GetWorld())
	{
		return FMath::Clamp(FadeAlpha, 0.0f, 1.0f);
	}

	const float PulseAlpha = FMath::Lerp(
		FMath::Min(StripPulseMinAlpha, StripPulseMaxAlpha),
		FMath::Max(StripPulseMinAlpha, StripPulseMaxAlpha),
		GetStripPulseAlpha());
	return FMath::Clamp(FadeAlpha * PulseAlpha, 0.0f, 1.0f);
}

void ABezierCurve3DActor::UpdateStripPulseOpacity()
{
	const float TargetOpacity = GetStripPulseOpacity();
	if (FMath::IsNearlyEqual(TargetOpacity, CachedStripOpacity))
	{
		return;
	}

	if (StripMeshComponent)
	{
		StripMeshComponent->SetCustomPrimitiveDataFloat(0, TargetOpacity);
	}

	if (CubeStripISM)
	{
		CubeStripISM->SetCustomPrimitiveDataFloat(0, TargetOpacity);
	}

	CachedStripOpacity = TargetOpacity;
}

void ABezierCurve3DActor::RefreshControlPointVisuals()
{
	if (!ControlPointISM) return;
	if (ControlPointISM->NumCustomDataFloats < 4) ControlPointISM->NumCustomDataFloats = 4;

	if (ControlPointMaterial)
	{
		ControlPointISM->SetMaterial(0, ControlPointMaterial);
	}

	ControlPointISM->ClearInstances();

	// Build instances even if hidden, because UI toggles want to show instantly
	if (!bEnableRuntimeEditing)
	{
		CachedControlPointScale = -1.0f;
		return;
	}

	const float VisualScale = ControlPointVisualScale;

	for (const FVector& P : Control)
	{
		FTransform InstanceXf;
		InstanceXf.SetLocation(P * Scale);
		InstanceXf.SetRotation(FQuat::Identity);
		InstanceXf.SetScale3D(FVector(VisualScale));
		ControlPointISM->AddInstance(InstanceXf);
	}

	UpdateControlPointInstanceColors();
	CachedControlPointScale = -1.0f;
}

void ABezierCurve3DActor::UpdateCubeStrip()
{
	if (!CubeStripISM)
	{
		return;
	}

	CubeStripISM->ClearInstances();
	if (StripMaterial)
	{
		CubeStripISM->SetMaterial(0, StripMaterial);
	}

	if (Control.Num() < 2)
	{
		return;
	}

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);
	const float CubeSizeCm = 100.0f;
	const float WidthScale = FMath::Max(0.001f, GetStripWidthForRender()) / CubeSizeCm;
	const float ThickScale = FMath::Max(0.001f, GetStripThicknessForRender()) / CubeSizeCm;

	// Instance transforms are local to this actor/component. Do not use
	// GetActorUpVector() here, because that is world-space and can offset the
	// generated strip when the actor is rotated.
	const FVector LocalUp = FVector::UpVector;
	const FVector LocalRight = FVector::RightVector;

	for (int32 i = 0; i < Segs; ++i)
	{
		const double t0 = static_cast<double>(i) / static_cast<double>(Segs);
		const double t1 = static_cast<double>(i + 1) / static_cast<double>(Segs);

		const FVector P0 = Eval(t0) * Scale;
		const FVector P1 = Eval(t1) * Scale;
		const FVector Dir = P1 - P0;
		const float Len = Dir.Size();

		if (Len < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector X = Dir / Len;
		FVector Side = FVector::CrossProduct(LocalUp, X).GetSafeNormal();

		if (Side.IsNearlyZero())
		{
			Side = FVector::CrossProduct(LocalRight, X).GetSafeNormal();
		}

		if (Side.IsNearlyZero())
		{
			continue;
		}

		const FVector TrueUp = FVector::CrossProduct(X, Side).GetSafeNormal();
		const FVector Mid = (P0 + P1) * 0.5f;

		FTransform Xf;
		Xf.SetLocation(Mid);
		Xf.SetRotation(FRotationMatrix::MakeFromXZ(X, TrueUp).ToQuat());
		Xf.SetScale3D(FVector(Len / CubeSizeCm, WidthScale, ThickScale));
		CubeStripISM->AddInstance(Xf);
	}
}


void ABezierCurve3DActor::UpdateStripMesh()
{
	if (!bShowStripMesh)
	{
		if (StripMeshComponent)
		{
			StripMeshComponent->ClearAllMeshSections();
		}

		if (CubeStripISM)
		{
			CubeStripISM->ClearInstances();
		}

		return;
	}

	if (bUseCubeStrip)
	{
		if (StripMeshComponent)
		{
			StripMeshComponent->ClearAllMeshSections();
		}

		UpdateCubeStrip();
		return;
	}

	if (CubeStripISM)
	{
		CubeStripISM->ClearInstances();
	}

	if (!StripMeshComponent || Control.Num() < 2)
	{
		if (StripMeshComponent)
		{
			StripMeshComponent->ClearAllMeshSections();
		}

		return;
	}

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector2D> UVs;

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);
	const float EffectiveWidth = FMath::Max(0.001f, GetStripWidthForRender());

	// Procedural mesh vertices are local to this actor/component. Do not use
	// GetActorUpVector() here, because that is world-space and can offset the
	// generated strip when the actor is rotated.
	const FVector LocalUp = FVector::UpVector;
	const FVector LocalRight = FVector::RightVector;

	for (int32 i = 0; i <= Segs; ++i)
	{
		const double t = static_cast<double>(i) / static_cast<double>(Segs);
		const FVector P = Eval(t) * Scale;

		const double TangentStep = 0.001;
		const double TA = FMath::Clamp(t - TangentStep, 0.0, 1.0);
		const double TB = FMath::Clamp(t + TangentStep, 0.0, 1.0);

		FVector Tangent = (Eval(TB) * Scale - Eval(TA) * Scale).GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = FVector::ForwardVector;
		}

		FVector Side = FVector::CrossProduct(Tangent, LocalUp).GetSafeNormal();
		if (Side.IsNearlyZero())
		{
			Side = FVector::CrossProduct(Tangent, LocalRight).GetSafeNormal();
		}

		if (Side.IsNearlyZero())
		{
			Side = FVector::RightVector;
		}

		Verts.Add(P + (Side * EffectiveWidth * 0.5f));
		Verts.Add(P - (Side * EffectiveWidth * 0.5f));

		UVs.Add(FVector2D(static_cast<float>(t), 0.0f));
		UVs.Add(FVector2D(static_cast<float>(t), 1.0f));

		if (i < Segs)
		{
			const int32 B = i * 2;
			Tris.Add(B);
			Tris.Add(B + 1);
			Tris.Add(B + 2);

			Tris.Add(B + 2);
			Tris.Add(B + 1);
			Tris.Add(B + 3);
		}
	}

	StripMeshComponent->CreateMeshSection_LinearColor(0, Verts, Tris, TArray<FVector>(), UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	if (StripMaterial)
	{
		StripMeshComponent->SetMaterial(0, StripMaterial);
	}
}


// --- UI Runtime Edit functions that were declared but missing ---
void ABezierCurve3DActor::UI_SetEditMode(bool bInEditMode)
{
	bEditMode = bInEditMode;
	if (!bEditMode)
	{
		HoveredControlPointIndex = -1;
		SelectedControlPointIndex = SelectedControlPointIndex; // keep selection if you want, or clear it
	}
	ApplyRuntimeEditVisibility();
	UpdateControlPointInstanceColors();
}

void ABezierCurve3DActor::UI_SetActorVisibleInGame(bool bInVisible)
{
	bActorVisibleInGame = bInVisible;
	ApplyRuntimeEditVisibility();
}

void ABezierCurve3DActor::UI_SetShowStrip(bool bInShow)
{
	bShowStripMesh = bInShow;
	UpdateStripMesh();
	ApplyRuntimeEditVisibility();
}

void ABezierCurve3DActor::UI_SetShowCubeStrip(bool bInShow)
{
	bUseCubeStrip = bInShow;
	UpdateStripMesh();
	ApplyRuntimeEditVisibility();
}

void ABezierCurve3DActor::UI_SetStripSize(float InWidth, float InThickness)
{
	StripWidth = FMath::Max(0.001f, InWidth);
	StripThickness = FMath::Max(0.001f, InThickness);
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_SetShowControlPoints(bool bInShow)
{
	bShowControlPoints = bInShow;
	ApplyRuntimeEditVisibility();
}

void ABezierCurve3DActor::UI_SetSamplingMode(EBezierSamplingMode InMode)
{
	SamplingMode = InMode;
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_SetSampleCount(int32 InCount)
{
	StripSegments = FMath::Clamp(InCount, 2, 512);
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_SetControlPointSize(float InVisualScale)
{
	ControlPointVisualScale = FMath::Max(0.0001f, InVisualScale);
	RefreshControlPointVisuals();
}

void ABezierCurve3DActor::UI_SetControlPointColors(FLinearColor InNormal, FLinearColor InHover, FLinearColor InSelected)
{
	ControlPointColor = InNormal;
	ControlPointHoverColor = InHover;
	ControlPointSelectedColor = InSelected;
	UpdateControlPointInstanceColors();
}

void ABezierCurve3DActor::UI_SetHoveredControlPoint(int32 Index)
{
	if (HoveredControlPointIndex == Index) return;
	HoveredControlPointIndex = Index;
	UpdateControlPointInstanceColors();
}

void ABezierCurve3DActor::UI_ClearHoveredControlPoint()
{
	if (HoveredControlPointIndex == -1) return;
	HoveredControlPointIndex = -1;
	UpdateControlPointInstanceColors();
}

// --- Core operations / IO ---
void ABezierCurve3DActor::SyncControlFromSpline()
{
	ReadSplineToControl();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::OverwriteSplineFromControl()
{
	if (bForcePlanar)
	{
		ForcePlanarBaseControl = Control;
		bForcePlanarHasBase = true;
	}

	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::ReverseControlOrder()
{
	Algo::Reverse(Control);
	OverwriteSplineFromControl();
}

void ABezierCurve3DActor::ResampleBezierToSpline()
{
	if (Control.Num() < 2)
	{
		OverwriteSplineFromControl();
		return;
	}

	const int32 TargetCount = FMath::Clamp(StripSegments, 3, 512);
	TArray<FVector> Samples;
	TEBezier::SampleUniform(Control, TargetCount, Samples);

	if (Samples.Num() < 2)
	{
		OverwriteSplineFromControl();
		return;
	}

	Control = Samples;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

bool ABezierCurve3DActor::ExportControlPointsToJson() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> ControlValues;
	ControlValues.Reserve(Control.Num());

	for (const FVector& P : Control)
	{
		TArray<TSharedPtr<FJsonValue>> PointValues;
		PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
		PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
		PointValues.Add(MakeShared<FJsonValueNumber>(P.Z));
		ControlValues.Add(MakeShared<FJsonValueArray>(PointValues));
	}

	Root->SetArrayField(TEXT("control"), ControlValues);

	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("control3d.json"), TEXT("Bezier"));
	return TE_FileUtils::SaveJson(Path, Root);
}

bool ABezierCurve3DActor::ExportCurveSamplesToJson() const
{
	TArray<FVector> Samples;
	const int32 SampleCount = FMath::Clamp(StripSegments, 8, 4096);
	SampleCurvePoints(SampleCount, Samples);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> SampleValues;
	SampleValues.Reserve(Samples.Num());

	for (const FVector& P : Samples)
	{
		TArray<TSharedPtr<FJsonValue>> PointValues;
		PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
		PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
		PointValues.Add(MakeShared<FJsonValueNumber>(P.Z));
		SampleValues.Add(MakeShared<FJsonValueArray>(PointValues));
	}

	Root->SetArrayField(TEXT("samples"), SampleValues);

	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("samples3d.json"), TEXT("Bezier"));
	return TE_FileUtils::SaveJson(Path, Root);
}

bool ABezierCurve3DActor::ExportDeCasteljauProofJson() const
{
	TArray<TArray<FVector>> Levels;
	TEBezier::DeCasteljauLevels(Control, ProofT, Levels);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> LevelValues;
	LevelValues.Reserve(Levels.Num());

	for (const TArray<FVector>& Level : Levels)
	{
		TArray<TSharedPtr<FJsonValue>> LevelPoints;
		LevelPoints.Reserve(Level.Num());
		for (const FVector& P : Level)
		{
			TArray<TSharedPtr<FJsonValue>> PointValues;
			PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
			PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
			PointValues.Add(MakeShared<FJsonValueNumber>(P.Z));
			LevelPoints.Add(MakeShared<FJsonValueArray>(PointValues));
		}
		LevelValues.Add(MakeShared<FJsonValueArray>(LevelPoints));
	}

	Root->SetArrayField(TEXT("levels"), LevelValues);

	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("proof3d.json"), TEXT("Bezier"));
	return TE_FileUtils::SaveJson(Path, Root);
}

// --- UI Logic ---
void ABezierCurve3DActor::UI_ResetCurveState()
{
	Control = (InitialControl.Num() >= 2) ? InitialControl : TArray<FVector>{ FVector(0,0,0), FVector(1,0,0) };
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_SetInitialControlFromCurrent()
{
	InitialControl = Control;
}

void ABezierCurve3DActor::UI_CenterCurve()
{
	if (Control.Num() == 0) return;
	FVector Avg = FVector::ZeroVector;
	for (const FVector& P : Control) Avg += P;
	Avg /= Control.Num();
	for (FVector& P : Control) P -= Avg;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_MirrorCurveX()
{
	if (Control.Num() == 0) return;
	FVector Pivot = FVector::ZeroVector;
	for (const FVector& P : Control) Pivot += P;
	Pivot /= static_cast<double>(Control.Num());
	for (FVector& P : Control)
	{
		P.X = (2.0 * Pivot.X) - P.X;
	}
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_MirrorCurveY()
{
	if (Control.Num() == 0) return;
	FVector Pivot = FVector::ZeroVector;
	for (const FVector& P : Control) Pivot += P;
	Pivot /= static_cast<double>(Control.Num());
	for (FVector& P : Control)
	{
		P.Y = (2.0 * Pivot.Y) - P.Y;
	}
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_MirrorCurveZ()
{
	if (Control.Num() == 0) return;
	FVector Pivot = FVector::ZeroVector;
	for (const FVector& P : Control) Pivot += P;
	Pivot /= static_cast<double>(Control.Num());
	for (FVector& P : Control)
	{
		P.Z = (2.0 * Pivot.Z) - P.Z;
	}
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_ToggleClosedLoop()
{
	if (Spline)
	{
		Spline->SetClosedLoop(!Spline->IsClosedLoop());
		Spline->UpdateSpline();
		UpdateStripMesh();
	}
}

void ABezierCurve3DActor::UI_SetClosedLoop(bool bInClosed)
{
	if (Spline)
	{
		Spline->SetClosedLoop(bInClosed);
		Spline->UpdateSpline();
		UpdateStripMesh();
	}
}

bool ABezierCurve3DActor::UI_IsClosedLoop() const
{
	return Spline ? Spline->IsClosedLoop() : false;
}

void ABezierCurve3DActor::UI_ResampleParam()
{
	ResampleBezierToSpline();
}

void ABezierCurve3DActor::UI_ResampleArcLength()
{
	if (Control.Num() < 2)
	{
		OverwriteSplineFromControl();
		return;
	}

	const int32 TargetCount = FMath::Clamp(StripSegments, 3, 512);
	TArray<FVector> Samples;
	TEBezier::UniformArcLengthSample(Control, TargetCount, Samples);

	if (Samples.Num() < 2)
	{
		OverwriteSplineFromControl();
		return;
	}

	Control = Samples;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_ReverseControlOrder() { ReverseControlOrder(); }

void ABezierCurve3DActor::UI_SyncControlFromSpline()
{
	SyncControlFromSpline();
}

void ABezierCurve3DActor::UI_OverwriteSplineFromControl()
{
	OverwriteSplineFromControl();
}

void ABezierCurve3DActor::UI_SetForcePlanar(bool bInForce)
{
	if (bInForce)
	{
		const EBezierPlanarAxis AxisToSet = (ForcePlanarAxis == EBezierPlanarAxis::None) ? EBezierPlanarAxis::XY : ForcePlanarAxis;
		UI_SetForcePlanarAxis(AxisToSet);
		return;
	}

	bForcePlanar = false;
	ForcePlanarAxis = EBezierPlanarAxis::None;
	if (bForcePlanarHasBase && ForcePlanarBaseControl.Num() == Control.Num())
	{
		Control = ForcePlanarBaseControl;
		WriteControlToSpline();
		RefreshControlPointVisuals();
		UpdateStripMesh();
	}
	bForcePlanarHasBase = false;
	ForcePlanarPlaneOffsetLocal = 0.0f;
	ForcePlanarBaseControl.Reset();
}

void ABezierCurve3DActor::UI_SetForcePlanarAxis(EBezierPlanarAxis InAxis)
{
	const bool bWasForcePlanar = bForcePlanar;
	ForcePlanarAxis = InAxis;
	bForcePlanar = ForcePlanarAxis != EBezierPlanarAxis::None;

	if (!bForcePlanar)
	{
		if (bForcePlanarHasBase && ForcePlanarBaseControl.Num() == Control.Num())
		{
			Control = ForcePlanarBaseControl;
			WriteControlToSpline();
			RefreshControlPointVisuals();
			UpdateStripMesh();
		}
		bForcePlanarHasBase = false;
		ForcePlanarPlaneOffsetLocal = 0.0f;
		ForcePlanarBaseControl.Reset();
		return;
	}

	if (!bWasForcePlanar)
	{
		ForcePlanarBaseControl = Control;
		bForcePlanarHasBase = true;
	}

	const TArray<FVector>& SourceControl = bForcePlanarHasBase ? ForcePlanarBaseControl : Control;
	if (SourceControl.Num() > 0)
	{
		double Sum = 0.0;
		for (const FVector& P : SourceControl)
		{
			switch (ForcePlanarAxis)
			{
			case EBezierPlanarAxis::XY:
				Sum += P.Z;
				break;
			case EBezierPlanarAxis::XZ:
				Sum += P.Y;
				break;
			case EBezierPlanarAxis::YZ:
				Sum += P.X;
				break;
			default:
				break;
			}
		}
		ForcePlanarPlaneOffsetLocal = static_cast<float>(Sum / static_cast<double>(SourceControl.Num()));
	}
	if (SourceControl.Num() == 0)
	{
		return;
	}

	const int32 Count = FMath::Min(SourceControl.Num(), Control.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector LocalPos = SourceControl[Index];
		switch (ForcePlanarAxis)
		{
		case EBezierPlanarAxis::XY:
			LocalPos.Z = ForcePlanarPlaneOffsetLocal;
			break;
		case EBezierPlanarAxis::XZ:
			LocalPos.Y = ForcePlanarPlaneOffsetLocal;
			break;
		case EBezierPlanarAxis::YZ:
			LocalPos.X = ForcePlanarPlaneOffsetLocal;
			break;
		default:
			break;
		}
		Control[Index] = LocalPos;
	}

	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve3DActor::UI_SetSnapToGrid(bool bInSnap)
{
	bSnapToGrid = bInSnap;
	bShowGrid = bInSnap;
}

void ABezierCurve3DActor::UI_SetShowGrid(bool bInShow)
{
	bShowGrid = bInShow;
}

void ABezierCurve3DActor::UI_SetGridSizeCm(float InGridSizeCm)
{
	GridSizeCm = FMath::Max(0.01f, InGridSizeCm);
}

void ABezierCurve3DActor::UI_SetGridSizeCycleValues(const TArray<float>& InValues)
{
	GridSizeCycleValues.Reset();
	for (float Value : InValues)
	{
		if (Value > 0.0f)
		{
			GridSizeCycleValues.Add(Value);
		}
	}
	if (GridSizeCycleValues.Num() == 0)
	{
		GridSizeCycleValues = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	}
	GridSizeCycleIndex = 0;
}

void ABezierCurve3DActor::UI_ResetGridSizeCycleIndex(int32 InIndex)
{
	if (GridSizeCycleValues.Num() > 0)
	{
		GridSizeCycleIndex = FMath::Clamp(InIndex, 0, GridSizeCycleValues.Num() - 1);
	}
	else
	{
		GridSizeCycleIndex = 0;
	}
}

void ABezierCurve3DActor::UI_CycleGridSize()
{
	const TArray<float> Defaults = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	const TArray<float>& Values = GridSizeCycleValues.Num() > 0 ? GridSizeCycleValues : Defaults;
	if (Values.Num() == 0) return;

	GridSizeCycleIndex = (GridSizeCycleIndex + 1) % Values.Num();
	UI_SetGridSizeCm(Values[GridSizeCycleIndex]);
	UI_SetShowGrid(true);
}

void ABezierCurve3DActor::UI_SetGridExtentCm(float InGridExtentCm)
{
	GridExtentCm = FMath::Max(1.0f, InGridExtentCm);
}

void ABezierCurve3DActor::UI_SetGridOriginWorld(FVector InOrigin)
{
	GridOriginWorld = InOrigin;
}

void ABezierCurve3DActor::UI_SetGridColor(FLinearColor InColor)
{
	GridColor = InColor;
}

void ABezierCurve3DActor::UI_SetGridBaseAlpha(float InAlpha)
{
	GridBaseAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
}

void ABezierCurve3DActor::UI_SetShowGridXY(bool bInShow)
{
	bShowGridXY = bInShow;
}

void ABezierCurve3DActor::UI_SetShowGridXZ(bool bInShow)
{
	bShowGridXZ = bInShow;
}

void ABezierCurve3DActor::UI_SetShowGridYZ(bool bInShow)
{
	bShowGridYZ = bInShow;
}

void ABezierCurve3DActor::UI_SetLockToLocalXY(bool bInLock)
{
	bLockToLocalXY = bInLock;
	if (bInLock)
	{
		bLockPlanar = true;
		LockPlanarAxis = EBezierPlanarAxis::XY;
	}
	else if (LockPlanarAxis == EBezierPlanarAxis::XY)
	{
		bLockPlanar = false;
		LockPlanarAxis = EBezierPlanarAxis::None;
	}
}

void ABezierCurve3DActor::UI_SetLockAxis(EBezierPlanarAxis InAxis)
{
	bLockToLocalXY = (InAxis == EBezierPlanarAxis::XY);
	LockPlanarAxis = InAxis;
	bLockPlanar = InAxis != EBezierPlanarAxis::None;
}

EBezierPlanarAxis ABezierCurve3DActor::UI_CycleForcePlanarAxis()
{
	const EBezierPlanarAxis CurrentAxis = bForcePlanar ? ForcePlanarAxis : EBezierPlanarAxis::None;
	EBezierPlanarAxis NextAxis = EBezierPlanarAxis::None;
	switch (CurrentAxis)
	{
	case EBezierPlanarAxis::None:
		NextAxis = EBezierPlanarAxis::XY;
		break;
	case EBezierPlanarAxis::XY:
		NextAxis = EBezierPlanarAxis::XZ;
		break;
	case EBezierPlanarAxis::XZ:
		NextAxis = EBezierPlanarAxis::YZ;
		break;
	case EBezierPlanarAxis::YZ:
		NextAxis = EBezierPlanarAxis::None;
		break;
	default:
		break;
	}

	UI_SetForcePlanarAxis(NextAxis);
	return NextAxis;
}

void ABezierCurve3DActor::UI_AddControlPoint(FVector ModelPos, int32 Index)
{
	if (Index < 0 || Index > Control.Num()) Index = Control.Num();
	Control.Insert(ModelPos, Index);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

bool ABezierCurve3DActor::UI_AddControlPointAfterSelected()
{
	int32 I = SelectedControlPointIndex;
	if (!Control.IsValidIndex(I)) { I = Control.Num() - 1; }
	FVector NewP = (I + 1 < Control.Num()) ? (Control[I] + Control[I + 1]) * 0.5 : Control[I] + FVector(0.5, 0, 0);
	Control.Insert(NewP, I + 1);
	SelectedControlPointIndex = I + 1;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

bool ABezierCurve3DActor::UI_DeleteControlPoint(int32 Index)
{
	if (Control.Num() <= 2 || !Control.IsValidIndex(Index)) return false;
	Control.RemoveAt(Index);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

bool ABezierCurve3DActor::UI_DeleteSelectedControlPoint()
{
	if (bSelectAllControlPoints)
	{
		return Destroy();
	}

	if (Control.IsValidIndex(SelectedControlPointIndex))
	{
		return UI_DeleteControlPoint(SelectedControlPointIndex);
	}

	// Programmatic safety fallback for UI/subsystem calls when no explicit index is set.
	return UI_DeleteControlPoint(Control.Num() - 1);
}

bool ABezierCurve3DActor::UI_DuplicateControlPoint(int32 Index)
{
	if (!Control.IsValidIndex(Index)) return false;
	const FVector Copy = Control[Index];
	Control.Insert(Copy, Index + 1);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}
bool ABezierCurve3DActor::UI_DuplicateSelectedControlPoint()
{
	if (Control.IsValidIndex(SelectedControlPointIndex))
	{
		return UI_DuplicateControlPoint(SelectedControlPointIndex);
	}

	// Programmatic safety fallback for UI/subsystem calls when no explicit index is set.
	return UI_DuplicateControlPoint(0);
}

bool ABezierCurve3DActor::UI_SelectFromHit(const FHitResult& Hit)
{
	int32 HitIndex = INDEX_NONE;
	return UI_TryResolveControlPointFromHit(Hit, HitIndex) && UI_SelectControlPoint(HitIndex);
}

bool ABezierCurve3DActor::UI_TryResolveControlPointFromHit(const FHitResult& Hit, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (Hit.Component.Get() != ControlPointISM || Hit.Item == INDEX_NONE) return false;
	if (!Control.IsValidIndex(Hit.Item)) return false;

	OutIndex = Hit.Item;
	return true;
}

bool ABezierCurve3DActor::UI_SelectControlPoint(int32 Index)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (!Control.IsValidIndex(Index)) return false;

	SelectedControlPointIndex = Index;
	bSelectAllControlPoints = false;
	UpdateControlPointInstanceColors();
	return true;
}

bool ABezierCurve3DActor::UI_GetControlPointWorld(int32 Index, FVector& OutWorld) const
{
	if (!Control.IsValidIndex(Index)) return false;
	OutWorld = GetActorTransform().TransformPosition(Control[Index] * Scale);
	return true;
}

bool ABezierCurve3DActor::UI_SetControlPointWorld(int32 Index, const FVector& WorldPos)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (!Control.IsValidIndex(Index)) return false;

	FVector W = WorldPos;
	W -= GridOriginWorld;

	if (bSnapToGrid)
	{
		const float G = FMath::Max(0.01f, GridSizeCm);
		W.X = FMath::GridSnap(W.X, G);
		W.Y = FMath::GridSnap(W.Y, G);
		W.Z = FMath::GridSnap(W.Z, G);
	}

	W += GridOriginWorld;

	FVector Local = GetActorTransform().InverseTransformPosition(W) / Scale;

	if (bForcePlanar)
	{
		switch (ForcePlanarAxis)
		{
		case EBezierPlanarAxis::XY:
			Local.Z = ForcePlanarPlaneOffsetLocal;
			break;
		case EBezierPlanarAxis::XZ:
			Local.Y = ForcePlanarPlaneOffsetLocal;
			break;
		case EBezierPlanarAxis::YZ:
			Local.X = ForcePlanarPlaneOffsetLocal;
			break;
		default:
			break;
		}
	}
	const FVector CurrentLocal = Control[Index];
	if (bLockPlanar)
	{
		switch (LockPlanarAxis)
		{
		case EBezierPlanarAxis::XY:
			Local.Z = CurrentLocal.Z;
			break;
		case EBezierPlanarAxis::XZ:
			Local.Y = CurrentLocal.Y;
			break;
		case EBezierPlanarAxis::YZ:
			Local.X = CurrentLocal.X;
			break;
		default:
			break;
		}
	}
	Control[Index] = Local;

	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

void ABezierCurve3DActor::UI_ClearSelectedControlPoint()
{
	SelectedControlPointIndex = -1;
	bSelectAllControlPoints = false;
	UpdateControlPointInstanceColors();
}

void ABezierCurve3DActor::UI_SelectAllControlPoints()
{
	if (!bEnableRuntimeEditing) return;
	bSelectAllControlPoints = true;
	SelectedControlPointIndex = -1;
	UpdateControlPointInstanceColors();
}

bool ABezierCurve3DActor::UI_GetAllControlPointsWorld(TArray<FVector>& OutWorld) const
{
	OutWorld.Reset();
	if (Control.IsEmpty()) return false;
	OutWorld.Reserve(Control.Num());
	const FTransform ActorXf = GetActorTransform();
	for (const FVector& P : Control)
	{
		OutWorld.Add(ActorXf.TransformPosition(P * Scale));
	}
	return true;
}

bool ABezierCurve3DActor::UI_SetAllControlPointsWorld(const TArray<FVector>& WorldPositions)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (WorldPositions.Num() != Control.Num()) return false;

	TArray<FVector> NewControl;
	NewControl.SetNum(WorldPositions.Num());

	const FTransform ActorXf = GetActorTransform();
	const float G = FMath::Max(0.01f, GridSizeCm);

	for (int32 i = 0; i < WorldPositions.Num(); ++i)
	{
		FVector W = WorldPositions[i];
		W -= GridOriginWorld;

		if (bSnapToGrid)
		{
			W.X = FMath::GridSnap(W.X, G);
			W.Y = FMath::GridSnap(W.Y, G);
			W.Z = FMath::GridSnap(W.Z, G);
		}

		W += GridOriginWorld;

		FVector Local = ActorXf.InverseTransformPosition(W) / Scale;

		if (bForcePlanar)
		{
			switch (ForcePlanarAxis)
			{
			case EBezierPlanarAxis::XY:
				Local.Z = ForcePlanarPlaneOffsetLocal;
				break;
			case EBezierPlanarAxis::XZ:
				Local.Y = ForcePlanarPlaneOffsetLocal;
				break;
			case EBezierPlanarAxis::YZ:
				Local.X = ForcePlanarPlaneOffsetLocal;
				break;
			default:
				break;
			}
		}
		const FVector CurrentLocal = Control[i];
		if (bLockPlanar)
		{
			switch (LockPlanarAxis)
			{
			case EBezierPlanarAxis::XY:
				Local.Z = CurrentLocal.Z;
				break;
			case EBezierPlanarAxis::XZ:
				Local.Y = CurrentLocal.Y;
				break;
			case EBezierPlanarAxis::YZ:
				Local.X = CurrentLocal.X;
				break;
			default:
				break;
			}
		}

		NewControl[i] = Local;
	}

	Control = MoveTemp(NewControl);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

void ABezierCurve3DActor::UI_ImportFromJson()
{
	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("control3d.json"), TEXT("Bezier"));
	TSharedPtr<FJsonObject> Root;
	if (!TE_FileUtils::LoadJson(Path, Root) || !Root.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* ControlArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("control"), ControlArray) || !ControlArray) return;

	TArray<FVector> NewControl;
	for (const TSharedPtr<FJsonValue>& Entry : *ControlArray)
	{
		const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
		if (!Entry.IsValid() || !Entry->TryGetArray(PointArray) || !PointArray || PointArray->Num() < 3) continue;

		const double X = (*PointArray)[0]->AsNumber();
		const double Y = (*PointArray)[1]->AsNumber();
		const double Z = (*PointArray)[2]->AsNumber();
		NewControl.Add(FVector(X, Y, Z));
	}

	if (NewControl.Num() < 2) return;
	Control = NewControl;
	InitialControl = Control;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

// --- IO Logic ---
void ABezierCurve3DActor::UI_ExportAllJson()
{
	ExportControlPointsToJson();
	ExportCurveSamplesToJson();
	ExportDeCasteljauProofJson();
}

bool ABezierCurve3DActor::WriteJson(const FString& AbsPath, const FString& Str) const
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	const FString Dir = FPaths::GetPath(AbsPath);
	if (!PF.DirectoryExists(*Dir))
	{
		PF.CreateDirectoryTree(*Dir);
	}
	return FFileHelper::SaveStringToFile(Str, *AbsPath);
}

bool ABezierCurve3DActor::ReadJson(const FString& AbsPath, FString& OutStr) const
{
	return TE_FileUtils::LoadTextFile(AbsPath, OutStr);
}

FString ABezierCurve3DActor::MakeAbs(const FString& FileName) const
{
	return TE_PathUtils::ResolveFile(IOPathAbsolute, FileName, TEXT("Bezier"));
}

void ABezierCurve3DActor::EnsureSpline()
{
	if (!Spline) return;
	Spline->SetMobility(EComponentMobility::Movable);
}

void ABezierCurve3DActor::ReadSplineToControl()
{
	if (!Spline) return;
	const int32 N = Spline->GetNumberOfSplinePoints();
	Control.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		Control[i] = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local) / Scale;
	}
}

void ABezierCurve3DActor::WriteControlToSpline()
{
	if (!Spline) return;
	Spline->ClearSplinePoints(false);
	for (int32 i = 0; i < Control.Num(); ++i)
	{
		Spline->AddSplinePoint(Control[i] * Scale, ESplineCoordinateSpace::Local, false);
	}
	Spline->UpdateSpline();
}

void ABezierCurve3DActor::UniformArcLengthSample3D(int32 TargetCount, TArray<FVector>& Out) const
{
	TEBezier::UniformArcLengthSample(Control, TargetCount, Out);
}

void ABezierCurve3DActor::SampleCurvePoints(int32 TargetCount, TArray<FVector>& Out) const
{
	Out.Reset();
	if (Control.Num() < 2 || TargetCount < 2)
	{
		return;
	}

	if (SamplingMode == EBezierSamplingMode::ArcLength)
	{
		TEBezier::UniformArcLengthSample(Control, TargetCount, Out);
	}
	else
	{
		TEBezier::SampleUniform(Control, TargetCount, Out);
	}
}

FVector ABezierCurve3DActor::Eval(double T) const
{
	return TEBezier::DeCasteljauEval<FVector>(Control, T);
}

// -------- Interface forwarding --------
FName ABezierCurve3DActor::BEZ_GetTypeName_Implementation() const { return TEXT("Bezier3D"); }

void ABezierCurve3DActor::BEZ_SetEditMode_Implementation(bool bInEditMode) { UI_SetEditMode(bInEditMode); }
bool ABezierCurve3DActor::BEZ_GetEditMode_Implementation() const { return bEditMode; }

void ABezierCurve3DActor::BEZ_SetActorVisibleInGame_Implementation(bool bInVisible) { UI_SetActorVisibleInGame(bInVisible); }
bool ABezierCurve3DActor::BEZ_GetActorVisibleInGame_Implementation() const { return bActorVisibleInGame; }

void ABezierCurve3DActor::BEZ_SetShowControlPoints_Implementation(bool bInShow) { UI_SetShowControlPoints(bInShow); }
bool ABezierCurve3DActor::BEZ_GetShowControlPoints_Implementation() const { return bShowControlPoints; }

void ABezierCurve3DActor::BEZ_SetShowStrip_Implementation(bool bInShow) { UI_SetShowStrip(bInShow); }
bool ABezierCurve3DActor::BEZ_GetShowStrip_Implementation() const { return bShowStripMesh; }

void ABezierCurve3DActor::BEZ_SetControlPointSize_Implementation(float InVisualScale) { UI_SetControlPointSize(InVisualScale); }
void ABezierCurve3DActor::BEZ_SetStripSize_Implementation(float InWidth, float InThickness) { UI_SetStripSize(InWidth, InThickness); }
void ABezierCurve3DActor::BEZ_SetControlPointColors_Implementation(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected) { UI_SetControlPointColors(Normal, Hover, Selected); }

void ABezierCurve3DActor::BEZ_SetSnapToGrid_Implementation(bool bInSnap) { UI_SetSnapToGrid(bInSnap); }
bool ABezierCurve3DActor::BEZ_GetSnapToGrid_Implementation() const { return bSnapToGrid; }

void ABezierCurve3DActor::BEZ_SetGridSize_Implementation(float InGridSizeCm) { UI_SetGridSizeCm(InGridSizeCm); }
float ABezierCurve3DActor::BEZ_GetGridSize_Implementation() const { return GridSizeCm; }

void ABezierCurve3DActor::BEZ_SetForcePlanar_Implementation(bool bInForce) { UI_SetForcePlanar(bInForce); }
void ABezierCurve3DActor::BEZ_ResetCurveState_Implementation() { UI_ResetCurveState(); }

bool ABezierCurve3DActor::BEZ_AddControlPointAfterSelected_Implementation() { return UI_AddControlPointAfterSelected(); }
bool ABezierCurve3DActor::BEZ_DeleteSelectedControlPoint_Implementation() { return UI_DeleteSelectedControlPoint(); }
bool ABezierCurve3DActor::BEZ_DuplicateSelectedControlPoint_Implementation() { return UI_DuplicateSelectedControlPoint(); }
