// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "Camera/CameraComponent.h"
#include "CineCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
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

	if (ActiveOrbitCamera)
	{
		SetActorLocationAndRotation(ActiveOrbitCamera->GetActorLocation(), ActiveOrbitCamera->GetActorRotation());
		CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
	}

	if (bWalkModeOnly || bStartInWalkOutMode)
	{
		EnterWalkOutMode();
	}
}

// Called every frame
void AOrbitCameraManagerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateModeTransition(DeltaTime);

	if (bIsWalkOutMode)
	{
		UpdateWalkOutMovement(DeltaTime);
	}
}

// Called to bind functionality to input
void AOrbitCameraManagerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindKey(ToggleWalkOutKey, IE_Pressed, this, &AOrbitCameraManagerBase::ToggleWalkOutMode);

	PlayerInputComponent->BindKey(MoveForwardKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetMoveForwardPressed);
	PlayerInputComponent->BindKey(MoveForwardKey, IE_Released, this, &AOrbitCameraManagerBase::SetMoveForwardReleased);
	PlayerInputComponent->BindKey(MoveBackwardKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetMoveBackwardPressed);
	PlayerInputComponent->BindKey(MoveBackwardKey, IE_Released, this, &AOrbitCameraManagerBase::SetMoveBackwardReleased);
	PlayerInputComponent->BindKey(MoveRightKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetMoveRightPressed);
	PlayerInputComponent->BindKey(MoveRightKey, IE_Released, this, &AOrbitCameraManagerBase::SetMoveRightReleased);
	PlayerInputComponent->BindKey(MoveLeftKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetMoveLeftPressed);
	PlayerInputComponent->BindKey(MoveLeftKey, IE_Released, this, &AOrbitCameraManagerBase::SetMoveLeftReleased);
	PlayerInputComponent->BindKey(MoveUpKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetMoveUpPressed);
	PlayerInputComponent->BindKey(MoveUpKey, IE_Released, this, &AOrbitCameraManagerBase::SetMoveUpReleased);
	PlayerInputComponent->BindKey(MoveDownKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetMoveDownPressed);
	PlayerInputComponent->BindKey(MoveDownKey, IE_Released, this, &AOrbitCameraManagerBase::SetMoveDownReleased);
	PlayerInputComponent->BindKey(SprintKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetSprintPressed);
	PlayerInputComponent->BindKey(SprintKey, IE_Released, this, &AOrbitCameraManagerBase::SetSprintReleased);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AOrbitCameraManagerBase::OnLookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AOrbitCameraManagerBase::OnLookPitch);
}

void AOrbitCameraManagerBase::ToggleWalkOutMode()
{
	if (bWalkModeOnly)
	{
		EnterWalkOutMode();
		return;
	}

	if (!bAllowWalkOutMode)
	{
		return;
	}

	if (bIsWalkOutMode)
	{
		ExitWalkOutMode();
		return;
	}

	EnterWalkOutMode();
}

void AOrbitCameraManagerBase::EnterWalkOutMode()
{
	if (bIsWalkOutMode)
	{
		return;
	}

	bIsWalkOutMode = true;
	WalkModeStartLocation = GetActorLocation();
	WalkModeStartRotation = GetActorRotation();
	WalkBoundsOrigin = WalkModeStartLocation;

	if (ActiveOrbitCamera)
	{
		CaptureOrbitCameraTransform(StartCameraDefinition, ActiveOrbitCamera);
	}
}

void AOrbitCameraManagerBase::ExitWalkOutMode()
{
	if (bWalkModeOnly)
	{
		return;
	}

	if (!bIsWalkOutMode)
	{
		return;
	}

	bIsWalkOutMode = false;
	bMoveForwardPressed = false;
	bMoveBackwardPressed = false;
	bMoveRightPressed = false;
	bMoveLeftPressed = false;
	bMoveUpPressed = false;
	bMoveDownPressed = false;
	bSprintPressed = false;

	if (ActiveOrbitCamera)
	{
		TransitionToOrbitCamera(ActiveOrbitCamera, ModeTransitionDuration);
	}
}

void AOrbitCameraManagerBase::SetWalkOutModeEnabled(bool bEnable)
{
	if (bEnable)
	{
		EnterWalkOutMode();
		return;
	}

	ExitWalkOutMode();
}

void AOrbitCameraManagerBase::CutToOrbitCamera(AOrbitCameraBase* NewOrbitCamera)
{
	if (!NewOrbitCamera)
	{
		return;
	}

	ActiveOrbitCamera = NewOrbitCamera;
	bTransitionInProgress = false;
	SetActorLocationAndRotation(NewOrbitCamera->GetActorLocation(), NewOrbitCamera->GetActorRotation());
	CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
}

void AOrbitCameraManagerBase::TransitionToOrbitCamera(AOrbitCameraBase* NewOrbitCamera, float Duration)
{
	if (!NewOrbitCamera)
	{
		return;
	}

	ActiveOrbitCamera = NewOrbitCamera;
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
	}
}

