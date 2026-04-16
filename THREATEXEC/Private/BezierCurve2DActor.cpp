#include "BezierCurve2DActor.h"

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
#include "Components/LineBatchComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "THREATEXEC_FileUtils.h"
#include "Algo/Reverse.h"

namespace
{
	static ULineBatchComponent* TE_GetRuntimeLineBatcher2D(const UObject* Owner, UWorld* World)
	{
		if (!Owner || !World)
		{
			return nullptr;
		}

		static TMap<TWeakObjectPtr<const UObject>, TWeakObjectPtr<ULineBatchComponent>> LineBatchers2D;
		TWeakObjectPtr<ULineBatchComponent>& CachedBatcher = LineBatchers2D.FindOrAdd(Owner);
		ULineBatchComponent* LineBatcher = CachedBatcher.Get();
		if (!IsValid(LineBatcher))
		{
			LineBatcher = NewObject<ULineBatchComponent>(World, NAME_None, RF_Transient);
			if (LineBatcher)
			{
				LineBatcher->RegisterComponentWithWorld(World);
				CachedBatcher = LineBatcher;
			}
		}
		return LineBatcher;
	}

	static void TE_DrawRuntimeLine2D(const UObject* Owner, UWorld* World, const FVector& Start, const FVector& End, const FColor& Color, uint8 DepthPriority, float Thickness)
	{
		if (!World)
		{
			return;
		}

#if ENABLE_DRAW_DEBUG
		DrawDebugLine(World, Start, End, Color, false, 0.0f, DepthPriority, Thickness);
#else
		ULineBatchComponent* LineBatcher = TE_GetRuntimeLineBatcher2D(Owner, World);
		if (LineBatcher)
		{
			LineBatcher->DrawLine(Start, End, FLinearColor(Color), DepthPriority, Thickness, 0.0f);
		}
#endif
	}

	static void TE_DrawRuntimePoint2D(const UObject* Owner, UWorld* World, const FVector& Position, float PointSize, const FColor& Color, uint8 DepthPriority)
	{
		if (!World)
		{
			return;
		}

#if ENABLE_DRAW_DEBUG
		DrawDebugPoint(World, Position, PointSize, Color, false, 0.0f, DepthPriority);
#else
		ULineBatchComponent* LineBatcher = TE_GetRuntimeLineBatcher2D(Owner, World);
		if (LineBatcher)
		{
			LineBatcher->DrawPoint(Position, FLinearColor(Color), PointSize, DepthPriority, 0.0f);
		}
#endif
	}
}

static void SetInstanceColorRGB2D(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const FLinearColor& C, float Alpha)
{
	if (!ISM) return;
	if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return;
	if (ISM->NumCustomDataFloats < 4) ISM->NumCustomDataFloats = 4;

	ISM->SetCustomDataValue(InstanceIndex, 0, C.R, false);
	ISM->SetCustomDataValue(InstanceIndex, 1, C.G, false);
	ISM->SetCustomDataValue(InstanceIndex, 2, C.B, false);
	ISM->SetCustomDataValue(InstanceIndex, 3, Alpha, true);
}

ABezierCurve2DActor::ABezierCurve2DActor()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(Root);

	ControlPointISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ControlPoints"));
	ControlPointISM->SetupAttachment(Root);
	ControlPointISM->NumCustomDataFloats = 3;
	ControlPointISM->SetCollisionResponseToAllChannels(ECR_Ignore);
	ControlPointISM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ControlPointISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) { ControlPointISM->SetStaticMesh(CubeMesh.Object); }

	StripMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StripMesh"));
	StripMeshComponent->SetupAttachment(Root);
	StripMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CubeStripISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CubeStrip"));
	CubeStripISM->SetupAttachment(Root);
	CubeStripISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMesh.Succeeded()) { CubeStripISM->SetStaticMesh(CubeMesh.Object); }

	if (GridSizeCycleValues.Num() == 0)
	{
		GridSizeCycleValues = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	}
}

void ABezierCurve2DActor::BeginPlay()
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

void ABezierCurve2DActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void ABezierCurve2DActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureSpline();
	if (Control.Num() == 0) { Control = { FVector2D(0,0), FVector2D(1,0) }; WriteControlToSpline(); }

	RefreshControlPointVisuals();
	ApplyRuntimeEditVisibility();
}

#if WITH_EDITOR
bool ABezierCurve2DActor::ShouldTickIfViewportsOnly() const { return true; }
#endif

void ABezierCurve2DActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
#if !ENABLE_DRAW_DEBUG
	if (ULineBatchComponent* RuntimeBatcher = TE_GetRuntimeLineBatcher2D(this, GetWorld()))
	{
		RuntimeBatcher->Flush();
	}
