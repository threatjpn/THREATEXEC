// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraBase.h"

//Unreal includes
#include "Engine/Selection.h"
#include "CineCameraComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

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
}

void AOrbitCameraBase::BeginPlay()
{
	Super::BeginPlay();
	ValidateAndNormalizeSettings();
	ApplyComfortProfile(ComfortProfile);
	ApplyDOFPreset(DOFPreset);
	InitializeRuntimeStateFromDefaults();
	ClampOrbitRootToBounds();
}

void AOrbitCameraBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!OrbitRoot || !CineCamRef)
	{
		return;
	}

	ValidateAndNormalizeSettings();
	ApplyDOFPreset(DOFPreset);

	if (bUseActorTransformForInitialState)
	{
		const FVector ActorLocation = GetActorLocation();
		const FRotator ActorRotation = GetActorRotation();

		OrbitRoot->SetWorldLocation(ActorLocation);
		OrbitRoot->SetWorldRotation(ActorRotation);

		InitialYaw = FMath::Clamp(ActorRotation.Yaw, MinYaw, MaxYaw);
		InitialPitch = FMath::Clamp(ActorRotation.Pitch, MinPitch, MaxPitch);
		InitialRoll = ActorRotation.Roll;
	}

	CineCamRef->SetRelativeLocation(FVector(-InitialDistance, 0.0f, 0.0f));
	CineCamRef->CurrentFocalLength = InitialFocalLength;
	CineCamRef->CurrentAperture = TargetAperture;
}

void AOrbitCameraBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateRuntimeState(DeltaSeconds);
	ClampOrbitRootToBounds();
}

void AOrbitCameraBase::ValidateAndNormalizeSettings()
{
	if (MinYaw > MaxYaw)
	{
		Swap(MinYaw, MaxYaw);
	}
	if (MinPitch > MaxPitch)
	{
		Swap(MinPitch, MaxPitch);
	}
	if (MinDistance > MaxDistance)
	{
		Swap(MinDistance, MaxDistance);
	}
	if (MinFocalLength > MaxFocalLength)
	{
		Swap(MinFocalLength, MaxFocalLength);
	}
	if (MinAutoFocusDistance > MaxAutoFocusDistance)
	{
		Swap(MinAutoFocusDistance, MaxAutoFocusDistance);
	}

	InitialYaw = FMath::Clamp(InitialYaw, MinYaw, MaxYaw);
	InitialPitch = FMath::Clamp(InitialPitch, MinPitch, MaxPitch);
	InitialDistance = FMath::Clamp(InitialDistance, MinDistance, MaxDistance);
	InitialFocalLength = FMath::Clamp(InitialFocalLength, MinFocalLength, MaxFocalLength);

	Internal_LocationInterpolationSpeed = FMath::Max(0.0f, Internal_LocationInterpolationSpeed);
	Internal_RotationInterpolationSpeed = FMath::Max(0.0f, Internal_RotationInterpolationSpeed);
	Internal_DistanceInterpolationSpeed = FMath::Max(0.0f, Internal_DistanceInterpolationSpeed);
	Internal_ZoomInterpolationSpeed = FMath::Max(0.0f, Internal_ZoomInterpolationSpeed);

	FocusDeadzone = FMath::Max(0.0f, FocusDeadzone);
	FocusMaxStepPerSecond = FMath::Max(0.0f, FocusMaxStepPerSecond);
	FocusPredictionLeadSeconds = FMath::Max(0.0f, FocusPredictionLeadSeconds);
	MinAperture = FMath::Clamp(MinAperture, 0.7f, 32.0f);
	MaxAperture = FMath::Clamp(MaxAperture, 0.7f, 32.0f);
	if (MinAperture > MaxAperture)
	{
		Swap(MinAperture, MaxAperture);
	}
	TargetAperture = FMath::Clamp(TargetAperture, MinAperture, MaxAperture);
	AutoApertureNearDistance = FMath::Max(0.0f, AutoApertureNearDistance);
	AutoApertureFarDistance = FMath::Max(AutoApertureNearDistance + 1.0f, AutoApertureFarDistance);
	ApertureInterpolationSpeed = FMath::Max(0.0f, ApertureInterpolationSpeed);
	CineFocusSmoothingInterpSpeed = FMath::Max(0.0f, CineFocusSmoothingInterpSpeed);
}

