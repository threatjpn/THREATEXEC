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
				Xf.TransformPosition(FVector(Control[i].X * Scale, Control[i].Y * Scale, 0)),
				Xf.TransformPosition(FVector(Control[i + 1].X * Scale, Control[i + 1].Y * Scale, 0)),
				FColor(255, 255, 255, DebugAlpha), false, 0.f, 0, DebugThickness
			);
		}
	}

	if (bShowGrid)
	{
		const float G = FMath::Max(0.1f, GridSizeCm);
		const int32 HalfCells = 10;
		const float Extent = G * HalfCells;
		const FTransform Xf = GetActorTransform();
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

void ABezierCurve2DActor::ApplyRuntimeEditVisibility()
{
	SetActorHiddenInGame(!bActorVisibleInGame);

	const bool bCanShowVisuals = bActorVisibleInGame && bEnableRuntimeEditing && (bEditMode || !bHideVisualsWhenNotEditing);

	const bool bShowCP = bCanShowVisuals && bShowControlPoints;
	const bool bShowStrip = bCanShowVisuals && bShowStripMesh;

	if (ControlPointISM)
	{
		ControlPointISM->SetHiddenInGame(!bShowCP);
		ControlPointISM->SetVisibility(bShowCP, true);
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

void ABezierCurve2DActor::UpdateControlPointInstanceColors()
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

void ABezierCurve2DActor::RefreshControlPointVisuals()
{
	if (!ControlPointISM) return;
	if (ControlPointISM->NumCustomDataFloats < 3) ControlPointISM->NumCustomDataFloats = 3;

	if (ControlPointMaterial)
	{
		ControlPointISM->SetMaterial(0, ControlPointMaterial);
	}

	ControlPointISM->ClearInstances();
	if (!bEnableRuntimeEditing) return;

	for (const FVector2D& P : Control)
	{
		FTransform Xf;
		Xf.SetLocation(FVector(P.X * Scale, P.Y * Scale, 0.0f));
		Xf.SetRotation(FQuat::Identity);
		Xf.SetScale3D(FVector(ControlPointVisualScale));
		ControlPointISM->AddInstance(Xf);
	}

	UpdateControlPointInstanceColors();
}

void ABezierCurve2DActor::UpdateCubeStrip()
{
	if (!CubeStripISM) return;

	CubeStripISM->ClearInstances();
	if (StripMaterial) CubeStripISM->SetMaterial(0, StripMaterial);

	if (Control.Num() < 2) return;

	const int32 Segs = FMath::Clamp(StripSegments, 2, 2048);

	const float CubeSizeCm = 100.0f;
	const float WidthScale = FMath::Max(0.001f, StripWidth) / CubeSizeCm;
	const float ThickScale = FMath::Max(0.001f, StripThickness) / CubeSizeCm;

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
	const FVector SideAxis = FVector(0, 0, 1);

	for (int32 i = 0; i <= Segs; ++i)
	{
		double t = (double)i / (double)Segs;
		FVector2D P2 = Eval(t);
		FVector P3 = FVector(P2.X * Scale, P2.Y * Scale, 0);

		FVector2D PNext2 = Eval(FMath::Min(t + 0.001, 1.0));
		FVector Tangent = FVector(PNext2.X * Scale - P3.X, PNext2.Y * Scale - P3.Y, 0).GetSafeNormal();
		FVector Side = FVector::CrossProduct(Tangent, SideAxis).GetSafeNormal();

		Verts.Add(P3 + (Side * StripWidth * 0.5f));
		Verts.Add(P3 - (Side * StripWidth * 0.5f));

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

void ABezierCurve2DActor::UI_SetSnapToGrid(bool bInSnap)
{
	bSnapToGrid = bInSnap;
	if (bInSnap)
	{
		bShowGrid = true;
	}
}

void ABezierCurve2DActor::UI_SetControlPointSize(float InVisualScale)
{
	ControlPointVisualScale = FMath::Max(0.001f, InVisualScale);
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
	TEBezier::SampleUniform(Control, SampleCount, Samples);

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
	if (!bEnableRuntimeEditing || !bEditMode) return false;
	if (Hit.Component != ControlPointISM || Hit.Item == INDEX_NONE) return false;
	SelectedControlPointIndex = Hit.Item;
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
	return UI_DeleteControlPoint(SelectedControlPointIndex);
}

bool ABezierCurve2DActor::UI_DuplicateControlPoint(int32 Index)
{
	if (!Control.IsValidIndex(Index)) return false;
	Control.Insert(Control[Index], Index + 1);
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

	// 2D stays planar
	if (bForcePlanar) W.Z = 0.0f;

	if (bSnapToGrid)
	{
		const float G = FMath::Max(0.1f, GridSizeCm);
		W.X = FMath::GridSnap(W.X, G);
		W.Y = FMath::GridSnap(W.Y, G);
		W.Z = 0.0f;
	}

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
	UpdateControlPointInstanceColors();
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