#endif
	const FTransform Xf = GetActorTransform();
	const float PulseAlpha = bPulseDebugLines
		? FMath::Lerp(DebugPulseMinAlpha, DebugPulseMaxAlpha, (FMath::Sin(GetWorld()->GetTimeSeconds() * DebugPulseSpeed) + 1.0f) * 0.5f)
		: DebugPulseMaxAlpha;
	const uint8 DebugAlpha = static_cast<uint8>(FMath::Clamp(PulseAlpha, 0.0f, 1.0f) * 255.0f);
	const float DebugThickness = bPulseDebugLines
		? FMath::Lerp(DebugPulseMinThickness, DebugPulseMaxThickness, (FMath::Sin(GetWorld()->GetTimeSeconds() * DebugPulseSpeed) + 1.0f) * 0.5f)
		: DebugPulseMaxThickness;
	const float GridPulseAlpha = bPulseGrid
		? FMath::Lerp(GridPulseMinAlpha, GridPulseMaxAlpha, (FMath::Sin(GetWorld()->GetTimeSeconds() * GridPulseSpeed) + 1.0f) * 0.5f)
		: GridPulseMaxAlpha;
	const uint8 GridAlpha = static_cast<uint8>(FMath::Clamp(GridPulseAlpha, 0.0f, 1.0f) * 255.0f);
	const float GridThickness = bPulseGrid
		? FMath::Lerp(GridPulseMinThickness, GridPulseMaxThickness, (FMath::Sin(GetWorld()->GetTimeSeconds() * GridPulseSpeed) + 1.0f) * 0.5f)
		: GridPulseMaxThickness;
	const float FinalDebugThickness = FMath::Max(0.01f, DebugThickness * DebugThicknessScale);
	const float FinalGridThickness = FMath::Max(0.01f, GridThickness * GridThicknessScale);
	const uint8 DebugDepthPriority = bForceVisualsOnTop ? SDPG_Foreground : SDPG_World;

	if (bShowControlPolygon && Control.Num() >= 2)
	{
		for (int32 i = 0; i + 1 < Control.Num(); ++i)
		{
			TE_DrawRuntimeLine2D(
				this,
				GetWorld(),
				Xf.TransformPosition(FVector(Control[i].X * Scale, Control[i].Y * Scale, 0)),
				Xf.TransformPosition(FVector(Control[i + 1].X * Scale, Control[i + 1].Y * Scale, 0)),
				FColor(255, 255, 255, DebugAlpha),
				DebugDepthPriority,
				FinalDebugThickness);
		}
	}

	if (bShowLevelsAtT && Control.Num() >= 2)
	{
		TArray<TArray<FVector2D>> Levels;
		TEBezier::DeCasteljauLevels<FVector2D>(Control, FMath::Clamp(ProofT, 0.0, 1.0), Levels);
		for (int32 L = 0; L < Levels.Num(); ++L)
		{
			FColor C = (L == Levels.Num() - 1) ? FColor::Red : FColor(128, 200, 255, 255);
			for (int32 i = 0; i + 1 < Levels[L].Num(); ++i)
			{
				TE_DrawRuntimeLine2D(
					this,
					GetWorld(),
					Xf.TransformPosition(FVector(Levels[L][i].X * Scale, Levels[L][i].Y * Scale, 0.0f)),
					Xf.TransformPosition(FVector(Levels[L][i + 1].X * Scale, Levels[L][i + 1].Y * Scale, 0.0f)),
					FColor(C.R, C.G, C.B, DebugAlpha),
					DebugDepthPriority,
					FinalDebugThickness);
			}
		}
	}

	if (bShowSamplePoints && Control.Num() >= 2)
	{
		TArray<FVector2D> Samples;
		const int32 SampleCount = FMath::Clamp(StripSegments, 2, 512);
		SampleCurvePoints(SampleCount, Samples);
		for (const FVector2D& Sample : Samples)
		{
			TE_DrawRuntimePoint2D(
				this,
				GetWorld(),
				Xf.TransformPosition(FVector(Sample.X * Scale, Sample.Y * Scale, 0.0f)),
				6.0f,
				FColor(64, 220, 255, DebugAlpha),
				DebugDepthPriority);
		}
	}

	if (bShowGrid || bSnapToGrid)
	{
		const float G = FMath::Max(0.01f, GridSizeCm);
		const int32 HalfCells = FMath::Max(1, FMath::RoundToInt(GridExtentCm / G));
		const float Extent = G * HalfCells;
		const float FinalAlpha = FMath::Clamp(GridColor.A * GridBaseAlpha * GridPulseAlpha, 0.0f, 1.0f);
		const uint8 GridColorAlpha = static_cast<uint8>(FinalAlpha * 255.0f);
		const FColor GridLineColor(
			static_cast<uint8>(FMath::Clamp(GridColor.R, 0.0f, 1.0f) * 255.0f),
			static_cast<uint8>(FMath::Clamp(GridColor.G, 0.0f, 1.0f) * 255.0f),
			static_cast<uint8>(FMath::Clamp(GridColor.B, 0.0f, 1.0f) * 255.0f),
			GridColorAlpha
		);
		const FVector Origin(GridOriginWorld.X, GridOriginWorld.Y, GridOriginWorld.Z);
		if (bShowGridXY)
		{
			for (int32 i = -HalfCells; i <= HalfCells; ++i)
			{
				const float Offset = i * G;
				const FVector A(-Extent, Offset, 0.0f);
				const FVector B(Extent, Offset, 0.0f);
					DrawDebugLine(GetWorld(), A + Origin, B + Origin, GridLineColor, false, 0.f, DebugDepthPriority, FinalGridThickness);

				const FVector C(Offset, -Extent, 0.0f);
				const FVector D(Offset, Extent, 0.0f);
					DrawDebugLine(GetWorld(), C + Origin, D + Origin, GridLineColor, false, 0.f, DebugDepthPriority, FinalGridThickness);
			}
		}
	}
}