void AOrbitCameraBase::InitializeRuntimeStateFromDefaults()
{
	if (!OrbitRoot || !CineCamRef)
	{
		return;
	}

	Internal_InitalLocation = OrbitRoot->GetComponentLocation();
	if (bUseActorTransformForInitialState)
	{
		Internal_InitalLocation = GetActorLocation();
		OrbitRoot->SetWorldLocation(Internal_InitalLocation);
	}
	Internal_TargetLocation = Internal_InitalLocation;
	Internal_CurrentLocation = Internal_InitalLocation;

	Internal_TargetRotation = FRotator(InitialPitch, InitialYaw, InitialRoll);
	if (bUseActorTransformForInitialState)
	{
		const FRotator ActorRot = GetActorRotation();
		Internal_TargetRotation = FRotator(
			FMath::Clamp(ActorRot.Pitch, MinPitch, MaxPitch),
			FMath::Clamp(ActorRot.Yaw, MinYaw, MaxYaw),
			ActorRot.Roll);
	}
	Internal_CurrentRotation = Internal_TargetRotation;
	OrbitRoot->SetWorldRotation(Internal_CurrentRotation);

	Internal_TargetDistance = InitialDistance;
	Internal_CurrentDistance = InitialDistance;
	CineCamRef->SetRelativeLocation(FVector(-Internal_CurrentDistance, 0.0f, 0.0f));

	Internal_TargetFocalLength = InitialFocalLength;
	Internal_CurrentFocalLength = InitialFocalLength;
	CineCamRef->CurrentFocalLength = Internal_CurrentFocalLength;

	Internal_TargetFocusDistance = InitialDistance;
	LastStableAutoFocusDistance = Internal_TargetFocusDistance;
	CineCamRef->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
	CineCamRef->FocusSettings.ManualFocusDistance = Internal_TargetFocusDistance;
	CineCamRef->CurrentAperture = TargetAperture;
}

