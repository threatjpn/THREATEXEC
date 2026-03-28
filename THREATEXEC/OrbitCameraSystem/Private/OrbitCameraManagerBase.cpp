// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "OrbitWalkCharacter.h"
#include "Camera/CameraComponent.h"
#include "CineCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AOrbitCameraManagerBase::AOrbitCameraManagerBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	WalkCharacterClass = AOrbitWalkCharacter::StaticClass();
}

// Called when the game starts or when spawned
void AOrbitCameraManagerBase::BeginPlay()
{
	Super::BeginPlay();
	ManagerPlacedTransform = GetActorTransform();
	const bool bStartInWalkMode = (bWalkModeOnly || bStartInWalkOutMode);

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

	if (ActiveOrbitCamera && !(bStartInWalkMode && bWalkSpawnFromManagerPlacement))
	{
		SetActorLocationAndRotation(ActiveOrbitCamera->GetActorLocation(), ActiveOrbitCamera->GetActorRotation());
		CaptureOrbitCameraTransform(CurrentCameraDefinition, ActiveOrbitCamera);
	}

	if (bStartInWalkMode)
	{
		if (bWalkSpawnFromManagerPlacement)
		{
			SetActorTransform(ManagerPlacedTransform);
		}
		EnterWalkOutMode();
	}

	ApplyDesiredViewTarget();
}

// Called every frame
void AOrbitCameraManagerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateModeTransition(DeltaTime);

	if (bIsWalkOutMode)
	{
		// Character-based walk mode handles movement input directly.
		// Fallback to manager walk movement if character spawn/possession failed.
		if (!WalkCharacterInstance)
		{
			UpdateWalkOutMovement(DeltaTime);
		}
	}
	else
	{
		UpdateOrbitCameraSmoothing(DeltaTime);
	}

	UpdateDebugOverlay(DeltaTime);
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
	PlayerInputComponent->BindKey(OrbitLookHoldKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetOrbitLookPressed);
	PlayerInputComponent->BindKey(OrbitLookHoldKey, IE_Released, this, &AOrbitCameraManagerBase::SetOrbitLookReleased);
	PlayerInputComponent->BindKey(OrbitPanHoldKey, IE_Pressed, this, &AOrbitCameraManagerBase::SetOrbitPanPressed);
	PlayerInputComponent->BindKey(OrbitPanHoldKey, IE_Released, this, &AOrbitCameraManagerBase::SetOrbitPanReleased);
	PlayerInputComponent->BindKey(ToggleDebugMenuKey, IE_Pressed, this, &AOrbitCameraManagerBase::ToggleDebugMenu);
	PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AOrbitCameraManagerBase::ToggleDebugTraces);
	PlayerInputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AOrbitCameraManagerBase::ToggleDebugBounds);
	PlayerInputComponent->BindKey(EKeys::F7, IE_Pressed, this, &AOrbitCameraManagerBase::ToggleDebugWalkState);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AOrbitCameraManagerBase::OnLookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AOrbitCameraManagerBase::OnLookPitch);
	PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AOrbitCameraManagerBase::OnOrbitMouseWheel);
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
	bOrbitTargetsInitialized = false;
	WalkVelocity = FVector::ZeroVector;
	WalkModeStartLocation = GetActorLocation();
	WalkModeStartRotation = GetActorRotation();
	WalkBoundsOrigin = WalkModeStartLocation;

	if (ActiveOrbitCamera)
	{
		CaptureOrbitCameraTransform(StartCameraDefinition, ActiveOrbitCamera);
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		const FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
		if (!WalkCharacterClass)
		{
			WalkCharacterClass = AOrbitWalkCharacter::StaticClass();
		}

		if (!WalkCharacterInstance && WalkCharacterClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			WalkCharacterInstance = GetWorld()->SpawnActor<AOrbitWalkCharacter>(WalkCharacterClass, SpawnTransform, SpawnParams);
		}

		if (WalkCharacterInstance)
		{
			WalkCharacterInstance->SetActorLocationAndRotation(GetActorLocation(), GetActorRotation());
			WalkCharacterInstance->InitializeFromManager(
				this,
				WalkMoveSpeed,
				WalkSprintMultiplier,
				WalkMinPitch,
				WalkMaxPitch,
				bAllowWalkVerticalInput,
				ToggleWalkOutKey,
				MoveForwardKey,
				MoveBackwardKey,
				MoveRightKey,
				MoveLeftKey,
				MoveUpKey,
				MoveDownKey,
				SprintKey);
			PlayerController->Possess(WalkCharacterInstance);
			PlayerController->SetViewTarget(WalkCharacterInstance);
		}
	}

	ApplyDesiredViewTarget();
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
	bOrbitTargetsInitialized = false;
	WalkVelocity = FVector::ZeroVector;
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

	if (WalkCharacterInstance)
	{
		SetActorLocationAndRotation(WalkCharacterInstance->GetActorLocation(), WalkCharacterInstance->GetActorRotation());

		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			PlayerController->Possess(this);
		}

		if (bDestroyWalkCharacterOnExit)
		{
			WalkCharacterInstance->Destroy();
			WalkCharacterInstance = nullptr;
		}
	}

	ApplyDesiredViewTarget();
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

	AActor* DesiredViewTarget = nullptr;
	if (bIsWalkOutMode)
	{
		DesiredViewTarget = (WalkCharacterInstance)
			? Cast<AActor>(WalkCharacterInstance.Get())
			: Cast<AActor>(this);
	}
	else
	{
		DesiredViewTarget = Cast<AActor>(ActiveOrbitCamera.Get());
	}
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
	bOrbitTargetsInitialized = true;
}