void ABezierCurve2DActor::ApplyRuntimeEditVisibility()
{
	SetActorHiddenInGame(!bActorVisibleInGame);

	const bool bAllowFade = bEnableVisualFade && (ControlPointFadeAlpha > KINDA_SMALL_NUMBER || StripFadeAlpha > KINDA_SMALL_NUMBER);
	const bool bCanShowVisuals = bActorVisibleInGame && bEnableRuntimeEditing && (bEditMode || !bHideVisualsWhenNotEditing || bAllowFade);

	const bool bShowCP = bCanShowVisuals && bShowControlPoints;
	const bool bShowStrip = bCanShowVisuals && bShowStripMesh;
	const bool bShowCPVisual = ControlPointFadeAlpha > KINDA_SMALL_NUMBER;
	const bool bShowStripVisual = StripFadeAlpha > KINDA_SMALL_NUMBER;

	if (ControlPointISM)
	{
		const bool bVisible = bShowCP && bShowCPVisual;
		ControlPointISM->SetHiddenInGame(!bVisible);
		ControlPointISM->SetVisibility(bVisible, true);
		ControlPointISM->SetCollisionEnabled((bVisible && bEditMode) ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		ControlPointISM->SetDepthPriorityGroup(SDPG_Foreground);
		ControlPointISM->TranslucencySortPriority = FMath::Max(VisualTranslucencySortPriority, 10000);
	}

	if (StripMeshComponent)
	{
		const bool bShowProc = bShowStrip && bShowStripVisual && !bUseCubeStrip;
		StripMeshComponent->SetHiddenInGame(!bShowProc);
		StripMeshComponent->SetVisibility(bShowProc, true);
		StripMeshComponent->SetDepthPriorityGroup(bForceVisualsOnTop ? SDPG_Foreground : SDPG_World);
		StripMeshComponent->TranslucencySortPriority = bForceVisualsOnTop ? VisualTranslucencySortPriority : 0;
	}

	if (CubeStripISM)
	{
		const bool bShowCube = bShowStrip && bShowStripVisual && bUseCubeStrip;
		CubeStripISM->SetHiddenInGame(!bShowCube);
		CubeStripISM->SetVisibility(bShowCube, true);
		CubeStripISM->SetDepthPriorityGroup(bForceVisualsOnTop ? SDPG_Foreground : SDPG_World);
		CubeStripISM->TranslucencySortPriority = bForceVisualsOnTop ? VisualTranslucencySortPriority : 0;
	}
}

void ABezierCurve2DActor::UpdateVisualFadeTargets()
{
	const bool bCanShowVisuals = bActorVisibleInGame && bEnableRuntimeEditing && (bEditMode || !bHideVisualsWhenNotEditing);
	TargetControlPointFade = (bCanShowVisuals && bShowControlPoints) ? 1.0f : 0.0f;
	TargetStripFade = (bCanShowVisuals && bShowStripMesh) ? 1.0f : 0.0f;

	if (!bEnableVisualFade)
	{
		ControlPointFadeAlpha = TargetControlPointFade;
		StripFadeAlpha = TargetStripFade;
	}
}

void ABezierCurve2DActor::UpdateVisualFade(float DeltaSeconds)
{
	if (!bEnableVisualFade)
	{
		return;
	}

	ControlPointFadeAlpha = FMath::FInterpTo(ControlPointFadeAlpha, TargetControlPointFade, DeltaSeconds, VisualFadeSpeed);
	StripFadeAlpha = FMath::FInterpTo(StripFadeAlpha, TargetStripFade, DeltaSeconds, VisualFadeSpeed);
}

void ABezierCurve2DActor::UpdateControlPointInstanceColors()
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

		SetInstanceColorRGB2D(ControlPointISM, i, C, Alpha);
	}

	ControlPointISM->MarkRenderStateDirty();
}

void ABezierCurve2DActor::UpdateControlPointInstanceScale(float InScale)
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

float ABezierCurve2DActor::GetControlPointPulseScale() const
{
	const float FadeAlpha = bEnableVisualFade ? ControlPointFadeAlpha : 1.0f;
	return ControlPointVisualScale * FadeAlpha;
}

void ABezierCurve2DActor::UpdateControlPointPulse()
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

float ABezierCurve2DActor::GetStripPulseAlpha() const
{
	if (!bPulseStrip || !GetWorld())
	{
		return 1.0f;
	}

	return (FMath::Sin(GetWorld()->GetTimeSeconds() * StripPulseSpeed) + 1.0f) * 0.5f;
}

