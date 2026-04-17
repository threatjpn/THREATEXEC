// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "Camera/CameraComponent.h"
#include "CineCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AOrbitCameraManagerBase::AOrbitCameraManagerBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AOrbitCameraManagerBase::BeginPlay()
{
	Super::BeginPlay();

	if (!ActiveOrbitCamera && bAutoFindOrbitCameraIfUnset)
	{
		TArray<AActor*> FoundOrbitCameras;
		UGameplayStatics::GetAllActorsOfClass(this, AOrbitCameraBase::StaticClass(), FoundOrbitCameras);

		AOrbitCameraBase* FirstOrbitCamera = nullptr;
		AOrbitCameraBase* TaggedOrbitCamera = nullptr;
		for (AActor* FoundActor : FoundOrbitCameras)
		{
			AOrbitCameraBase* FoundOrbitCamera = Cast<AOrbitCameraBase>(FoundActor);
			if (!FoundOrbitCamera)
			{
				continue;
			}

			if (!FirstOrbitCamera)
			{
				FirstOrbitCamera = FoundOrbitCamera;
			}

			if (!AutoFindOrbitCameraTag.IsNone() && FoundOrbitCamera->ActorHasTag(AutoFindOrbitCameraTag))
			{
				TaggedOrbitCamera = FoundOrbitCamera;
				break;
			}
		}

		ActiveOrbitCamera = TaggedOrbitCamera ? TaggedOrbitCamera : FirstOrbitCamera;
	}

	if (bAutoPossessPlayer0OnBeginPlay)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PlayerController->GetPawn() != this)
			{
				PlayerController->Possess(this);
			}
		}
	}

	if (ActiveOrbitCamera)
	{
		SetActorLocationAndRotation(ActiveOrbitCamera->GetActorLocation(), ActiveOrbitCamera->GetActorRotation());
		CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
	}

	ApplyDesiredViewTarget();
}

// Called every frame
void AOrbitCameraManagerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateModeTransition(DeltaTime);
	UpdateOrbitCameraSmoothing(DeltaTime);
}

// Called to bind functionality to input
void AOrbitCameraManagerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindKey(OrbitLookHoldKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetOrbitLookPressed);
	PlayerInputComponent->BindKey(OrbitLookHoldKey, IE_Released, this, &AOrbitCameraManagerBase::SetOrbitLookReleased);
	PlayerInputComponent->BindKey(OrbitPanHoldKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetOrbitPanPressed);
	PlayerInputComponent->BindKey(OrbitPanHoldKey, IE_Released, this, &AOrbitCameraManagerBase::SetOrbitPanReleased);
	PlayerInputComponent->BindKey(CycleOrbitCameraKey, IE_Pressed, this, &AOrbitCameraManagerBase::CycleToNextOrbitCamera);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AOrbitCameraManagerBase::OnLookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AOrbitCameraManagerBase::OnLookPitch);
	PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AOrbitCameraManagerBase::OnOrbitMouseWheel);
}

void AOrbitCameraManagerBase::ApplyDesiredViewTarget()
{
	if (!bAutoManageViewTarget)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	AActor* DesiredViewTarget = Cast<AActor>(ActiveOrbitCamera.Get());
	if (!DesiredViewTarget)
	{
		DesiredViewTarget = this;
	}

	if (PlayerController->GetViewTarget() != DesiredViewTarget)
	{
		PlayerController->SetViewTarget(DesiredViewTarget);
	}
}

void AOrbitCameraManagerBase::EnsureOrbitTargetsInitialized()
{
	if (bOrbitTargetsInitialized || !ActiveOrbitCamera || !ActiveOrbitCamera->CineCamRef || !ActiveOrbitCamera->OrbitRoot)
	{
		return;
	}

	OrbitTargetRootLocation = ActiveOrbitCamera->OrbitRoot->GetComponentLocation();
	OrbitTargetRootRotation = ActiveOrbitCamera->OrbitRoot->GetRelativeRotation();
	OrbitTargetDistance = FMath::Abs(ActiveOrbitCamera->CineCamRef->GetRelativeLocation().X);
	OrbitTargetFocalLength = ActiveOrbitCamera->CineCamRef->CurrentFocalLength;
	OrbitLookBoundsPushOffset = FVector::ZeroVector;
	bOrbitTargetsInitialized = true;
}