void AOrbitCameraBase::UpdateRuntimeState(float DeltaSeconds)
{
	if (!OrbitRoot || !CineCamRef)
	{
		return;
	}

	if (bTransitionInProgress)
	{
		TransitionElapsed += DeltaSeconds;
		const float RawAlpha = (TransitionParams.Duration <= KINDA_SMALL_NUMBER) ? 1.0f : FMath::Clamp(TransitionElapsed / TransitionParams.Duration, 0.0f, 1.0f);
		const float Alpha = EvaluateTransitionAlpha(RawAlpha, TransitionParams);

		Internal_TargetLocation = FMath::Lerp(TransitionStart.Position, TransitionTarget.Position, Alpha);
		Internal_TargetRotation = FMath::Lerp(TransitionStart.Rotation, TransitionTarget.Rotation, Alpha);
		Internal_TargetDistance = FMath::Lerp(TransitionStart.Distance, TransitionTarget.Distance, Alpha);
		Internal_TargetFocalLength = FMath::Lerp(TransitionStart.Zoom, TransitionTarget.Zoom, Alpha);
		CineCamRef->SetFieldOfView(FMath::Lerp(TransitionStart.FoV, TransitionTarget.FoV, Alpha));

		if (TransitionParams.bBlendFocusDistance)
		{
			Internal_TargetFocusDistance = FMath::Lerp(TransitionStart.Distance, TransitionTarget.Distance, Alpha);
		}

		if (RawAlpha >= 1.0f)
		{
			bTransitionInProgress = false;
		}
	}

	// velocity-based damping + jerk-limited angular response
	const float DesiredYawVelocity = FMath::Clamp(PendingYawInput * InputTuning.OrbitSensitivityYaw, -InputTuning.MaxAngularVelocity, InputTuning.MaxAngularVelocity);
	const float DesiredPitchVelocity = FMath::Clamp(PendingPitchInput * InputTuning.OrbitSensitivityPitch, -InputTuning.MaxAngularVelocity, InputTuning.MaxAngularVelocity);
	const float MaxVelDelta = InputTuning.AngularAcceleration * DeltaSeconds;
	Internal_YawVelocity = FMath::FInterpTo(Internal_YawVelocity, DesiredYawVelocity, DeltaSeconds, (MaxVelDelta <= KINDA_SMALL_NUMBER) ? 0.0f : 10.0f);
	Internal_PitchVelocity = FMath::FInterpTo(Internal_PitchVelocity, DesiredPitchVelocity, DeltaSeconds, (MaxVelDelta <= KINDA_SMALL_NUMBER) ? 0.0f : 10.0f);
	Internal_YawVelocity = FMath::Clamp(Internal_YawVelocity, DesiredYawVelocity - MaxVelDelta, DesiredYawVelocity + MaxVelDelta);
	Internal_PitchVelocity = FMath::Clamp(Internal_PitchVelocity, DesiredPitchVelocity - MaxVelDelta, DesiredPitchVelocity + MaxVelDelta);
	PendingYawInput = 0.0f;
	PendingPitchInput = 0.0f;

	Internal_TargetRotation.Yaw = FMath::Clamp(Internal_TargetRotation.Yaw + (Internal_YawVelocity * DeltaSeconds), MinYaw, MaxYaw);
	Internal_TargetRotation.Pitch = FMath::Clamp(Internal_TargetRotation.Pitch + (Internal_PitchVelocity * DeltaSeconds), MinPitch, MaxPitch);

	const FVector TargetWithLookAhead = Internal_TargetLocation + Internal_LookAheadOffset;
	Internal_CurrentLocation = SmoothDampVector(
		Internal_CurrentLocation,
		TargetWithLookAhead,
		Internal_LocationVelocity,
		1.0f / FMath::Max(1.0f, Internal_LocationInterpolationSpeed),
		DeltaSeconds);
	OrbitRoot->SetWorldLocation(Internal_CurrentLocation);

	Internal_CurrentRotation = FMath::RInterpTo(Internal_CurrentRotation, Internal_TargetRotation, DeltaSeconds, Internal_RotationInterpolationSpeed);
	OrbitRoot->SetWorldRotation(Internal_CurrentRotation);

	Internal_CurrentDistance = SmoothDampFloat(
		Internal_CurrentDistance,
		Internal_TargetDistance,
		Internal_DistanceVelocity,
		1.0f / FMath::Max(1.0f, Internal_DistanceInterpolationSpeed),
		DeltaSeconds);
	Internal_CurrentFocalLength = SmoothDampFloat(
		Internal_CurrentFocalLength,
		Internal_TargetFocalLength,
		Internal_FocalVelocity,
		1.0f / FMath::Max(1.0f, Internal_ZoomInterpolationSpeed),
		DeltaSeconds);

	UpdateCollisionSoftSolve(DeltaSeconds);

	CineCamRef->SetRelativeLocation(FVector(-Internal_CurrentDistance, 0.0f, 0.0f));
	CineCamRef->CurrentFocalLength = Internal_CurrentFocalLength;

	const FVector DesiredLookAhead = OrbitRoot->GetForwardVector() * FMath::Clamp(
		Internal_LocationVelocity.Size() * InputTuning.LookAheadStrength,
		0.0f,
		InputTuning.LookAheadMaxDistance);
	Internal_LookAheadOffset = FMath::VInterpTo(Internal_LookAheadOffset, DesiredLookAhead, DeltaSeconds, 4.0f);

	UpdateFocus(DeltaSeconds);
}