float ABezierCurve2DActor::GetControlPointPulseOpacity() const
{
	const float FadeAlpha = bEnableVisualFade ? ControlPointFadeAlpha : 1.0f;
	if (!bPulseControlPoints || !GetWorld())
	{
		return FadeAlpha;
	}

	const float Alpha = (FMath::Sin(GetWorld()->GetTimeSeconds() * ControlPointPulseSpeed) + 1.0f) * 0.5f;
	return FadeAlpha * FMath::Lerp(ControlPointPulseMinAlpha, ControlPointPulseMaxAlpha, Alpha);
}

float ABezierCurve2DActor::GetStripWidthForRender() const
{
	const float FadeAlpha = bEnableVisualFade ? StripFadeAlpha : 1.0f;
	return StripWidth * FadeAlpha;
}

float ABezierCurve2DActor::GetStripThicknessForRender() const
{
	const float FadeAlpha = bEnableVisualFade ? StripFadeAlpha : 1.0f;
	return StripThickness * FadeAlpha;
}

float ABezierCurve2DActor::GetStripPulseOpacity() const
{
	const float FadeAlpha = bEnableVisualFade ? StripFadeAlpha : 1.0f;
	if (!bPulseStrip || !GetWorld())
	{
		return FadeAlpha;
	}

	return FadeAlpha * FMath::Lerp(StripPulseMinAlpha, StripPulseMaxAlpha, GetStripPulseAlpha());
}

void ABezierCurve2DActor::UpdateStripPulseOpacity()
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

void ABezierCurve2DActor::RefreshControlPointVisuals()
{
	if (!ControlPointISM) return;
	if (ControlPointISM->NumCustomDataFloats < 4) ControlPointISM->NumCustomDataFloats = 4;

	if (ControlPointMaterial)
	{
		ControlPointISM->SetMaterial(0, ControlPointMaterial);
	}

	ControlPointISM->ClearInstances();
	if (!bEnableRuntimeEditing)
	{
		CachedControlPointScale = -1.0f;
		return;
	}

	for (const FVector2D& P : Control)
	{
		FTransform Xf;
		Xf.SetLocation(FVector(P.X * Scale, P.Y * Scale, 0.0f));
		Xf.SetRotation(FQuat::Identity);
		Xf.SetScale3D(FVector(ControlPointVisualScale));
		ControlPointISM->AddInstance(Xf);
	}

	UpdateControlPointInstanceColors();
	CachedControlPointScale = -1.0f;
}

void ABezierCurve2DActor::UpdateCubeStrip()
{
	if (!CubeStripISM) return;

	CubeStripISM->ClearInstances();
	if (StripMaterial) CubeStripISM->SetMaterial(0, StripMaterial);

	if (Control.Num() < 2) return;

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);

	const float CubeSizeCm = 100.0f;
	const float WidthScale = FMath::Max(0.001f, GetStripWidthForRender()) / CubeSizeCm;
	const float ThickScale = FMath::Max(0.001f, GetStripThicknessForRender()) / CubeSizeCm;

	const FVector SideAxis = FVector(0, 0, 1);

	for (int32 i = 0; i < Segs; ++i)
	{
		const double t0 = (double)i / (double)Segs;
		const double t1 = (double)(i + 1) / (double)Segs;

		const FVector2D P0_2 = Eval(t0);
		const FVector2D P1_2 = Eval(t1);

		const FVector P0(P0_2.X * Scale, P0_2.Y * Scale, 0.0f);
		const FVector P1(P1_2.X * Scale, P1_2.Y * Scale, 0.0f);

		const FVector Dir = (P1 - P0);
		const float Len = Dir.Size();
		if (Len < KINDA_SMALL_NUMBER) continue;

		const FVector Mid = (P0 + P1) * 0.5f;
		const FVector X = Dir / Len;

		const FVector Side = FVector::CrossProduct(X, SideAxis).GetSafeNormal();
		const FVector Up = FVector::CrossProduct(Side, X).GetSafeNormal();

		FTransform Xf;
		Xf.SetLocation(Mid);
		Xf.SetRotation(FRotationMatrix::MakeFromXZ(X, Up).ToQuat());
		Xf.SetScale3D(FVector(Len / CubeSizeCm, WidthScale, ThickScale));
		CubeStripISM->AddInstance(Xf);
	}
}

void ABezierCurve2DActor::UpdateStripMesh()
{
	if (!bShowStripMesh) return;

	if (bUseCubeStrip)
	{
		UpdateCubeStrip();
		return;
	}

	if (!StripMeshComponent || Control.Num() < 2) return;

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector2D> UVs;

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);
	const float EffectiveWidth = FMath::Max(0.001f, GetStripWidthForRender());
	const FVector SideAxis = FVector(0, 0, 1);

	for (int32 i = 0; i <= Segs; ++i)
	{
		double t = (double)i / (double)Segs;
		FVector2D P2 = Eval(t);
		FVector P3 = FVector(P2.X * Scale, P2.Y * Scale, 0);

		FVector2D PNext2 = Eval(FMath::Min(t + 0.001, 1.0));
		FVector Tangent = FVector(PNext2.X * Scale - P3.X, PNext2.Y * Scale - P3.Y, 0).GetSafeNormal();
		FVector Side = FVector::CrossProduct(Tangent, SideAxis).GetSafeNormal();

		Verts.Add(P3 + (Side * EffectiveWidth * 0.5f));
		Verts.Add(P3 - (Side * EffectiveWidth * 0.5f));

		UVs.Add(FVector2D((float)t, 0.f));
		UVs.Add(FVector2D((float)t, 1.f));

		if (i < Segs)
		{
			int32 B = i * 2;
			Tris.Add(B); Tris.Add(B + 1); Tris.Add(B + 2);
			Tris.Add(B + 2); Tris.Add(B + 1); Tris.Add(B + 3);
		}
	}

	StripMeshComponent->CreateMeshSection_LinearColor(0, Verts, Tris, TArray<FVector>(), UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	if (StripMaterial) { StripMeshComponent->SetMaterial(0, StripMaterial); }
}