const UBoxComponent* AOrbitCameraManagerBase::GetActiveBoundsBox() const
{
	if (!bEnforceOrbitBoundsInManager || !ActiveOrbitCamera || !ActiveOrbitCamera->bClampToBounds || !ActiveOrbitCamera->CameraBoundsActor)
	{
		return nullptr;
	}

	return ActiveOrbitCamera->CameraBoundsActor->FindComponentByClass<UBoxComponent>();
}

FVector AOrbitCameraManagerBase::ClampPointToActiveBounds(const FVector& Point) const
{
	const UBoxComponent* BoundsBox = GetActiveBoundsBox();
	if (!BoundsBox)
	{
		return Point;
	}

	const FVector Center = BoundsBox->GetComponentLocation();
	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(ActiveOrbitCamera->BoundsPadding + ManagerBoundsPadding);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	return FVector(
		FMath::Clamp(Point.X, Center.X - Extent.X, Center.X + Extent.X),
		FMath::Clamp(Point.Y, Center.Y - Extent.Y, Center.Y + Extent.Y),
		FMath::Clamp(Point.Z, Center.Z - Extent.Z, Center.Z + Extent.Z));
}

FVector AOrbitCameraManagerBase::CalculateLookBoundsPushOffset(const FVector& RootLocation, const FRotator& RootRotation, float CameraDistance) const
{
	if (!bEnforceOrbitBoundsInManager || !bConstrainOrbitCameraToBounds || !bUseTransientBoundsPushForLook)
	{
		return FVector::ZeroVector;
	}

	const UBoxComponent* BoundsBox = GetActiveBoundsBox();
	if (!BoundsBox)
	{
		return FVector::ZeroVector;
	}

	const FVector CameraOffset = RootRotation.RotateVector(FVector(-CameraDistance, 0.0f, 0.0f));
	const FVector DesiredCameraLocation = RootLocation + CameraOffset;

	const FVector Center = BoundsBox->GetComponentLocation();
	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(ActiveOrbitCamera->BoundsPadding + ManagerBoundsPadding + LookBoundsPushInset);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	const FVector ClampedCameraLocation(
		FMath::Clamp(DesiredCameraLocation.X, Center.X - Extent.X, Center.X + Extent.X),
		FMath::Clamp(DesiredCameraLocation.Y, Center.Y - Extent.Y, Center.Y + Extent.Y),
		FMath::Clamp(DesiredCameraLocation.Z, Center.Z - Extent.Z, Center.Z + Extent.Z));

	return ClampedCameraLocation - DesiredCameraLocation;
}


float AOrbitCameraManagerBase::ResolveMaxOrbitDistanceForBounds(const FVector& RootLocation, const FRotator& RootRotation, float DesiredDistance) const
{
	if (!bEnforceOrbitBoundsInManager || !bConstrainOrbitCameraToBounds || !bUseDistanceCompressionForLookBounds || bUseTransientBoundsPushForLook)
	{
		return DesiredDistance;
	}

	const UBoxComponent* BoundsBox = GetActiveBoundsBox();
	if (!BoundsBox)
	{
		return DesiredDistance;
	}

	const FVector Center = BoundsBox->GetComponentLocation();
	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(ActiveOrbitCamera->BoundsPadding + ManagerBoundsPadding + LookBoundsDistanceInset);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	const FVector Dir = RootRotation.RotateVector(FVector(-1.0f, 0.0f, 0.0f)).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return DesiredDistance;
	}

	float MaxDistance = TNumericLimits<float>::Max();
	const FVector LocalRoot = RootLocation - Center;

	auto AccumulateAxisLimit = [&](float RootAxisComponent, float DirComponent, float AxisExtent)
	{
		if (FMath::Abs(DirComponent) <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float Bound = (DirComponent > 0.0f) ? AxisExtent : -AxisExtent;
		const float Candidate = (Bound - RootAxisComponent) / DirComponent;
		MaxDistance = FMath::Min(MaxDistance, Candidate);
	};

	AccumulateAxisLimit(LocalRoot.X, Dir.X, Extent.X);
	AccumulateAxisLimit(LocalRoot.Y, Dir.Y, Extent.Y);
	AccumulateAxisLimit(LocalRoot.Z, Dir.Z, Extent.Z);

	if (!FMath::IsFinite(MaxDistance))
	{
		return DesiredDistance;
	}

	const float SafeMaxDistance = FMath::Max(LookBoundsMinDistance, MaxDistance);
	return FMath::Clamp(DesiredDistance, LookBoundsMinDistance, SafeMaxDistance);
}