void AOrbitCameraBase::UpdateFocus(float DeltaSeconds)
{
	if (!CineCamRef || FocusBehavior == EOrbitCameraFocus::Off)
	{
		return;
	}

	const FVector CameraLocation = CineCamRef->GetComponentLocation();
	float DesiredFocusDistance = Internal_TargetFocusDistance;
	const float PredictedTravelOffset = Internal_LocationVelocity.Size() * FocusPredictionLeadSeconds;

	switch (FocusBehavior)
	{
	case EOrbitCameraFocus::OC_FocusOrigin:
		DesiredFocusDistance = FVector::Distance(CameraLocation, OrbitRoot->GetComponentLocation()) + PredictedTravelOffset;
		break;
	case EOrbitCameraFocus::OC_AutoFocus:
		DesiredFocusDistance = ComputeAutoFocusDistance(FVector::Distance(CameraLocation, OrbitRoot->GetComponentLocation()) + PredictedTravelOffset);
		break;
	case EOrbitCameraFocus::OC_TargetActor:
		if (IsValid(FocusTargetActor))
		{
			DesiredFocusDistance = FVector::Distance(CameraLocation, FocusTargetActor->GetActorLocation()) + PredictedTravelOffset;
		}
		break;
	default:
		break;
	}

	DesiredFocusDistance = FMath::Clamp(DesiredFocusDistance + FocusOffset, MinAutoFocusDistance, MaxAutoFocusDistance);
	if (FocusBehavior == EOrbitCameraFocus::OC_AutoFocus)
	{
		if (FMath::Abs(DesiredFocusDistance - LastStableAutoFocusDistance) > FocusSwitchThreshold)
		{
			FocusHoldTimer += DeltaSeconds;
			if (FocusHoldTimer < FocusTargetHoldSeconds)
			{
				DesiredFocusDistance = LastStableAutoFocusDistance;
			}
			else
			{
				LastStableAutoFocusDistance = DesiredFocusDistance;
				FocusHoldTimer = 0.0f;
			}
		}
		else
		{
			LastStableAutoFocusDistance = DesiredFocusDistance;
			FocusHoldTimer = 0.0f;
		}
	}

	const float Delta = DesiredFocusDistance - Internal_TargetFocusDistance;
	if (FMath::Abs(Delta) <= FocusDeadzone)
	{
		DesiredFocusDistance = Internal_TargetFocusDistance;
	}

	const float Speed = (Delta < 0.0f) ? AutoFocus_InterpolationSpeedIn : AutoFocus_InterpolationSpeedOut;
	Internal_TargetFocusDistance = SmoothDampFloat(
		Internal_TargetFocusDistance,
		DesiredFocusDistance,
		Internal_FocusVelocity,
		1.0f / FMath::Max(1.0f, Speed),
		DeltaSeconds);

	if (FocusMaxStepPerSecond > 0.0f)
	{
		const float MaxStep = FocusMaxStepPerSecond * DeltaSeconds;
		Internal_TargetFocusDistance = FMath::Clamp(
			Internal_TargetFocusDistance,
			CineCamRef->FocusSettings.ManualFocusDistance - MaxStep,
			CineCamRef->FocusSettings.ManualFocusDistance + MaxStep);
	}

	CineCamRef->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
	CineCamRef->FocusSettings.ManualFocusDistance = Internal_TargetFocusDistance;
	UpdateDepthOfField(DeltaSeconds, Internal_TargetFocusDistance);

	if (bDrawDebugFocus)
	{
		const FVector End = CameraLocation + (CineCamRef->GetForwardVector() * Internal_TargetFocusDistance);
		DrawDebugLine(GetWorld(), CameraLocation, End, FColor::Cyan, false, -1.0f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), End, 8.0f, 8, FColor::Yellow, false, -1.0f);
	}
}