void AOrbitCameraManagerBase::ClampOrbitTargetToBounds()
{
	if (!ActiveOrbitCamera || !ActiveOrbitCamera->bClampToBounds || !ActiveOrbitCamera->CameraBoundsActor)
	{
		return;
	}

	const UBoxComponent* BoundsBox = ActiveOrbitCamera->CameraBoundsActor->FindComponentByClass<UBoxComponent>();
	if (!BoundsBox)
	{
		return;
	}

	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(ActiveOrbitCamera->BoundsPadding);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	const FVector Center = BoundsBox->GetComponentLocation();
	OrbitTargetRootLocation.X = FMath::Clamp(OrbitTargetRootLocation.X, Center.X - Extent.X, Center.X + Extent.X);
	OrbitTargetRootLocation.Y = FMath::Clamp(OrbitTargetRootLocation.Y, Center.Y - Extent.Y, Center.Y + Extent.Y);
	OrbitTargetRootLocation.Z = FMath::Clamp(OrbitTargetRootLocation.Z, Center.Z - Extent.Z, Center.Z + Extent.Z);
}

void AOrbitCameraManagerBase::UpdateOrbitCameraSmoothing(float DeltaTime)
{
	if (!ActiveOrbitCamera || !ActiveOrbitCamera->CineCamRef || !ActiveOrbitCamera->OrbitRoot)
	{
		bOrbitTargetsInitialized = false;
		return;
	}

	EnsureOrbitTargetsInitialized();
	ClampOrbitTargetToBounds();

	if (!bSmoothOrbitControls)
	{
		ActiveOrbitCamera->OrbitRoot->SetWorldLocation(OrbitTargetRootLocation);
		ActiveOrbitCamera->OrbitRoot->SetRelativeRotation(OrbitTargetRootRotation);
		ActiveOrbitCamera->CineCamRef->SetRelativeLocation(FVector(-OrbitTargetDistance, 0.0f, 0.0f));
		ActiveOrbitCamera->CineCamRef->CurrentFocalLength = OrbitTargetFocalLength;
	}
	else
	{
		const FVector SmoothedLocation = FMath::VInterpTo(
			ActiveOrbitCamera->OrbitRoot->GetComponentLocation(),
			OrbitTargetRootLocation,
			DeltaTime,
			OrbitPanSmoothingSpeed);
		const FRotator SmoothedRotation = FMath::RInterpTo(
			ActiveOrbitCamera->OrbitRoot->GetRelativeRotation(),
			OrbitTargetRootRotation,
			DeltaTime,
			OrbitLookSmoothingSpeed);
		const float SmoothedDistance = FMath::FInterpTo(
			FMath::Abs(ActiveOrbitCamera->CineCamRef->GetRelativeLocation().X),
			OrbitTargetDistance,
			DeltaTime,
			OrbitZoomSmoothingSpeed);
		const float SmoothedFocal = FMath::FInterpTo(
			ActiveOrbitCamera->CineCamRef->CurrentFocalLength,
			OrbitTargetFocalLength,
			DeltaTime,
			OrbitZoomSmoothingSpeed);

		ActiveOrbitCamera->OrbitRoot->SetWorldLocation(SmoothedLocation);
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
	ActiveOrbitCamera->ForceApplyBoundsClamp();
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

void AOrbitCameraManagerBase::UpdateWalkOutMovement(float DeltaTime)
{
	FVector Forward = GetActorForwardVector();
	FVector Right = GetActorRightVector();
	const FVector Up = FVector::UpVector;
	if (bWalkPlanarMovement)
	{
		Forward.Z = 0.0f;
		Right.Z = 0.0f;
		Forward = Forward.GetSafeNormal();
		Right = Right.GetSafeNormal();
	}

	float ForwardInput = 0.0f;
	ForwardInput += bMoveForwardPressed ? 1.0f : 0.0f;
	ForwardInput -= bMoveBackwardPressed ? 1.0f : 0.0f;

	float RightInput = 0.0f;
	RightInput += bMoveRightPressed ? 1.0f : 0.0f;
	RightInput -= bMoveLeftPressed ? 1.0f : 0.0f;

	float UpInput = 0.0f;
	if (bAllowWalkVerticalInput)
	{
		UpInput += bMoveUpPressed ? 1.0f : 0.0f;
		UpInput -= bMoveDownPressed ? 1.0f : 0.0f;
	}

	const float TargetSpeed = WalkMoveSpeed * (bSprintPressed ? WalkSprintMultiplier : 1.0f);
	const FVector DesiredHorizontalDirection = ((Forward * ForwardInput) + (Right * RightInput)).GetSafeNormal();
	const FVector DesiredHorizontalVelocity = DesiredHorizontalDirection * TargetSpeed;
	const FVector CurrentHorizontalVelocity(WalkVelocity.X, WalkVelocity.Y, 0.0f);

	FVector NewHorizontalVelocity = DesiredHorizontalVelocity;
	if (bUseFirstPersonWalkModel)
	{
		const bool bHasHorizontalInput = !DesiredHorizontalDirection.IsNearlyZero();
		const float InterpSpeed = bHasHorizontalInput
			? FMath::Max(1.0f, WalkAcceleration / FMath::Max(1.0f, TargetSpeed))
			: FMath::Max(1.0f, WalkBrakingDeceleration / FMath::Max(1.0f, WalkMoveSpeed));
		NewHorizontalVelocity = FMath::VInterpConstantTo(CurrentHorizontalVelocity, DesiredHorizontalVelocity, DeltaTime, InterpSpeed * TargetSpeed);
	}

	float NewVerticalVelocity = 0.0f;
	if (bAllowWalkVerticalInput)
	{
		const float DesiredVerticalVelocity = UpInput * WalkVerticalSpeed * (bSprintPressed ? WalkSprintMultiplier : 1.0f);
		if (bUseFirstPersonWalkModel)
		{
			const float VerticalInterpSpeed = FMath::Max(1.0f, WalkAcceleration / FMath::Max(1.0f, WalkVerticalSpeed));
			NewVerticalVelocity = FMath::FInterpConstantTo(WalkVelocity.Z, DesiredVerticalVelocity, DeltaTime, VerticalInterpSpeed * WalkVerticalSpeed);
		}
		else
		{
			NewVerticalVelocity = DesiredVerticalVelocity;
		}
	}

	WalkVelocity = FVector(NewHorizontalVelocity.X, NewHorizontalVelocity.Y, NewVerticalVelocity);
	if (WalkVelocity.IsNearlyZero(0.1f))
	{
		return;
	}

	FVector DeltaMove = WalkVelocity * DeltaTime;
	FHitResult Hit;
	AddActorWorldOffset(DeltaMove, bWalkSweepCollision, &Hit);

	if (Hit.bBlockingHit)
	{
		const FVector SlideDelta = FVector::VectorPlaneProject(DeltaMove, Hit.ImpactNormal);
		AddActorWorldOffset(SlideDelta, bWalkSweepCollision);
	}

	FVector ClampedLocation = GetActorLocation();
	ClampWalkLocation(ClampedLocation);
	SetActorLocation(ClampedLocation);
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
void AOrbitCameraManagerBase::SetOrbitLookPressed() { bOrbitLookPressed = true; }
void AOrbitCameraManagerBase::SetOrbitLookReleased() { bOrbitLookPressed = false; }
void AOrbitCameraManagerBase::SetOrbitPanPressed() { bOrbitPanPressed = true; }
void AOrbitCameraManagerBase::SetOrbitPanReleased() { bOrbitPanPressed = false; }

void AOrbitCameraManagerBase::ToggleDebugMenu()
{
	bDebugMenuEnabled = !bDebugMenuEnabled;
	if (!bDebugMenuEnabled && GEngine)
	{
		GEngine->ClearOnScreenDebugMessages();
	}
}

void AOrbitCameraManagerBase::ToggleDebugTraces()
{
	bDebugTracesEnabled = !bDebugTracesEnabled;
	if (ActiveOrbitCamera)
	{
		ActiveOrbitCamera->EnableDebugging = bDebugTracesEnabled;
	}
}

void AOrbitCameraManagerBase::ToggleDebugBounds()
{
	bDebugBoundsEnabled = !bDebugBoundsEnabled;
}

void AOrbitCameraManagerBase::ToggleDebugWalkState()
{
	bDebugWalkStateEnabled = !bDebugWalkStateEnabled;
}

void AOrbitCameraManagerBase::UpdateDebugOverlay(float DeltaTime)
{
	if (!bDebugMenuEnabled || !GEngine)
	{
		return;
	}

	const FString ActiveCamName = ActiveOrbitCamera ? ActiveOrbitCamera->GetName() : TEXT("None");
	const FString WalkCharName = WalkCharacterInstance ? WalkCharacterInstance->GetName() : TEXT("None");
	GEngine->AddOnScreenDebugMessage(1001, 0.0f, FColor::Cyan, FString::Printf(TEXT("[Orbit Debug] WalkMode: %s  Transition: %s"),
		bIsWalkOutMode ? TEXT("ON") : TEXT("OFF"),
		bTransitionInProgress ? TEXT("ON") : TEXT("OFF")));
	GEngine->AddOnScreenDebugMessage(1002, 0.0f, FColor::Cyan, FString::Printf(TEXT("[Orbit Debug] ActiveOrbitCamera: %s"), *ActiveCamName));
	GEngine->AddOnScreenDebugMessage(1003, 0.0f, FColor::Cyan, FString::Printf(TEXT("[Orbit Debug] WalkCharacter: %s"), *WalkCharName));
	GEngine->AddOnScreenDebugMessage(1004, 0.0f, FColor::Yellow, FString::Printf(TEXT("[Orbit Debug] Traces(F5): %s  Bounds(F6): %s  WalkState(F7): %s"),
		bDebugTracesEnabled ? TEXT("ON") : TEXT("OFF"),
		bDebugBoundsEnabled ? TEXT("ON") : TEXT("OFF"),
		bDebugWalkStateEnabled ? TEXT("ON") : TEXT("OFF")));

	if (bDebugWalkStateEnabled)
	{
		GEngine->AddOnScreenDebugMessage(1005, 0.0f, FColor::Green, FString::Printf(TEXT("[Orbit Debug] ManagerLoc: %s"), *GetActorLocation().ToCompactString()));
		if (WalkCharacterInstance)
		{
			GEngine->AddOnScreenDebugMessage(1006, 0.0f, FColor::Green, FString::Printf(TEXT("[Orbit Debug] WalkLoc: %s"), *WalkCharacterInstance->GetActorLocation().ToCompactString()));
		}
	}

	if (bDebugBoundsEnabled && ActiveOrbitCamera && ActiveOrbitCamera->CameraBoundsActor)
	{
		if (const UBoxComponent* Bounds = ActiveOrbitCamera->CameraBoundsActor->FindComponentByClass<UBoxComponent>())
		{
			DrawDebugBox(
				GetWorld(),
				Bounds->GetComponentLocation(),
				Bounds->GetScaledBoxExtent(),
				FColor::Blue,
				false,
				DeltaTime + 0.01f,
				0,
				2.0f);
		}
	}
}

void AOrbitCameraManagerBase::OnOrbitMouseWheel(float Value)
{
	if (bIsWalkOutMode || FMath::IsNearlyZero(Value) || !ActiveOrbitCamera || !ActiveOrbitCamera->CineCamRef)
	{
		return;
	}
	EnsureOrbitTargetsInitialized();

	const float DistanceMin = FMath::Min(ActiveOrbitCamera->MinDistance, ActiveOrbitCamera->MaxDistance);
	const float DistanceMax = FMath::Max(ActiveOrbitCamera->MinDistance, ActiveOrbitCamera->MaxDistance);
	OrbitTargetDistance = FMath::Clamp(OrbitTargetDistance - (Value * OrbitZoomStep), DistanceMin, DistanceMax);

	if (bOrbitZoomChangesFocalLength)
	{
		const float FocalMin = FMath::Min(ActiveOrbitCamera->MinFocalLength, ActiveOrbitCamera->MaxFocalLength);
		const float FocalMax = FMath::Max(ActiveOrbitCamera->MinFocalLength, ActiveOrbitCamera->MaxFocalLength);
		OrbitTargetFocalLength = FMath::Clamp(OrbitTargetFocalLength + (Value * OrbitZoomFocalStep), FocalMin, FocalMax);
	}
}

void AOrbitCameraManagerBase::OnLookYaw(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	if (bIsWalkOutMode)
	{
		FRotator Rot = GetActorRotation();
		Rot.Yaw += Value * WalkLookYawSpeed * DeltaSeconds;
		SetActorRotation(Rot);
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
		OrbitTargetRootLocation += RightMove;
		return;
	}

	if (!bOrbitLookPressed)
	{
		return;
	}

	FRotator OrbitRot = OrbitTargetRootRotation;
	const float YawMin = FMath::Min(ActiveOrbitCamera->MinYaw, ActiveOrbitCamera->MaxYaw);
	const float YawMax = FMath::Max(ActiveOrbitCamera->MinYaw, ActiveOrbitCamera->MaxYaw);
	OrbitRot.Yaw = FMath::Clamp(OrbitRot.Yaw + (Value * OrbitLookYawSpeed * DeltaSeconds), YawMin, YawMax);
	OrbitTargetRootRotation = OrbitRot;
}

void AOrbitCameraManagerBase::OnLookPitch(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	if (bIsWalkOutMode)
	{
		FRotator Rot = GetActorRotation();
		Rot.Pitch = FMath::Clamp(Rot.Pitch + (-Value * WalkLookPitchSpeed * DeltaSeconds), WalkMinPitch, WalkMaxPitch);
		SetActorRotation(Rot);
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
		OrbitTargetRootLocation += UpMove;
		return;
	}

	if (!bOrbitLookPressed)
	{
		return;
	}

	FRotator OrbitRot = OrbitTargetRootRotation;
	const float PitchMin = FMath::Min(ActiveOrbitCamera->MinPitch, ActiveOrbitCamera->MaxPitch);
	const float PitchMax = FMath::Max(ActiveOrbitCamera->MinPitch, ActiveOrbitCamera->MaxPitch);
	OrbitRot.Pitch = FMath::Clamp(OrbitRot.Pitch + (-Value * OrbitLookPitchSpeed * DeltaSeconds), PitchMin, PitchMax);
	OrbitTargetRootRotation = OrbitRot;
}