FVector AOrbitCameraManagerBase::ConstrainOrbitRootLocationToBounds(
	const FVector& DesiredRootLocation,
	float DeltaTime,
	bool bApplyCorrectionSmoothing) const
{
	if (!bEnforceOrbitBoundsInManager || !bConstrainOrbitRootToBounds)
	{
		return DesiredRootLocation;
	}

	const FVector CorrectedRootLocation = ClampPointToActiveBounds(DesiredRootLocation);

	if (bApplyCorrectionSmoothing && bSmoothOrbitControls && OrbitBoundsCorrectionSpeed > 0.0f)
	{
		return FMath::VInterpTo(DesiredRootLocation, CorrectedRootLocation, DeltaTime, OrbitBoundsCorrectionSpeed);
	}

	return CorrectedRootLocation;
}

void AOrbitCameraManagerBase::UpdateOrbitCameraSmoothing(float DeltaTime)
{
	if (!ActiveOrbitCamera || !ActiveOrbitCamera->CineCamRef || !ActiveOrbitCamera->OrbitRoot)
	{
		bOrbitTargetsInitialized = false;
		OrbitLookBoundsPushOffset = FVector::ZeroVector;
		return;
	}

	EnsureOrbitTargetsInitialized();

	OrbitTargetRootLocation = ConstrainOrbitRootLocationToBounds(
		OrbitTargetRootLocation,
		DeltaTime,
		false);

	if (!bSmoothOrbitControls)
	{
		const FVector BoundedLocation = ConstrainOrbitRootLocationToBounds(OrbitTargetRootLocation, DeltaTime, false);
		const float BoundedDistance = ResolveMaxOrbitDistanceForBounds(BoundedLocation, OrbitTargetRootRotation, OrbitTargetDistance);
		const FVector TargetPushOffset = CalculateLookBoundsPushOffset(BoundedLocation, OrbitTargetRootRotation, BoundedDistance);
		OrbitLookBoundsPushOffset = TargetPushOffset;

		ActiveOrbitCamera->OrbitRoot->SetWorldLocation(BoundedLocation + OrbitLookBoundsPushOffset);
		ActiveOrbitCamera->OrbitRoot->SetRelativeRotation(OrbitTargetRootRotation);
		ActiveOrbitCamera->CineCamRef->SetRelativeLocation(FVector(-BoundedDistance, 0.0f, 0.0f));
		ActiveOrbitCamera->CineCamRef->CurrentFocalLength = OrbitTargetFocalLength;
		OrbitTargetDistance = BoundedDistance;
	}
	else
	{
		const FVector RawSmoothedLocation = FMath::VInterpTo(
			ActiveOrbitCamera->OrbitRoot->GetComponentLocation() - OrbitLookBoundsPushOffset,
			OrbitTargetRootLocation,
			DeltaTime,
			OrbitPanSmoothingSpeed);

		const FRotator SmoothedRotation = FMath::RInterpTo(
			ActiveOrbitCamera->OrbitRoot->GetRelativeRotation(),
			OrbitTargetRootRotation,
			DeltaTime,
			OrbitLookSmoothingSpeed);

		const FVector SmoothedBaseLocation = ConstrainOrbitRootLocationToBounds(RawSmoothedLocation, DeltaTime, true);

		const float CurrentDistance = FMath::Abs(ActiveOrbitCamera->CineCamRef->GetRelativeLocation().X);
		const float DesiredDistanceForBounds = ResolveMaxOrbitDistanceForBounds(SmoothedBaseLocation, SmoothedRotation, OrbitTargetDistance);
		const bool bCompressing = DesiredDistanceForBounds < CurrentDistance;
		const float DistanceInterpSpeed = bCompressing ? LookBoundsCompressionSpeed : LookBoundsRecoverySpeed;
		const float SmoothedDistance = FMath::FInterpTo(
			CurrentDistance,
			DesiredDistanceForBounds,
			DeltaTime,
			FMath::Max(0.0f, DistanceInterpSpeed > 0.0f ? DistanceInterpSpeed : OrbitZoomSmoothingSpeed));
		OrbitTargetDistance = DesiredDistanceForBounds;

		const FVector TargetPushOffset = CalculateLookBoundsPushOffset(SmoothedBaseLocation, SmoothedRotation, SmoothedDistance);
		const bool bHasPushTarget = !TargetPushOffset.IsNearlyZero(0.01f);
		const float PushInterpSpeed = bHasPushTarget ? LookBoundsPushInterpSpeed : LookBoundsReturnSpeed;
		OrbitLookBoundsPushOffset = FMath::VInterpTo(
			OrbitLookBoundsPushOffset,
			TargetPushOffset,
			DeltaTime,
			FMath::Max(0.0f, PushInterpSpeed > 0.0f ? PushInterpSpeed : OrbitBoundsCorrectionSpeed));

		const float SmoothedFocal = FMath::FInterpTo(
			ActiveOrbitCamera->CineCamRef->CurrentFocalLength,
			OrbitTargetFocalLength,
			DeltaTime,
			OrbitZoomSmoothingSpeed);

		ActiveOrbitCamera->OrbitRoot->SetWorldLocation(SmoothedBaseLocation + OrbitLookBoundsPushOffset);
		ActiveOrbitCamera->OrbitRoot->SetRelativeRotation(SmoothedRotation);
		ActiveOrbitCamera->CineCamRef->SetRelativeLocation(FVector(-SmoothedDistance, 0.0f, 0.0f));
		ActiveOrbitCamera->CineCamRef->CurrentFocalLength = SmoothedFocal;
	}

	ActiveOrbitCamera->Internal_CurrentRotation = ActiveOrbitCamera->OrbitRoot->GetRelativeRotation();
	ActiveOrbitCamera->Internal_TargetRotation = OrbitTargetRootRotation;
	ActiveOrbitCamera->Internal_CurrentDistance = FMath::Abs(ActiveOrbitCamera->CineCamRef->GetRelativeLocation().X);
	ActiveOrbitCamera->Internal_TargetDistance = OrbitTargetDistance;
	ActiveOrbitCamera->Internal_CurrentFocalLength = ActiveOrbitCamera->CineCamRef->CurrentFocalLength;
	ActiveOrbitCamera->Internal_TargetFocalLength = OrbitTargetFocalLength;
}