void AOrbitCameraBase::UpdateDepthOfField(float DeltaSeconds, float BaseFocusDistance)
{
	if (!CineCamRef)
	{
		return;
	}

	if (!bEnableDepthOfField)
	{
		CineCamRef->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
		return;
	}

	CineCamRef->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;

	CineCamRef->FocusSettings.bSmoothFocusChanges = bUseCineSmoothFocusChanges;
	CineCamRef->FocusSettings.FocusSmoothingInterpSpeed = CineFocusSmoothingInterpSpeed;

	const float FinalFocusDistance = FMath::Clamp(BaseFocusDistance + DOFFocusOffset, MinAutoFocusDistance, MaxAutoFocusDistance);
	CineCamRef->FocusSettings.ManualFocusDistance = FinalFocusDistance;

	float DesiredAperture = FMath::Clamp(TargetAperture, MinAperture, MaxAperture);
	if (bAutoApertureByFocusDistance)
	{
		const float Alpha = FMath::Clamp(
			(FinalFocusDistance - AutoApertureNearDistance) / FMath::Max(1.0f, AutoApertureFarDistance - AutoApertureNearDistance),
			0.0f,
			1.0f);

		// Near target -> lower f-stop (shallower DOF), far target -> higher f-stop.
		DesiredAperture = FMath::Lerp(MinAperture, MaxAperture, Alpha);
	}

	CineCamRef->CurrentAperture = FMath::FInterpTo(
		CineCamRef->CurrentAperture,
		DesiredAperture,
		DeltaSeconds,
		ApertureInterpolationSpeed);
}

float AOrbitCameraBase::ComputeAutoFocusDistance(float FallbackDistance) const
{
	if (!GetWorld() || FocusOnObjectTypes.Num() == 0 || !CineCamRef)
	{
		return FMath::Clamp(FallbackDistance, MinAutoFocusDistance, MaxAutoFocusDistance);
	}

	const FVector Start = CineCamRef->GetComponentLocation();
	const FVector End = Start + (CineCamRef->GetForwardVector() * MaxAutoFocusDistance);

	FCollisionObjectQueryParams ObjectParams;
	for (const EObjectTypeQuery QueryType : FocusOnObjectTypes)
	{
		const ECollisionChannel Channel = UEngineTypes::ConvertToCollisionChannel(QueryType);
		if (Channel != ECC_MAX)
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OrbitAutoFocus), bUseComplexCollisions, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByObjectType(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(FMath::Max(0.0f, AutoFocus_TraceRadius)),
		QueryParams);

	if (!bHit)
	{
		return FMath::Clamp(FallbackDistance, MinAutoFocusDistance, MaxAutoFocusDistance);
	}

	return FMath::Clamp(Hit.Distance, MinAutoFocusDistance, MaxAutoFocusDistance);
}

float AOrbitCameraBase::EvaluateTransitionAlpha(float RawAlpha, const FOrbitTransitionParams& Params) const
{
	const float Clamped = FMath::Clamp(RawAlpha, 0.0f, 1.0f);
	switch (Params.Easing)
	{
	case EOrbitTransitionEasing::Linear:
		return Clamped;
	case EOrbitTransitionEasing::EaseInOut:
		return FMath::InterpEaseInOut(0.0f, 1.0f, Clamped, 2.0f);
	case EOrbitTransitionEasing::CubicInOut:
		return FMath::CubicInterp(0.0f, 0.0f, 1.0f, 0.0f, Clamped);
	case EOrbitTransitionEasing::ExpoInOut:
		return FMath::InterpExpoInOut(0.0f, 1.0f, Clamped);
	case EOrbitTransitionEasing::CustomCurve:
		if (Params.CustomCurve)
		{
			return FMath::Clamp(Params.CustomCurve->GetFloatValue(Clamped), 0.0f, 1.0f);
		}
		return Clamped;
	default:
		return Clamped;
	}
}

