// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraBase.h"

//Unreal includes
#include "Engine/Selection.h"
#include "CineCameraComponent.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Algo/Sort.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(OrbitCamera);

AOrbitCameraBase::AOrbitCameraBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	//OrbitRoot
	OrbitRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OrbitRoot"));
	OrbitRoot->SetupAttachment(RootComponent);
	OrbitRoot->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));

	//CineCamera
	CineCamRef = GetCineCameraComponent();
	CineCamRef->SetupAttachment(OrbitRoot);
	CineCamRef->SetRelativeLocation(FVector(InitialDistance * -1, 0.f, 0.f));

	CinematicDOF = FOrbitCameraDOFSettings{ 1.8f, 50.0f, 0.0f, 8.0f, true };
	GameplayDOF = FOrbitCameraDOFSettings{ 4.0f, 35.0f, 0.0f, 10.0f, true };
	PortraitDOF = FOrbitCameraDOFSettings{ 1.4f, 85.0f, 5.0f, 6.0f, true };
	MacroDOF = FOrbitCameraDOFSettings{ 2.0f, 100.0f, -2.0f, 5.0f, true };
	CustomDOF = CinematicDOF;
}

void AOrbitCameraBase::BeginPlay()
{
	Super::BeginPlay();
	ClampOrbitRootToBounds();
}

void AOrbitCameraBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClampOrbitRootToBounds();
	UpdateAutoFocus(DeltaSeconds);
	UpdateDepthOfField(DeltaSeconds);
}

void AOrbitCameraBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlacementFromSettings();
}

float AOrbitCameraBase::ResolvePreviewValue(EEditorPositionPreview PreviewMode, float MinValue, float InitialValue, float MaxValue) const
{
	switch (PreviewMode)
	{
	case EEditorPositionPreview::OC_Min:
		return MinValue;
	case EEditorPositionPreview::OC_Max:
		return MaxValue;
	case EEditorPositionPreview::OC_Initial:
	default:
		return InitialValue;
	}
}

void AOrbitCameraBase::ApplyPlacementFromSettings()
{
	if (!OrbitRoot || !CineCamRef)
	{
		return;
	}

	const float YawMin = FMath::Min(MinYaw, MaxYaw);
	const float YawMax = FMath::Max(MinYaw, MaxYaw);
	const float PitchMin = FMath::Min(MinPitch, MaxPitch);
	const float PitchMax = FMath::Max(MinPitch, MaxPitch);
	const float DistMin = FMath::Min(MinDistance, MaxDistance);
	const float DistMax = FMath::Max(MinDistance, MaxDistance);
	const float FocalMin = FMath::Min(MinFocalLength, MaxFocalLength);
	const float FocalMax = FMath::Max(MinFocalLength, MaxFocalLength);

	const float PlacementYaw = FMath::Clamp(ResolvePreviewValue(YawPreview, MinYaw, InitialYaw, MaxYaw), YawMin, YawMax);
	const float PlacementPitch = FMath::Clamp(ResolvePreviewValue(PitchPreview, MinPitch, InitialPitch, MaxPitch), PitchMin, PitchMax);
	const float PlacementDistance = FMath::Clamp(ResolvePreviewValue(ZoomPreview, MinDistance, InitialDistance, MaxDistance), DistMin, DistMax);
	const float PlacementFocalLength = FMath::Clamp(ResolvePreviewValue(ZoomPreview, MaxFocalLength, InitialFocalLength, MinFocalLength), FocalMin, FocalMax);

	OrbitRoot->SetRelativeRotation(FRotator(PlacementPitch, PlacementYaw, InitialRoll));
	CineCamRef->SetRelativeLocation(FVector(-PlacementDistance, 0.0f, 0.0f));
	CineCamRef->CurrentFocalLength = PlacementFocalLength;

	Internal_TargetDistance = PlacementDistance;
	Internal_CurrentDistance = PlacementDistance;
	Internal_TargetFocalLength = PlacementFocalLength;
	Internal_CurrentFocalLength = PlacementFocalLength;
	Internal_TargetRotation = OrbitRoot->GetRelativeRotation();
	Internal_CurrentRotation = Internal_TargetRotation;
}