void AOrbitCameraManagerBase::CutToOrbitCamera(AOrbitCameraBase* NewOrbitCamera)
{
	if (!NewOrbitCamera)
	{
		return;
	}

	ActiveOrbitCamera = NewOrbitCamera;
	bOrbitTargetsInitialized = false;
	bTransitionInProgress = false;
	OrbitLookBoundsPushOffset = FVector::ZeroVector;
	SetActorLocationAndRotation(NewOrbitCamera->GetActorLocation(), NewOrbitCamera->GetActorRotation());
	CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
	ApplyDesiredViewTarget();
}

void AOrbitCameraManagerBase::TransitionToOrbitCamera(AOrbitCameraBase* NewOrbitCamera, float Duration)
{
	if (!NewOrbitCamera)
	{
		return;
	}

	ActiveOrbitCamera = NewOrbitCamera;
	bOrbitTargetsInitialized = false;
	OrbitLookBoundsPushOffset = FVector::ZeroVector;
	CaptureOrbitCameraTransform(TargetCameraDefinition, ActiveOrbitCamera);

	TransitionStart = GetActorTransform();
	TransitionTarget = ActiveOrbitCamera->GetActorTransform();
	TransitionDuration = FMath::Max(0.0f, Duration);
	TransitionElapsed = 0.0f;
	bTransitionInProgress = TransitionDuration > 0.0f;

	if (!bTransitionInProgress)
	{
		SetActorTransform(TransitionTarget);
		CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
		ApplyDesiredViewTarget();
	}
}

