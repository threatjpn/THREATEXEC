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
#include "HAL/PlatformFilemanager.h"
#include "THREATEXEC_FileUtils.h"
#include "Algo/Reverse.h"

static void SetInstanceColorRGB(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const FLinearColor& C)
{
	if (!ISM) return;
	if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return;
	if (ISM->NumCustomDataFloats < 3) ISM->NumCustomDataFloats = 3;

	ISM->SetCustomDataValue(InstanceIndex, 0, C.R, false);
	ISM->SetCustomDataValue(InstanceIndex, 1, C.G, false);
	ISM->SetCustomDataValue(InstanceIndex, 2, C.B, true);
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
	StripMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CubeStripISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CubeStrip"));
	CubeStripISM->SetupAttachment(Root);
	CubeStripISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (CubeMesh.Succeeded())
	{
		CubeStripISM->SetStaticMesh(CubeMesh.Object);
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

	if (bShowStripMesh)
	{
		UpdateStripMesh();
	}

	ApplyRuntimeEditVisibility();

	if (!GetWorld()) return;

	const FTransform Xf = GetActorTransform();
	const float PulseAlpha = bPulseDebugLines
		? FMath::Lerp(DebugPulseMinAlpha, DebugPulseMaxAlpha, (FMath::Sin(GetWorld()->GetTimeSeconds() * DebugPulseSpeed) + 1.0f) * 0.5f)
		: DebugPulseMaxAlpha;
	const uint8 DebugAlpha = static_cast<uint8>(FMath::Clamp(PulseAlpha, 0.0f, 1.0f) * 255.0f);
	const float DebugThickness = 0.5f + (PulseAlpha * 1.0f);

	if (bShowControlPolygon && Control.Num() >= 2)
	{
		for (int32 i = 0; i + 1 < Control.Num(); ++i)
		{
			DrawDebugLine(
				GetWorld(),
				Xf.TransformPosition(Control[i] * Scale),
				Xf.TransformPosition(Control[i + 1] * Scale),
				FColor(255, 255, 255, DebugAlpha), false, 0.f, 0, DebugThickness
			);
		}
	}

	if (bShowLevelsAtT && Control.Num() >= 2)
	{
		TArray<TArray<FVector>> Levels;
		TEBezier::DeCasteljauLevels<FVector>(Control, FMath::Clamp(ProofT, 0.0, 1.0), Levels);
		for (int32 L = 0; L < Levels.Num(); ++L)
		{
			FColor C = (L == Levels.Num() - 1) ? FColor::Red : FColor(128, 200, 255, 255);
			for (int32 i = 0; i + 1 < Levels[L].Num(); ++i)
			{
				DrawDebugLine(
					GetWorld(),
					Xf.TransformPosition(Levels[L][i] * Scale),
					Xf.TransformPosition(Levels[L][i + 1] * Scale),
					FColor(C.R, C.G, C.B, DebugAlpha), false, 0.f, 0, DebugThickness
				);
			}
		}
	}

	if (bShowGrid)
	{
		const float G = FMath::Max(0.1f, GridSizeCm);
		const int32 HalfCells = 10;
		const float Extent = G * HalfCells;
		for (int32 i = -HalfCells; i <= HalfCells; ++i)
		{
			const float Offset = i * G;
			const FVector A = Xf.TransformPosition(FVector(-Extent, Offset, 0.0f));
			const FVector B = Xf.TransformPosition(FVector(Extent, Offset, 0.0f));
			DrawDebugLine(GetWorld(), A, B, FColor(0, 255, 0, DebugAlpha), false, 0.f, 0, DebugThickness);

			const FVector C = Xf.TransformPosition(FVector(Offset, -Extent, 0.0f));
			const FVector D = Xf.TransformPosition(FVector(Offset, Extent, 0.0f));
			DrawDebugLine(GetWorld(), C, D, FColor(0, 255, 0, DebugAlpha), false, 0.f, 0, DebugThickness);
		}
	}
}

void ABezierCurve3DActor::ApplyRuntimeEditVisibility()
{
	// Whole actor
	SetActorHiddenInGame(!bActorVisibleInGame);

	// Visuals only show if enabled, and either in edit mode or not hiding when not editing
	const bool bCanShowVisuals = bActorVisibleInGame && bEnableRuntimeEditing && (bEditMode || !bHideVisualsWhenNotEditing);

	const bool bShowCP = bCanShowVisuals && bShowControlPoints;
	const bool bShowStrip = bCanShowVisuals && bShowStripMesh;

	if (ControlPointISM)
	{
		ControlPointISM->SetHiddenInGame(!bShowCP);
		ControlPointISM->SetVisibility(bShowCP, true);

		// Important: do NOT block mouse traces unless editing and CP are visible
		ControlPointISM->SetCollisionEnabled((bShowCP && bEditMode) ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	if (StripMeshComponent)
	{
		const bool bShowProc = bShowStrip && !bUseCubeStrip;
		StripMeshComponent->SetHiddenInGame(!bShowProc);
		StripMeshComponent->SetVisibility(bShowProc, true);
	}

	if (CubeStripISM)
	{
		const bool bShowCube = bShowStrip && bUseCubeStrip;
		CubeStripISM->SetHiddenInGame(!bShowCube);
		CubeStripISM->SetVisibility(bShowCube, true);
	}
}

void ABezierCurve3DActor::UpdateControlPointInstanceColors()
{
	if (!ControlPointISM) return;

	const int32 Count = ControlPointISM->GetInstanceCount();
	for (int32 i = 0; i < Count; ++i)
	{
		FLinearColor C = ControlPointColor;
		if (i == SelectedControlPointIndex) C = ControlPointSelectedColor;
		else if (i == HoveredControlPointIndex) C = ControlPointHoverColor;

		SetInstanceColorRGB(ControlPointISM, i, C);
	}

	ControlPointISM->MarkRenderStateDirty();
}

void ABezierCurve3DActor::RefreshControlPointVisuals()
{
	if (!ControlPointISM) return;
	if (ControlPointISM->NumCustomDataFloats < 3) ControlPointISM->NumCustomDataFloats = 3;

	if (ControlPointMaterial)
	{
		ControlPointISM->SetMaterial(0, ControlPointMaterial);
	}

	ControlPointISM->ClearInstances();

	// Build instances even if hidden, because UI toggles want to show instantly
	if (!bEnableRuntimeEditing) return;

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
}

void ABezierCurve3DActor::UpdateCubeStrip()
{
	if (!CubeStripISM) return;

	CubeStripISM->ClearInstances();
	if (StripMaterial) CubeStripISM->SetMaterial(0, StripMaterial);

	if (Control.Num() < 2) return;

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);

	const float CubeSizeCm = 100.0f; // UE cube is 100cm
	const float WidthScale = FMath::Max(0.001f, StripWidth) / CubeSizeCm;
	const float ThickScale = FMath::Max(0.001f, StripThickness) / CubeSizeCm;

	const FVector Up = GetActorUpVector();

	for (int32 i = 0; i < Segs; ++i)
	{
		const double t0 = (double)i / (double)Segs;
		const double t1 = (double)(i + 1) / (double)Segs;

		const FVector P0 = Eval(t0) * Scale;
		const FVector P1 = Eval(t1) * Scale;

		const FVector Dir = (P1 - P0);
		const float Len = Dir.Size();
		if (Len < KINDA_SMALL_NUMBER) continue;

		const FVector Mid = (P0 + P1) * 0.5f;
		const FVector X = Dir / Len;

		FTransform Xf;
		Xf.SetLocation(Mid);

		// Keep a stable frame: X forward, use Up as reference
		const FVector Side = FVector::CrossProduct(Up, X).GetSafeNormal();
		const FVector TrueUp = FVector::CrossProduct(X, Side).GetSafeNormal();
		Xf.SetRotation(FRotationMatrix::MakeFromXZ(X, TrueUp).ToQuat());

		Xf.SetScale3D(FVector(Len / CubeSizeCm, WidthScale, ThickScale));
		CubeStripISM->AddInstance(Xf);
	}
}

void ABezierCurve3DActor::UpdateStripMesh()
{
	if (!bShowStripMesh) return;

	if (bUseCubeStrip)
	{
		UpdateCubeStrip();
		return;
	}

	// Procedural ribbon (your original)
	if (!StripMeshComponent || Control.Num() < 2) return;

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector2D> UVs;

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);
	FVector Up = GetActorUpVector();

	for (int32 i = 0; i <= Segs; ++i)
	{
		double t = (double)i / (double)Segs;
		FVector P = Eval(t) * Scale;

		FVector Tangent = (Eval(FMath::Min(t + 0.001, 1.0)) * Scale - P).GetSafeNormal();
		FVector Side = FVector::CrossProduct(Tangent, Up).GetSafeNormal();

		Verts.Add(P + (Side * StripWidth * 0.5f));
		Verts.Add(P - (Side * StripWidth * 0.5f));

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

void ABezierCurve3DActor::UI_SetControlPointSize(float InVisualScale)
{
	ControlPointVisualScale = FMath::Max(0.001f, InVisualScale);
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
	TEBezier::SampleUniform(Control, SampleCount, Samples);

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
	TEBezier::DeCasteljauLevels(Control, 0.5, Levels);

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

void ABezierCurve3DActor::UI_MirrorCurveX() { for (FVector& P : Control) P.X = -P.X; WriteControlToSpline(); RefreshControlPointVisuals(); UpdateStripMesh(); }
void ABezierCurve3DActor::UI_MirrorCurveY() { for (FVector& P : Control) P.Y = -P.Y; WriteControlToSpline(); RefreshControlPointVisuals(); UpdateStripMesh(); }
void ABezierCurve3DActor::UI_MirrorCurveZ() { for (FVector& P : Control) P.Z = -P.Z; WriteControlToSpline(); RefreshControlPointVisuals(); UpdateStripMesh(); }

void ABezierCurve3DActor::UI_ToggleClosedLoop()
{
	if (Spline)
	{
		Spline->SetClosedLoop(!Spline->IsClosedLoop());
		Spline->UpdateSpline();
	}
}

void ABezierCurve3DActor::UI_SetClosedLoop(bool bInClosed)
{
	if (Spline)
	{
		Spline->SetClosedLoop(bInClosed);
		Spline->UpdateSpline();
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
	bForcePlanar = bInForce;
	if (bInForce)
	{
		for (auto& P : Control) P.Z = 0;
		WriteControlToSpline();
		RefreshControlPointVisuals();
		UpdateStripMesh();
	}
}

void ABezierCurve3DActor::UI_SetSnapToGrid(bool bInSnap)
{
	bSnapToGrid = bInSnap;
	if (bInSnap)
	{
		bShowGrid = true;
	}
}

void ABezierCurve3DActor::UI_SetShowGrid(bool bInShow)
{
	bShowGrid = bInShow;
}

void ABezierCurve3DActor::UI_SetGridSizeCm(float InGridSizeCm)
{
	GridSizeCm = FMath::Max(0.1f, InGridSizeCm);
}

void ABezierCurve3DActor::UI_SetLockToLocalXY(bool bInLock)
{
	bLockToLocalXY = bInLock;
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

bool ABezierCurve3DActor::UI_DeleteSelectedControlPoint() { return UI_DeleteControlPoint(SelectedControlPointIndex); }

bool ABezierCurve3DActor::UI_DuplicateControlPoint(int32 Index)
{
	if (!Control.IsValidIndex(Index)) return false;
	Control.Insert(Control[Index], Index + 1);
	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}
bool ABezierCurve3DActor::UI_DuplicateSelectedControlPoint() { return UI_DuplicateControlPoint(SelectedControlPointIndex); }

bool ABezierCurve3DActor::UI_SelectFromHit(const FHitResult& Hit)
{
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (Hit.Component != ControlPointISM || Hit.Item == INDEX_NONE) return false;
	SelectedControlPointIndex = Hit.Item;
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

	if (bSnapToGrid)
	{
		const float G = FMath::Max(0.1f, GridSizeCm);
		W.X = FMath::GridSnap(W.X, G);
		W.Y = FMath::GridSnap(W.Y, G);
		W.Z = FMath::GridSnap(W.Z, G);
	}

	Control[Index] = GetActorTransform().InverseTransformPosition(W) / Scale;
	if (bLockToLocalXY || bForcePlanar) Control[Index].Z = 0;

	WriteControlToSpline();
	RefreshControlPointVisuals();
	UpdateStripMesh();
	return true;
}

void ABezierCurve3DActor::UI_ClearSelectedControlPoint()
{
	SelectedControlPointIndex = -1;
	UpdateControlPointInstanceColors();
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