float AOrbitCameraBase::ApplyBoundaryDamping(float Value, float MinValue, float MaxValue) const
{
	const float Clamped = FMath::Clamp(Value, MinValue, MaxValue);
	const float Overflow = Value - Clamped;
	if (FMath::IsNearlyZero(Overflow))
	{
		return Clamped;
	}
	return Clamped + (Overflow * InputTuning.BoundaryDampingStrength);
}

void AOrbitCameraBase::AddOrbitInput(float DeltaYaw, float DeltaPitch)
{
	PendingYawInput += DeltaYaw;
	PendingPitchInput += DeltaPitch;
	Internal_TargetRotation.Roll = InitialRoll;
}

void AOrbitCameraBase::AddPanInput(float Right, float Up)
{
	if (!OrbitRoot)
	{
		return;
	}

	const FVector RightVector = OrbitRoot->GetRightVector();
	const FVector UpVector = OrbitRoot->GetUpVector();
	Internal_TargetLocation += ((RightVector * Right) + (UpVector * Up)) * InputTuning.PanSensitivity;
}

void AOrbitCameraBase::AddZoomInput(float DeltaZoom)
{
	const float ZoomAmount = DeltaZoom * InputTuning.ZoomSensitivity;
	Internal_TargetDistance = FMath::Clamp(Internal_TargetDistance - ZoomAmount, MinDistance, MaxDistance);

	const float ZoomRatio = (MaxDistance > MinDistance)
		? (Internal_TargetDistance - MinDistance) / (MaxDistance - MinDistance)
		: 0.0f;

	Internal_TargetFocalLength = FMath::Lerp(MinFocalLength, MaxFocalLength, ZoomRatio);
}

void AOrbitCameraBase::AddOrbitInputScaled(float DeltaYaw, float DeltaPitch, float SensitivityScale)
{
	AddOrbitInput(DeltaYaw * SensitivityScale, DeltaPitch * SensitivityScale);
}

void AOrbitCameraBase::AddPanInputWorld(const FVector& WorldOffset)
{
	Internal_TargetLocation += WorldOffset;
}

void AOrbitCameraBase::StartTransitionToDefinition(const FOrbitCameraDefinition& Definition, const FOrbitTransitionParams& Params)
{
	TransitionStart = GetCurrentDefinition();
	TransitionTarget = Definition;
	TransitionParams = Params;
	TransitionElapsed = 0.0f;
	bTransitionInProgress = true;
}

void AOrbitCameraBase::CancelTransition(bool bSnapToTarget)
{
	if (!bTransitionInProgress)
	{
		return;
	}

	bTransitionInProgress = false;
	if (bSnapToTarget)
	{
		Internal_TargetLocation = TransitionTarget.Position;
		Internal_TargetRotation = TransitionTarget.Rotation;
		Internal_TargetDistance = TransitionTarget.Distance;
		Internal_TargetFocalLength = TransitionTarget.Zoom;
		if (CineCamRef)
		{
			CineCamRef->SetFieldOfView(TransitionTarget.FoV);
		}
	}
}

void AOrbitCameraBase::ApplyComfortProfile(EOrbitComfortProfile NewProfile)
{
	ComfortProfile = NewProfile;
	if (ComfortProfile == EOrbitComfortProfile::Custom)
	{
		return;
	}

	switch (ComfortProfile)
	{
	case EOrbitComfortProfile::Cinematic:
		DefaultTransitionParams.Duration = 0.65f;
		DefaultTransitionParams.Easing = EOrbitTransitionEasing::CubicInOut;
		Internal_LocationInterpolationSpeed = 4.0f;
		Internal_RotationInterpolationSpeed = 5.0f;
		Internal_DistanceInterpolationSpeed = 5.0f;
		InputTuning.MaxAngularVelocity = 120.0f;
		InputTuning.AngularAcceleration = 280.0f;
		break;
	case EOrbitComfortProfile::Comfort:
		DefaultTransitionParams.Duration = 0.45f;
		DefaultTransitionParams.Easing = EOrbitTransitionEasing::EaseInOut;
		Internal_LocationInterpolationSpeed = 7.0f;
		Internal_RotationInterpolationSpeed = 8.0f;
		Internal_DistanceInterpolationSpeed = 8.0f;
		InputTuning.MaxAngularVelocity = 160.0f;
		InputTuning.AngularAcceleration = 440.0f;
		break;
	case EOrbitComfortProfile::Snappy:
		DefaultTransitionParams.Duration = 0.25f;
		DefaultTransitionParams.Easing = EOrbitTransitionEasing::Linear;
		Internal_LocationInterpolationSpeed = 12.0f;
		Internal_RotationInterpolationSpeed = 14.0f;
		Internal_DistanceInterpolationSpeed = 12.0f;
		InputTuning.MaxAngularVelocity = 240.0f;
		InputTuning.AngularAcceleration = 720.0f;
		break;
	default:
		break;
	}
}