void AOrbitCameraManagerBase::CycleToNextOrbitCamera()
{
	CycleOrbitCamera(1);
}

void AOrbitCameraManagerBase::CycleToPreviousOrbitCamera()
{
	CycleOrbitCamera(-1);
}

void AOrbitCameraManagerBase::CycleOrbitCamera(int32 Direction)
{
	if (Direction == 0 || !GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AOrbitCameraBase::StaticClass(), FoundActors);

	TArray<AOrbitCameraBase*> OrbitCameras;
	OrbitCameras.Reserve(FoundActors.Num());
	for (AActor* FoundActor : FoundActors)
	{
		if (AOrbitCameraBase* OrbitCamera = Cast<AOrbitCameraBase>(FoundActor))
		{
			OrbitCameras.Add(OrbitCamera);
		}
	}

	if (OrbitCameras.Num() < 2)
	{
		return;
	}

	OrbitCameras.Sort([](const AOrbitCameraBase* A, const AOrbitCameraBase* B)
	{
		if (!A || !B)
		{
			return A != nullptr;
		}

		return A->GetName() < B->GetName();
	});

	const int32 CurrentIndex = OrbitCameras.IndexOfByKey(ActiveOrbitCamera.Get());
	const int32 WrappedCurrentIndex = CurrentIndex >= 0 ? CurrentIndex : 0;
	const int32 NextIndex = (WrappedCurrentIndex + Direction + OrbitCameras.Num()) % OrbitCameras.Num();
	AOrbitCameraBase* NextOrbitCamera = OrbitCameras[NextIndex];

	if (!NextOrbitCamera || NextOrbitCamera == ActiveOrbitCamera)
	{
		return;
	}

	if (bUseTransitionWhenCycling)
	{
		TransitionToOrbitCamera(NextOrbitCamera, ModeTransitionDuration);
		return;
	}

	CutToOrbitCamera(NextOrbitCamera);
}

void AOrbitCameraManagerBase::UpdateModeTransition(float DeltaTime)
{
	if (!bTransitionInProgress)
	{
		return;
	}

	TransitionElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(TransitionElapsed / TransitionDuration, 0.0f, 1.0f);
	const float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	const FVector Pos = FMath::Lerp(TransitionStart.GetLocation(), TransitionTarget.GetLocation(), SmoothedAlpha);
	const FQuat Rot = FQuat::Slerp(TransitionStart.GetRotation(), TransitionTarget.GetRotation(), SmoothedAlpha);
	SetActorLocationAndRotation(Pos, Rot);

	if (Alpha >= 1.0f)
	{
		bTransitionInProgress = false;
		if (ActiveOrbitCamera)
		{
			CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
		}
	}
}

void AOrbitCameraManagerBase::CaptureOrbitCameraTransform(FOrbitCameraDefinition& OutDefinition, AOrbitCameraBase* InCamera) const
{
	if (!InCamera)
	{
		return;
	}

	OutDefinition.ActorTransform = InCamera->GetActorTransform();
	OutDefinition.Position = InCamera->GetActorLocation();
	OutDefinition.Rotation = InCamera->GetActorRotation();
	OutDefinition.Distance = InCamera->Internal_CurrentDistance;
	OutDefinition.Zoom = InCamera->Internal_CurrentFocalLength;
	OutDefinition.FoV = InCamera->GetCineCameraComponent() ? InCamera->GetCineCameraComponent()->FieldOfView : OutDefinition.FoV;
}

void AOrbitCameraManagerBase::SetOrbitLookPressed() { bOrbitLookPressed = true; }
void AOrbitCameraManagerBase::SetOrbitLookReleased() { bOrbitLookPressed = false; }
void AOrbitCameraManagerBase::SetOrbitPanPressed() { bOrbitPanPressed = true; }
void AOrbitCameraManagerBase::SetOrbitPanReleased() { bOrbitPanPressed = false; }