void AOrbitCameraManagerBase::UpdateWalkOutMovement(float DeltaTime)
{
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	const FVector Up = FVector::UpVector;

	float ForwardInput = 0.0f;
	ForwardInput += bMoveForwardPressed ? 1.0f : 0.0f;
	ForwardInput -= bMoveBackwardPressed ? 1.0f : 0.0f;

	float RightInput = 0.0f;
	RightInput += bMoveRightPressed ? 1.0f : 0.0f;
	RightInput -= bMoveLeftPressed ? 1.0f : 0.0f;

	float UpInput = 0.0f;
	UpInput += bMoveUpPressed ? 1.0f : 0.0f;
	UpInput -= bMoveDownPressed ? 1.0f : 0.0f;

	FVector MoveDir = (Forward * ForwardInput) + (Right * RightInput) + (Up * UpInput);
	if (MoveDir.IsNearlyZero())
	{
		return;
	}

	MoveDir = MoveDir.GetSafeNormal();
	const float Speed = WalkMoveSpeed * (bSprintPressed ? WalkSprintMultiplier : 1.0f);

	FVector Target = GetActorLocation() + (MoveDir * Speed * DeltaTime);
	ClampWalkLocation(Target);
	SetActorLocation(Target);
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

void AOrbitCameraManagerBase::ClampWalkLocation(FVector& InOutLocation) const
{
	if (!bEnableWalkBounds)
	{
		return;
	}

	if (WalkBoundsActor)
	{
		const UBoxComponent* Bounds = WalkBoundsActor->FindComponentByClass<UBoxComponent>();
		if (Bounds)
		{
			const FVector Center = Bounds->GetComponentLocation();
			const FVector Extent = Bounds->GetScaledBoxExtent();
			InOutLocation.X = FMath::Clamp(InOutLocation.X, Center.X - Extent.X, Center.X + Extent.X);
			InOutLocation.Y = FMath::Clamp(InOutLocation.Y, Center.Y - Extent.Y, Center.Y + Extent.Y);
			InOutLocation.Z = FMath::Clamp(InOutLocation.Z, Center.Z - Extent.Z, Center.Z + Extent.Z);
			return;
		}
	}

	InOutLocation.X = FMath::Clamp(InOutLocation.X, WalkBoundsOrigin.X - WalkBoundsExtent.X, WalkBoundsOrigin.X + WalkBoundsExtent.X);
	InOutLocation.Y = FMath::Clamp(InOutLocation.Y, WalkBoundsOrigin.Y - WalkBoundsExtent.Y, WalkBoundsOrigin.Y + WalkBoundsExtent.Y);
	InOutLocation.Z = FMath::Clamp(InOutLocation.Z, WalkBoundsOrigin.Z - WalkBoundsExtent.Z, WalkBoundsOrigin.Z + WalkBoundsExtent.Z);
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

void AOrbitCameraManagerBase::SetMoveForwardPressed() { bMoveForwardPressed = true; }
void AOrbitCameraManagerBase::SetMoveForwardReleased() { bMoveForwardPressed = false; }
void AOrbitCameraManagerBase::SetMoveBackwardPressed() { bMoveBackwardPressed = true; }
void AOrbitCameraManagerBase::SetMoveBackwardReleased() { bMoveBackwardPressed = false; }
void AOrbitCameraManagerBase::SetMoveRightPressed() { bMoveRightPressed = true; }
void AOrbitCameraManagerBase::SetMoveRightReleased() { bMoveRightPressed = false; }
void AOrbitCameraManagerBase::SetMoveLeftPressed() { bMoveLeftPressed = true; }
void AOrbitCameraManagerBase::SetMoveLeftReleased() { bMoveLeftPressed = false; }
void AOrbitCameraManagerBase::SetMoveUpPressed() { bMoveUpPressed = true; }
void AOrbitCameraManagerBase::SetMoveUpReleased() { bMoveUpPressed = false; }
void AOrbitCameraManagerBase::SetMoveDownPressed() { bMoveDownPressed = true; }
void AOrbitCameraManagerBase::SetMoveDownReleased() { bMoveDownPressed = false; }
void AOrbitCameraManagerBase::SetSprintPressed() { bSprintPressed = true; }
void AOrbitCameraManagerBase::SetSprintReleased() { bSprintPressed = false; }

void AOrbitCameraManagerBase::OnLookYaw(float Value)
{
	if (!bIsWalkOutMode || FMath::IsNearlyZero(Value))
	{
		return;
	}

	FRotator Rot = GetActorRotation();
	Rot.Yaw += Value * WalkLookYawSpeed * GetWorld()->GetDeltaSeconds();
	SetActorRotation(Rot);
}

void AOrbitCameraManagerBase::OnLookPitch(float Value)
{
	if (!bIsWalkOutMode || FMath::IsNearlyZero(Value))
	{
		return;
	}

	FRotator Rot = GetActorRotation();
	Rot.Pitch = FMath::Clamp(Rot.Pitch + (-Value * WalkLookPitchSpeed * GetWorld()->GetDeltaSeconds()), WalkMinPitch, WalkMaxPitch);
	SetActorRotation(Rot);
}