FOrbitCameraDefinition AOrbitCameraBase::GetCurrentDefinition() const
{
	FOrbitCameraDefinition Definition;
	Definition.ActorTransform = GetActorTransform();
	Definition.Position = Internal_CurrentLocation;
	Definition.Rotation = Internal_CurrentRotation;
	Definition.Distance = Internal_CurrentDistance;
	Definition.Zoom = Internal_CurrentFocalLength;
	Definition.FoV = CineCamRef ? CineCamRef->FieldOfView : 0.0f;
	return Definition;
}

void AOrbitCameraBase::SetFocusTargetActor(AActor* NewFocusTarget)
{
	FocusTargetActor = NewFocusTarget;
}

void AOrbitCameraBase::SnapToCurrentTargetState()
{
	Internal_CurrentLocation = Internal_TargetLocation;
	Internal_CurrentRotation = Internal_TargetRotation;
	Internal_CurrentDistance = Internal_TargetDistance;
	Internal_CurrentFocalLength = Internal_TargetFocalLength;
	Internal_LocationVelocity = FVector3f::ZeroVector;
	Internal_DistanceVelocity = 0.0f;
	Internal_FocalVelocity = 0.0f;

	if (OrbitRoot)
	{
		OrbitRoot->SetWorldLocation(Internal_CurrentLocation);
		OrbitRoot->SetWorldRotation(Internal_CurrentRotation);
	}

	if (CineCamRef)
	{
		CineCamRef->SetRelativeLocation(FVector(-Internal_CurrentDistance, 0.0f, 0.0f));
		CineCamRef->CurrentFocalLength = Internal_CurrentFocalLength;
		CineCamRef->CurrentAperture = TargetAperture;
	}
}

void AOrbitCameraBase::SetDepthOfFieldEnabled(bool bEnabled)
{
	bEnableDepthOfField = bEnabled;
	if (!bEnableDepthOfField && CineCamRef)
	{
		CineCamRef->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
	}
}

void AOrbitCameraBase::SetDOFPreset(EOrbitDOFPreset NewPreset)
{
	ApplyDOFPreset(NewPreset);
}

void AOrbitCameraBase::SetTargetAperture(float NewAperture)
{
	TargetAperture = FMath::Clamp(NewAperture, MinAperture, MaxAperture);
}