void AOrbitCameraBase::CaptureCurrentPlacementAsInitial()
{
	if (!OrbitRoot || !CineCamRef)
	{
		return;
	}

	const FRotator RelativeRotation = OrbitRoot->GetRelativeRotation();
	InitialYaw = RelativeRotation.Yaw;
	InitialPitch = RelativeRotation.Pitch;
	InitialRoll = RelativeRotation.Roll;

	const float MeasuredDistance = FMath::Abs(CineCamRef->GetRelativeLocation().X);
	InitialDistance = FMath::Clamp(MeasuredDistance, FMath::Min(MinDistance, MaxDistance), FMath::Max(MinDistance, MaxDistance));
	InitialFocalLength = FMath::Clamp(CineCamRef->CurrentFocalLength, FMath::Min(MinFocalLength, MaxFocalLength), FMath::Max(MinFocalLength, MaxFocalLength));

	YawPreview = EEditorPositionPreview::OC_Initial;
	PitchPreview = EEditorPositionPreview::OC_Initial;
	ZoomPreview = EEditorPositionPreview::OC_Initial;

	ApplyPlacementFromSettings();
}

void AOrbitCameraBase::ClampOrbitRootToBounds()
{
	if (!bClampToBounds || !CameraBoundsActor || !OrbitRoot)
	{
		return;
	}

	const UBoxComponent* BoundsBox = CameraBoundsActor->FindComponentByClass<UBoxComponent>();
	if (!BoundsBox)
	{
		return;
	}

	const FVector Center = BoundsBox->GetComponentLocation();

	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(BoundsPadding);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	// 1) Clamp the ORBIT ROOT first
	const FVector RootCur = OrbitRoot->GetComponentLocation();

	const FVector RootClamped(
		FMath::Clamp(RootCur.X, Center.X - Extent.X, Center.X + Extent.X),
		FMath::Clamp(RootCur.Y, Center.Y - Extent.Y, Center.Y + Extent.Y),
		FMath::Clamp(RootCur.Z, Center.Z - Extent.Z, Center.Z + Extent.Z)
	);

	if (!RootCur.Equals(RootClamped, 0.01f))
	{
		OrbitRoot->SetWorldLocation(RootClamped, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 2) Then clamp the CAMERA position too (this fixes rotate clipping)
	ClampCameraToBounds();
}

void AOrbitCameraBase::ClampCameraToBounds()
{
	if (!bClampToBounds || !CameraBoundsActor || !OrbitRoot || !CineCamRef)
	{
		return;
	}

	const UBoxComponent* BoundsBox = CameraBoundsActor->FindComponentByClass<UBoxComponent>();
	if (!BoundsBox)
	{
		return;
	}

	const FVector Center = BoundsBox->GetComponentLocation();

	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(BoundsPadding);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	const FVector CamCur = CineCamRef->GetComponentLocation();

	const FVector CamClamped(
		FMath::Clamp(CamCur.X, Center.X - Extent.X, Center.X + Extent.X),
		FMath::Clamp(CamCur.Y, Center.Y - Extent.Y, Center.Y + Extent.Y),
		FMath::Clamp(CamCur.Z, Center.Z - Extent.Z, Center.Z + Extent.Z)
	);

	// If camera is outside, push the ROOT so the camera comes back inside.
	const FVector Delta = CamClamped - CamCur;

	if (!Delta.IsNearlyZero(0.01f))
	{
		OrbitRoot->AddWorldOffset(Delta, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AOrbitCameraBase::UpdateAutoFocus(float DeltaSeconds)
{
	if (!CineCamRef)
	{
		return;
	}

	FCameraFocusSettings FocusSettings = CineCamRef->FocusSettings;
	float NewTargetFocusDistance = Internal_TargetFocusDistance;

	switch (FocusBehavior)
	{
	case EOrbitCameraFocus::OC_FocusOrigin:
	{
		const FVector FocusTarget = OrbitRoot ? OrbitRoot->GetComponentLocation() : GetActorLocation();
		NewTargetFocusDistance = FVector::Distance(CineCamRef->GetComponentLocation(), FocusTarget) + FocusOffset;
		FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
		break;
	}
	case EOrbitCameraFocus::OC_AutoFocus:
	{
		const FVector Start = CineCamRef->GetComponentLocation() + CineCamRef->GetForwardVector() * AutoFocus_TraceStartOffset.X
			+ CineCamRef->GetRightVector() * AutoFocus_TraceStartOffset.Y
			+ CineCamRef->GetUpVector() * AutoFocus_TraceStartOffset.Z;
		const FVector Forward = CineCamRef->GetForwardVector();
		const FVector Right = CineCamRef->GetRightVector();
		const FVector Up = CineCamRef->GetUpVector();

		TArray<FVector> TraceStarts;
		TraceStarts.Reserve(5);
		TraceStarts.Add(Start);
		if (bUseAutoFocusMultiSample)
		{
			const float Spread = FMath::Max(0.0f, AutoFocus_MultiSampleSpread);
			TraceStarts.Add(Start + (Right * Spread));
			TraceStarts.Add(Start - (Right * Spread));
			TraceStarts.Add(Start + (Up * Spread));
			TraceStarts.Add(Start - (Up * Spread));
		}

		bool bHit = false;
		TArray<float> HitDistances;
		HitDistances.Reserve(TraceStarts.Num());
		if (FocusOnObjectTypes.Num() > 0)
		{
			for (const FVector& TraceStart : TraceStarts)
			{
				FHitResult HitResult;
				const FVector End = TraceStart + Forward * MaxAutoFocusDistance;
				const bool bThisHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
					this,
					TraceStart,
					End,
					AutoFocus_TraceRadius,
					FocusOnObjectTypes,
					bUseComplexCollisions,
					TArray<AActor*>(),
					EnableDebugging ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
					HitResult,
					true);

				if (bThisHit)
				{
					bHit = true;
					HitDistances.Add(HitResult.Distance);
				}
			}
		}

		if (bHit && HitDistances.Num() > 0)
		{
			Algo::Sort(HitDistances);
			const float MedianHitDistance = HitDistances[HitDistances.Num() / 2];
			const float CandidateFocusDistance = FMath::Clamp(MedianHitDistance + FocusOffset, MinAutoFocusDistance, MaxAutoFocusDistance);
			const float PreviousTargetDistance = FMath::Clamp(Internal_TargetFocusDistance, MinAutoFocusDistance, MaxAutoFocusDistance);

			Internal_AutoFocusLostTime = 0.0f;
			Internal_LastValidFocusDistance = CandidateFocusDistance;

			if (FMath::Abs(CandidateFocusDistance - PreviousTargetDistance) <= AutoFocus_DeadZone)
			{
				NewTargetFocusDistance = PreviousTargetDistance;
			}
			else
			{
				NewTargetFocusDistance = CandidateFocusDistance;
			}
		}
		else
		{
			Internal_AutoFocusLostTime += DeltaSeconds;
			const bool bCanUseHeldFocus = Internal_LastValidFocusDistance > 0.0f && Internal_AutoFocusLostTime <= AutoFocus_LostTargetHoldTime;
			if (bCanUseHeldFocus)
			{
				NewTargetFocusDistance = FMath::Clamp(Internal_LastValidFocusDistance, MinAutoFocusDistance, MaxAutoFocusDistance);
			}
			else if (bAutoFocusFallbackToDistance)
			{
				NewTargetFocusDistance = FMath::Clamp(Internal_CurrentDistance + FocusOffset, MinAutoFocusDistance, MaxAutoFocusDistance);
			}
		}

		FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
		break;
	}
	case EOrbitCameraFocus::Off:
	default:
		FocusSettings.FocusMethod = ECameraFocusMethod::DoNotOverride;
		CineCamRef->FocusSettings = FocusSettings;
		return;
	}

	Internal_TargetFocusDistance = FMath::Clamp(NewTargetFocusDistance, MinAutoFocusDistance, MaxAutoFocusDistance);
	const float FocusDelta = FMath::Abs(Internal_TargetFocusDistance - FocusSettings.ManualFocusDistance);
	float FocusInterpSpeed = AutoFocus_InterpolationSpeed;
	if (bUseAdaptiveAutoFocusSpeed)
	{
		const float AdaptiveAlpha = FMath::Clamp(FocusDelta / 100.0f, 0.0f, 1.0f);
		FocusInterpSpeed = FMath::Lerp(
			FMath::Min(AutoFocus_MinInterpolationSpeed, AutoFocus_MaxInterpolationSpeed),
			FMath::Max(AutoFocus_MinInterpolationSpeed, AutoFocus_MaxInterpolationSpeed),
			AdaptiveAlpha);
	}

	FocusSettings.ManualFocusDistance = FMath::FInterpTo(
		FocusSettings.ManualFocusDistance,
		Internal_TargetFocusDistance,
		DeltaSeconds,
		FocusInterpSpeed);
	CineCamRef->FocusSettings = FocusSettings;
}

const FOrbitCameraDOFSettings& AOrbitCameraBase::ResolveDOFSettings() const
{
	switch (DOFPreset)
	{
	case EOrbitCameraDOFPreset::OC_DOF_Gameplay:
		return GameplayDOF;
	case EOrbitCameraDOFPreset::OC_DOF_Portrait:
		return PortraitDOF;
	case EOrbitCameraDOFPreset::OC_DOF_Macro:
		return MacroDOF;
	case EOrbitCameraDOFPreset::OC_DOF_Custom:
		return CustomDOF;
	case EOrbitCameraDOFPreset::OC_DOF_Off:
		return GameplayDOF;
	case EOrbitCameraDOFPreset::OC_DOF_Cinematic:
	default:
		return CinematicDOF;
	}
}

void AOrbitCameraBase::UpdateDepthOfField(float DeltaSeconds)
{
	if (!CineCamRef || !bEnableAdvancedDOF)
	{
		return;
	}

	if (DOFPreset == EOrbitCameraDOFPreset::OC_DOF_Off)
	{
		CineCamRef->CurrentAperture = FMath::FInterpTo(CineCamRef->CurrentAperture, 22.0f, DeltaSeconds, DOFTransitionSpeed);
		return;
	}

	const FOrbitCameraDOFSettings& Settings = ResolveDOFSettings();

	const float TargetAperture = FMath::Max(0.1f, Settings.Aperture);
	const float TargetFocalLength = FMath::Max(1.0f, Settings.FocalLength);

	if (bSmoothDOFTransitions)
	{
		CineCamRef->CurrentAperture = FMath::FInterpTo(CineCamRef->CurrentAperture, TargetAperture, DeltaSeconds, DOFTransitionSpeed);
		CineCamRef->CurrentFocalLength = FMath::FInterpTo(CineCamRef->CurrentFocalLength, TargetFocalLength, DeltaSeconds, DOFTransitionSpeed);
	}
	else
	{
		CineCamRef->CurrentAperture = TargetAperture;
		CineCamRef->CurrentFocalLength = TargetFocalLength;
	}

	// Keep manual focus path enabled for cinematic consistency.
	FCameraFocusSettings FocusSettings = CineCamRef->FocusSettings;
	FocusSettings.FocusMethod = ECameraFocusMethod::Manual;

	float TargetFocus = FocusSettings.ManualFocusDistance;
	if (Settings.bUseAutoFocus)
	{
		TargetFocus = Internal_TargetFocusDistance + Settings.FocusOffset;
	}
	else
	{
		TargetFocus = Internal_CurrentDistance + Settings.FocusOffset;
	}

	FocusSettings.ManualFocusDistance = FMath::FInterpTo(
		FocusSettings.ManualFocusDistance,
		FMath::Max(0.0f, TargetFocus),
		DeltaSeconds,
		FMath::Max(0.0f, Settings.FocusSmoothing));
	CineCamRef->FocusSettings = FocusSettings;
}

bool AOrbitCameraBase::IsInEditorMode(UObject* WorldContextObject)
{
#ifdef WITH_EDITOR
	//check if there is no valid game instance
	if (!WorldContextObject->GetWorld()->GetGameInstance())
	{
		return true;
	}
#endif
	return false;
}

#if WITH_EDITOR

void AOrbitCameraBase::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	//Bind To SelectionChangedEvent if delegate is not yet bound
	if (!OnSelectionChangedDelegateHandle.IsValid())
	{
		UE_LOG(OrbitCamera, Log, TEXT("AddUObject to SlectionChangedEvent"));
		OnSelectionChangedDelegateHandle = USelection::SelectionChangedEvent.AddUObject(this, &AOrbitCameraBase::OnSelectionChanged);
	}
}

void AOrbitCameraBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyPlacementFromSettings();
}

void AOrbitCameraBase::BeginDestroy()
{
	// If Delegate is bound remove it
	if (OnSelectionChangedDelegateHandle.IsValid())
	{
		UE_LOG(OrbitCamera, Log, TEXT("Removed from SlectionChangedEvent"));
		USelection::SelectionChangedEvent.Remove(OnSelectionChangedDelegateHandle);
		OnSelectionChangedDelegateHandle.Reset();
	}

	Super::BeginDestroy();
}

void AOrbitCameraBase::OnSelectionChanged(UObject* InObject)
{
	// Check if Actor is selected and store selection
	if (IsSelected())
	{
		bWasSelected = true;
	}
	// If the actor is not selected but was selected before
	else if (bWasSelected)
	{
		bWasSelected = false;

		//Reset Preview Values and call Construction again
		YawPreview = EEditorPositionPreview::OC_Initial;
		PitchPreview = EEditorPositionPreview::OC_Initial;
		ZoomPreview = EEditorPositionPreview::OC_Initial;
		RerunConstructionScripts();
	}
}

#endif