void AOrbitCameraManagerBase::OnOrbitMouseWheel(float Value)
{
	if (FMath::IsNearlyZero(Value) || !ActiveOrbitCamera || !ActiveOrbitCamera->CineCamRef)
	{
		return;
	}
	EnsureOrbitTargetsInitialized();

	const float DistanceMin = FMath::Min(ActiveOrbitCamera->MinDistance, ActiveOrbitCamera->MaxDistance);
	const float DistanceMax = FMath::Max(ActiveOrbitCamera->MinDistance, ActiveOrbitCamera->MaxDistance);
	OrbitTargetDistance = FMath::Clamp(OrbitTargetDistance - (Value * OrbitZoomStep), DistanceMin, DistanceMax);

	const float FocalMin = FMath::Min(ActiveOrbitCamera->MinFocalLength, ActiveOrbitCamera->MaxFocalLength);
	const float FocalMax = FMath::Max(ActiveOrbitCamera->MinFocalLength, ActiveOrbitCamera->MaxFocalLength);
	OrbitTargetFocalLength = FMath::Clamp(OrbitTargetFocalLength + (Value * 1.5f), FocalMin, FocalMax);
}

void AOrbitCameraManagerBase::OnLookYaw(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	if (!ActiveOrbitCamera || !ActiveOrbitCamera->OrbitRoot)
	{
		return;
	}
	EnsureOrbitTargetsInitialized();

 	if (bOrbitPanPressed)
	{
		const FVector RightMove = ActiveOrbitCamera->OrbitRoot->GetRightVector() * (Value * OrbitPanSpeed * 25.0f);
		OrbitTargetRootLocation = ConstrainOrbitRootLocationToBounds(
			OrbitTargetRootLocation + RightMove,
			0.0f,
			false);
		return;
	}

	if (!bOrbitLookPressed)
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	FRotator OrbitRot = OrbitTargetRootRotation;
	const float YawMin = FMath::Min(ActiveOrbitCamera->MinYaw, ActiveOrbitCamera->MaxYaw);
	const float YawMax = FMath::Max(ActiveOrbitCamera->MinYaw, ActiveOrbitCamera->MaxYaw);
	const float YawDelta = Value * OrbitLookYawSpeed * DeltaSeconds;

	// Treat the default (-359..359) range as effectively unbounded so users can keep orbiting
	// through multiple full rotations without hitting a hard yaw stop.
	const bool bYawIsEffectivelyUnbounded = (YawMin <= -359.0f && YawMax >= 359.0f);
	if (bYawIsEffectivelyUnbounded)
	{
		OrbitRot.Yaw += YawDelta;
	}
	else
	{
		OrbitRot.Yaw = FMath::Clamp(OrbitRot.Yaw + YawDelta, YawMin, YawMax);
	}

	OrbitTargetRootRotation = OrbitRot;
}

void AOrbitCameraManagerBase::OnLookPitch(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	if (!ActiveOrbitCamera || !ActiveOrbitCamera->OrbitRoot)
	{
		return;
	}
	EnsureOrbitTargetsInitialized();

 	if (bOrbitPanPressed)
	{
		const FVector UpMove = ActiveOrbitCamera->OrbitRoot->GetUpVector() * (Value * OrbitPanSpeed * 25.0f);
		OrbitTargetRootLocation = ConstrainOrbitRootLocationToBounds(
			OrbitTargetRootLocation + UpMove,
			0.0f,
			false);
		return;
	}

	if (!bOrbitLookPressed)
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	FRotator OrbitRot = OrbitTargetRootRotation;
	const float PitchMin = FMath::Min(ActiveOrbitCamera->MinPitch, ActiveOrbitCamera->MaxPitch);
	const float PitchMax = FMath::Max(ActiveOrbitCamera->MinPitch, ActiveOrbitCamera->MaxPitch);
	OrbitRot.Pitch = FMath::Clamp(OrbitRot.Pitch + (Value * OrbitLookPitchSpeed * DeltaSeconds), PitchMin, PitchMax);
	OrbitTargetRootRotation = OrbitRot;
}