void AOrbitCameraBase::ApplyDOFPreset(EOrbitDOFPreset NewPreset)
{
	DOFPreset = NewPreset;
	if (DOFPreset == EOrbitDOFPreset::Custom)
	{
		return;
	}

	switch (DOFPreset)
	{
	case EOrbitDOFPreset::Realistic:
		MinAperture = 2.8f;
		MaxAperture = 11.0f;
		TargetAperture = 5.6f;
		AutoApertureNearDistance = 150.0f;
		AutoApertureFarDistance = 2600.0f;
		ApertureInterpolationSpeed = 4.0f;
		break;
	case EOrbitDOFPreset::Cinematic:
		MinAperture = 1.4f;
		MaxAperture = 8.0f;
		TargetAperture = 2.0f;
		AutoApertureNearDistance = 100.0f;
		AutoApertureFarDistance = 2200.0f;
		ApertureInterpolationSpeed = 7.0f;
		break;
	case EOrbitDOFPreset::Portrait:
		MinAperture = 1.2f;
		MaxAperture = 5.6f;
		TargetAperture = 1.8f;
		AutoApertureNearDistance = 80.0f;
		AutoApertureFarDistance = 1400.0f;
		ApertureInterpolationSpeed = 8.0f;
		break;
	case EOrbitDOFPreset::Macro:
		MinAperture = 2.8f;
		MaxAperture = 16.0f;
		TargetAperture = 8.0f;
		AutoApertureNearDistance = 40.0f;
		AutoApertureFarDistance = 700.0f;
		ApertureInterpolationSpeed = 5.0f;
		break;
	default:
		break;
	}

	ValidateAndNormalizeSettings();
}

float AOrbitCameraBase::SmoothDampFloat(float Current, float Target, float& CurrentVelocity, float SmoothTime, float DeltaSeconds, float MaxSpeed) const
{
	SmoothTime = FMath::Max(0.0001f, SmoothTime);
	const float Omega = 2.0f / SmoothTime;
	const float X = Omega * DeltaSeconds;
	const float Exp = 1.0f / (1.0f + X + (0.48f * X * X) + (0.235f * X * X * X));
	float Change = Current - Target;
	const float OriginalTarget = Target;

	const float MaxChange = MaxSpeed * SmoothTime;
	Change = FMath::Clamp(Change, -MaxChange, MaxChange);
	Target = Current - Change;

	const float Temp = (CurrentVelocity + Omega * Change) * DeltaSeconds;
	CurrentVelocity = (CurrentVelocity - Omega * Temp) * Exp;
	float Output = Target + (Change + Temp) * Exp;

	if (((OriginalTarget - Current) > 0.0f) == (Output > OriginalTarget))
	{
		Output = OriginalTarget;
		CurrentVelocity = (Output - OriginalTarget) / DeltaSeconds;
	}
	return Output;
}

FVector AOrbitCameraBase::SmoothDampVector(const FVector& Current, const FVector& Target, FVector3f& CurrentVelocity, float SmoothTime, float DeltaSeconds, float MaxSpeed) const
{
	FVector Result;
	Result.X = SmoothDampFloat(Current.X, Target.X, CurrentVelocity.X, SmoothTime, DeltaSeconds, MaxSpeed);
	Result.Y = SmoothDampFloat(Current.Y, Target.Y, CurrentVelocity.Y, SmoothTime, DeltaSeconds, MaxSpeed);
	Result.Z = SmoothDampFloat(Current.Z, Target.Z, CurrentVelocity.Z, SmoothTime, DeltaSeconds, MaxSpeed);
	return Result;
}

void AOrbitCameraBase::UpdateCollisionSoftSolve(float DeltaSeconds)
{
	if (!bEnableCollisionSoftSolve || !GetWorld() || !OrbitRoot || !CineCamRef)
	{
		return;
	}

	const FVector Start = OrbitRoot->GetComponentLocation();
	const FVector DesiredCameraPos = OrbitRoot->GetComponentLocation() - (OrbitRoot->GetForwardVector() * Internal_TargetDistance);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OrbitCameraCollision), false, this);
	const bool bBlockingHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, DesiredCameraPos, ECC_Camera, Params);

	if (bBlockingHit)
	{
		const float SafeDistance = FMath::Max(MinDistance, Hit.Distance - 8.0f);
		Internal_TargetDistance = FMath::FInterpTo(Internal_TargetDistance, SafeDistance, DeltaSeconds, 10.0f);
	}
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

	// 2) Optional camera clamping pass (can cause extra correction/snap if enabled).
	if (bClampCameraPositionToBounds)
	{
		ClampCameraToBounds();
	}
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