void ABezierCurve2DActor::SampleCurvePoints(int32 TargetCount, TArray<FVector2D>& Out) const
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

// --- Newly added UI runtime edit functions ---
void ABezierCurve2DActor::UI_SetEditMode(bool bInEditMode)
{
	bEditMode = bInEditMode;
	if (!bEditMode)
	{
		HoveredControlPointIndex = -1;
	}
	ApplyRuntimeEditVisibility();
	UpdateControlPointInstanceColors();
}

void ABezierCurve2DActor::UI_SetActorVisibleInGame(bool bInVisible)
{
	bActorVisibleInGame = bInVisible;
	ApplyRuntimeEditVisibility();
}

void ABezierCurve2DActor::UI_SetShowStrip(bool bInShow)
{
	bShowStripMesh = bInShow;
	UpdateStripMesh();
	ApplyRuntimeEditVisibility();
}

void ABezierCurve2DActor::UI_SetShowCubeStrip(bool bInShow)
{
	bUseCubeStrip = bInShow;
	UpdateStripMesh();
	ApplyRuntimeEditVisibility();
}

void ABezierCurve2DActor::UI_SetStripSize(float InWidth, float InThickness)
{
	StripWidth = FMath::Max(0.001f, InWidth);
	StripThickness = FMath::Max(0.001f, InThickness);
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_SetShowControlPoints(bool bInShow)
{
	bShowControlPoints = bInShow;
	ApplyRuntimeEditVisibility();
}

void ABezierCurve2DActor::UI_SetSamplingMode(EBezierSamplingMode InMode)
{
	SamplingMode = InMode;
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_SetSampleCount(int32 InCount)
{
	StripSegments = FMath::Clamp(InCount, 2, 512);
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_SetSnapToGrid(bool bInSnap)
{
	bSnapToGrid = bInSnap;
	bShowGrid = bInSnap;
}

void ABezierCurve2DActor::UI_SetGridSizeCycleValues(const TArray<float>& InValues)
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

void ABezierCurve2DActor::UI_ResetGridSizeCycleIndex(int32 InIndex)
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

void ABezierCurve2DActor::UI_CycleGridSize()
{
	const TArray<float> Defaults = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	const TArray<float>& Values = GridSizeCycleValues.Num() > 0 ? GridSizeCycleValues : Defaults;
	if (Values.Num() == 0) return;

	GridSizeCycleIndex = (GridSizeCycleIndex + 1) % Values.Num();
	UI_SetGridSizeCm(Values[GridSizeCycleIndex]);
	UI_SetShowGrid(true);
}

void ABezierCurve2DActor::UI_SetControlPointSize(float InVisualScale)
{
	ControlPointVisualScale = FMath::Max(0.0001f, InVisualScale);
	RefreshControlPointVisuals();
}

void ABezierCurve2DActor::UI_SetControlPointColors(FLinearColor InNormal, FLinearColor InHover, FLinearColor InSelected)
{
	ControlPointColor = InNormal;
	ControlPointHoverColor = InHover;
	ControlPointSelectedColor = InSelected;
	UpdateControlPointInstanceColors();
}

void ABezierCurve2DActor::UI_SetHoveredControlPoint(int32 Index)
{
	if (HoveredControlPointIndex == Index) return;
	HoveredControlPointIndex = Index;
	UpdateControlPointInstanceColors();
}

void ABezierCurve2DActor::UI_ClearHoveredControlPoint()
{
	if (HoveredControlPointIndex == -1) return;
	HoveredControlPointIndex = -1;
	UpdateControlPointInstanceColors();
}

// --- Core operations / IO ---
void ABezierCurve2DActor::SyncControlFromSpline()
{
	ReadSplineToControl();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve2DActor::OverwriteSplineFromControl()
{
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve2DActor::ReverseControlOrder()
{
	Algo::Reverse(Control);
	OverwriteSplineFromControl();
}

void ABezierCurve2DActor::ResampleBezierToSpline()
{
	if (Control.Num() < 2)
	{
		OverwriteSplineFromControl();
		return;
	}

	const int32 TargetCount = FMath::Clamp(StripSegments, 3, 512);
	TArray<FVector2D> Samples;
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

bool ABezierCurve2DActor::ExportControlPointsToJson() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> ControlValues;
	ControlValues.Reserve(Control.Num());

	for (const FVector2D& P : Control)
	{
		TArray<TSharedPtr<FJsonValue>> PointValues;
		PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
		PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
		ControlValues.Add(MakeShared<FJsonValueArray>(PointValues));
	}

	Root->SetArrayField(TEXT("control"), ControlValues);

	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("control.json"), TEXT("Bezier"));
	return TE_FileUtils::SaveJson(Path, Root);
}

bool ABezierCurve2DActor::ExportCurveSamplesToJson() const
{
	TArray<FVector2D> Samples;
	const int32 SampleCount = FMath::Clamp(StripSegments, 8, 4096);
	SampleCurvePoints(SampleCount, Samples);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> SampleValues;
	SampleValues.Reserve(Samples.Num());

	for (const FVector2D& P : Samples)
	{
		TArray<TSharedPtr<FJsonValue>> PointValues;
		PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
		PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
		SampleValues.Add(MakeShared<FJsonValueArray>(PointValues));
	}

	Root->SetArrayField(TEXT("samples"), SampleValues);

	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("samples.json"), TEXT("Bezier"));
	return TE_FileUtils::SaveJson(Path, Root);
}

bool ABezierCurve2DActor::ExportDeCasteljauProofJson() const
{
	TArray<TArray<FVector2D>> Levels;
	TEBezier::DeCasteljauLevels(Control, ProofT, Levels);

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> LevelValues;
	LevelValues.Reserve(Levels.Num());

	for (const TArray<FVector2D>& Level : Levels)
	{
		TArray<TSharedPtr<FJsonValue>> LevelPoints;
		LevelPoints.Reserve(Level.Num());
		for (const FVector2D& P : Level)
		{
			TArray<TSharedPtr<FJsonValue>> PointValues;
			PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
			PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
			LevelPoints.Add(MakeShared<FJsonValueArray>(PointValues));
		}
		LevelValues.Add(MakeShared<FJsonValueArray>(LevelPoints));
	}

	Root->SetArrayField(TEXT("levels"), LevelValues);

	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("proof.json"), TEXT("Bezier"));
	return TE_FileUtils::SaveJson(Path, Root);
}

// --- Existing functions from your file (kept) ---
void ABezierCurve2DActor::UI_ResetCurveState()
{
	Control = (InitialControl.Num() >= 2) ? InitialControl : TArray<FVector2D>{ FVector2D(0,0), FVector2D(1,0) };
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_SetInitialControlFromCurrent()
{
	InitialControl = Control;
}

void ABezierCurve2DActor::UI_CenterCurve()
{
	if (Control.Num() == 0) return;
	FVector2D Avg = FVector2D::ZeroVector;
	for (const FVector2D& P : Control) Avg += P;
	Avg /= (double)Control.Num();
	for (FVector2D& P : Control) P -= Avg;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_MirrorCurveX()
{
	if (Control.Num() == 0) return;
	FVector2D Pivot = FVector2D::ZeroVector;
	for (const FVector2D& P : Control) Pivot += P;
	Pivot /= static_cast<double>(Control.Num());
	for (FVector2D& P : Control)
	{
		P.X = (2.0 * Pivot.X) - P.X;
	}
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_MirrorCurveY()
{
	if (Control.Num() == 0) return;
	FVector2D Pivot = FVector2D::ZeroVector;
	for (const FVector2D& P : Control) Pivot += P;
	Pivot /= static_cast<double>(Control.Num());
	for (FVector2D& P : Control)
	{
		P.Y = (2.0 * Pivot.Y) - P.Y;
	}
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

void ABezierCurve2DActor::UI_ToggleClosedLoop()
{
	if (!Spline) return;
	Spline->SetClosedLoop(!Spline->IsClosedLoop());
	Spline->UpdateSpline();
}

void ABezierCurve2DActor::UI_SetClosedLoop(bool bInClosed)
{
	if (!Spline) return;
	Spline->SetClosedLoop(bInClosed);
	Spline->UpdateSpline();
}

bool ABezierCurve2DActor::UI_IsClosedLoop() const
{
	return Spline ? Spline->IsClosedLoop() : false;
}

void ABezierCurve2DActor::UI_ReverseControlOrder()
{
	ReverseControlOrder();
}

void ABezierCurve2DActor::UI_ExportAllJson()
{
	ExportControlPointsToJson();
	ExportCurveSamplesToJson();
	ExportDeCasteljauProofJson();
}

void ABezierCurve2DActor::UI_ImportFromJson()
{
	const FString Path = TE_PathUtils::ResolveFile(IOPathAbsolute, TEXT("control.json"), TEXT("Bezier"));
	TSharedPtr<FJsonObject> Root;
	if (!TE_FileUtils::LoadJson(Path, Root) || !Root.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* ControlArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("control"), ControlArray) || !ControlArray) return;

	TArray<FVector2D> NewControl;
	for (const TSharedPtr<FJsonValue>& Entry : *ControlArray)
	{
		const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
		if (!Entry.IsValid() || !Entry->TryGetArray(PointArray) || !PointArray || PointArray->Num() < 2) continue;

		const double X = (*PointArray)[0]->AsNumber();
		const double Y = (*PointArray)[1]->AsNumber();
		NewControl.Add(FVector2D(X, Y));
	}

	if (NewControl.Num() < 2) return;
	Control = NewControl;
	InitialControl = Control;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

bool ABezierCurve2DActor::UI_SelectFromHit(const FHitResult& Hit)
{
	int32 HitIndex = INDEX_NONE;
	return UI_TryResolveControlPointFromHit(Hit, HitIndex) && UI_SelectControlPoint(HitIndex);
}

bool ABezierCurve2DActor::UI_TryResolveControlPointFromHit(const FHitResult& Hit, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (Hit.Component.Get() != ControlPointISM || Hit.Item == INDEX_NONE) return false;
	if (!Control.IsValidIndex(Hit.Item)) return false;

	OutIndex = Hit.Item;
	return true;
}

bool ABezierCurve2DActor::UI_SelectControlPoint(int32 Index)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (!Control.IsValidIndex(Index)) return false;

	SelectedControlPointIndex = Index;
	bSelectAllControlPoints = false;
	UpdateControlPointInstanceColors();
	return true;
}

void ABezierCurve2DActor::UI_AddControlPoint(FVector2D ModelPos, int32 Index)
{
	if (Index < 0 || Index > Control.Num()) Index = Control.Num();
	Control.Insert(ModelPos, Index);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
}

bool ABezierCurve2DActor::UI_AddControlPointAfterSelected()
{
	int32 I = SelectedControlPointIndex;
	if (!Control.IsValidIndex(I)) { I = Control.Num() - 1; }
	const FVector2D NewP = (I + 1 < Control.Num()) ? (Control[I] + Control[I + 1]) * 0.5 : Control[I] + FVector2D(0.5, 0);
	Control.Insert(NewP, I + 1);
	SelectedControlPointIndex = I + 1;
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

bool ABezierCurve2DActor::UI_DeleteControlPoint(int32 Index)
{
	if (Control.Num() <= 2 || !Control.IsValidIndex(Index)) return false;
	Control.RemoveAt(Index);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

bool ABezierCurve2DActor::UI_DeleteSelectedControlPoint()
{
	if (bSelectAllControlPoints)
	{
		return Destroy();
	}

	return UI_DeleteControlPoint(SelectedControlPointIndex);
}

bool ABezierCurve2DActor::UI_DuplicateControlPoint(int32 Index)
{
	if (!Control.IsValidIndex(Index)) return false;
	const FVector2D Copy = Control[Index];
	Control.Insert(Copy, Index + 1);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

bool ABezierCurve2DActor::UI_DuplicateSelectedControlPoint()
{
	return UI_DuplicateControlPoint(SelectedControlPointIndex);
}

bool ABezierCurve2DActor::UI_GetControlPointWorld(int32 Index, FVector& OutWorld) const
{
	if (!Control.IsValidIndex(Index)) return false;
	OutWorld = GetActorTransform().TransformPosition(FVector(Control[Index].X * Scale, Control[Index].Y * Scale, 0));
	return true;
}

bool ABezierCurve2DActor::UI_SetControlPointWorld(int32 Index, const FVector& WorldPos)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (!Control.IsValidIndex(Index)) return false;

	FVector W = WorldPos;
	const FVector Origin(GridOriginWorld.X, GridOriginWorld.Y, GridOriginWorld.Z);
	W -= Origin;

	// 2D stays planar
	if (bForcePlanar) W.Z = 0.0f;

	if (bSnapToGrid)
	{
		const float G = FMath::Max(0.01f, GridSizeCm);
		W.X = FMath::GridSnap(W.X, G);
		W.Y = FMath::GridSnap(W.Y, G);
		W.Z = 0.0f;
	}

	W += Origin;
	const FVector Local = GetActorTransform().InverseTransformPosition(W);
	Control[Index] = FVector2D(Local.X / Scale, Local.Y / Scale);

	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

void ABezierCurve2DActor::UI_ClearSelectedControlPoint()
{
	SelectedControlPointIndex = -1;
	bSelectAllControlPoints = false;
	UpdateControlPointInstanceColors();
}

void ABezierCurve2DActor::UI_SelectAllControlPoints()
{
	if (!bEnableRuntimeEditing || !bEditMode) return;
	bSelectAllControlPoints = true;
	SelectedControlPointIndex = -1;
	UpdateControlPointInstanceColors();
}

bool ABezierCurve2DActor::UI_GetAllControlPointsWorld(TArray<FVector>& OutWorld) const
{
	OutWorld.Reset();
	if (Control.IsEmpty()) return false;
	OutWorld.Reserve(Control.Num());
	const FTransform ActorXf = GetActorTransform();
	for (const FVector2D& P : Control)
	{
		OutWorld.Add(ActorXf.TransformPosition(FVector(P.X * Scale, P.Y * Scale, 0.0f)));
	}
	return true;
}

bool ABezierCurve2DActor::UI_SetAllControlPointsWorld(const TArray<FVector>& WorldPositions)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (WorldPositions.Num() != Control.Num()) return false;

	TArray<FVector2D> NewControl;
	NewControl.SetNum(WorldPositions.Num());

	const FVector Origin(GridOriginWorld.X, GridOriginWorld.Y, GridOriginWorld.Z);
	const FTransform ActorXf = GetActorTransform();
	const float G = FMath::Max(0.01f, GridSizeCm);

	for (int32 i = 0; i < WorldPositions.Num(); ++i)
	{
		FVector W = WorldPositions[i];
		W -= Origin;

		if (bForcePlanar) W.Z = 0.0f;

		if (bSnapToGrid)
		{
			W.X = FMath::GridSnap(W.X, G);
			W.Y = FMath::GridSnap(W.Y, G);
			W.Z = 0.0f;
		}

		W += Origin;
		const FVector Local = ActorXf.InverseTransformPosition(W);
		NewControl[i] = FVector2D(Local.X / Scale, Local.Y / Scale);
	}

	Control = MoveTemp(NewControl);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

FVector2D ABezierCurve2DActor::Eval(double T) const
{
	return TEBezier::DeCasteljauEval<FVector2D>(Control, T);
}

void ABezierCurve2DActor::EnsureSpline()
{
	if (!Spline) return;
	Spline->SetMobility(EComponentMobility::Movable);
}

void ABezierCurve2DActor::ReadSplineToControl()
{
	if (!Spline) return;
	const int32 N = Spline->GetNumberOfSplinePoints();
	Control.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		const FVector P = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
		Control[i] = FVector2D(P.X / Scale, P.Y / Scale);
	}
}

void ABezierCurve2DActor::WriteControlToSpline()
{
	if (!Spline) return;
	Spline->ClearSplinePoints(false);
	for (const FVector2D& P : Control)
	{
		Spline->AddSplinePoint(FVector(P.X * Scale, P.Y * Scale, 0.0f), ESplineCoordinateSpace::Local, false);
	}
	Spline->UpdateSpline();
}

FString ABezierCurve2DActor::MakeAbs(const FString& FileName) const
{
	return TE_PathUtils::ResolveFile(IOPathAbsolute, FileName, TEXT("Bezier"));
}

// -------- Interface forwarding --------
FName ABezierCurve2DActor::BEZ_GetTypeName_Implementation() const { return TEXT("Bezier2D"); }

void ABezierCurve2DActor::BEZ_SetEditMode_Implementation(bool bInEditMode) { UI_SetEditMode(bInEditMode); }
bool ABezierCurve2DActor::BEZ_GetEditMode_Implementation() const { return bEditMode; }

void ABezierCurve2DActor::BEZ_SetActorVisibleInGame_Implementation(bool bInVisible) { UI_SetActorVisibleInGame(bInVisible); }
bool ABezierCurve2DActor::BEZ_GetActorVisibleInGame_Implementation() const { return bActorVisibleInGame; }

void ABezierCurve2DActor::BEZ_SetShowControlPoints_Implementation(bool bInShow) { UI_SetShowControlPoints(bInShow); }
bool ABezierCurve2DActor::BEZ_GetShowControlPoints_Implementation() const { return bShowControlPoints; }

void ABezierCurve2DActor::BEZ_SetShowStrip_Implementation(bool bInShow) { UI_SetShowStrip(bInShow); }
bool ABezierCurve2DActor::BEZ_GetShowStrip_Implementation() const { return bShowStripMesh; }

void ABezierCurve2DActor::BEZ_SetControlPointSize_Implementation(float InVisualScale) { UI_SetControlPointSize(InVisualScale); }
void ABezierCurve2DActor::BEZ_SetStripSize_Implementation(float InWidth, float InThickness) { UI_SetStripSize(InWidth, InThickness); }
void ABezierCurve2DActor::BEZ_SetControlPointColors_Implementation(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected) { UI_SetControlPointColors(Normal, Hover, Selected); }

void ABezierCurve2DActor::BEZ_SetSnapToGrid_Implementation(bool bInSnap) { UI_SetSnapToGrid(bInSnap); }
bool ABezierCurve2DActor::BEZ_GetSnapToGrid_Implementation() const { return bSnapToGrid; }

void ABezierCurve2DActor::BEZ_SetGridSize_Implementation(float InGridSizeCm) { UI_SetGridSizeCm(InGridSizeCm); }
float ABezierCurve2DActor::BEZ_GetGridSize_Implementation() const { return GridSizeCm; }

void ABezierCurve2DActor::BEZ_SetForcePlanar_Implementation(bool bInForce) { UI_SetForcePlanar(bInForce); }
void ABezierCurve2DActor::BEZ_ResetCurveState_Implementation() { UI_ResetCurveState(); }

bool ABezierCurve2DActor::BEZ_AddControlPointAfterSelected_Implementation() { return UI_AddControlPointAfterSelected(); }
bool ABezierCurve2DActor::BEZ_DeleteSelectedControlPoint_Implementation() { return UI_DeleteSelectedControlPoint(); }
bool ABezierCurve2DActor::BEZ_DuplicateSelectedControlPoint_Implementation() { return UI_DuplicateSelectedControlPoint(); }
