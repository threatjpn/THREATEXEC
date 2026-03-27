#include "OrbitWalkingPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"

AOrbitWalkingPawn::AOrbitWalkingPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	SetRootComponent(CapsuleComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(CapsuleComponent);
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	MovementComponent->MaxSpeed = MoveSpeed;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

UPawnMovementComponent* AOrbitWalkingPawn::GetMovementComponent() const
{
	return MovementComponent;
}

void AOrbitWalkingPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AOrbitWalkingPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AOrbitWalkingPawn::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &AOrbitWalkingPawn::MoveUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AOrbitWalkingPawn::LookYaw);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AOrbitWalkingPawn::LookPitchInput);
	PlayerInputComponent->BindAction(TEXT("WalkSprint"), IE_Pressed, this, &AOrbitWalkingPawn::OnSprintPressed);
	PlayerInputComponent->BindAction(TEXT("WalkSprint"), IE_Released, this, &AOrbitWalkingPawn::OnSprintReleased);
	PlayerInputComponent->BindAction(TEXT("WalkSlow"), IE_Pressed, this, &AOrbitWalkingPawn::OnSlowPressed);
	PlayerInputComponent->BindAction(TEXT("WalkSlow"), IE_Released, this, &AOrbitWalkingPawn::OnSlowReleased);
}

void AOrbitWalkingPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MovementComponent)
	{
		const float SpeedScale = bSprintActive ? SprintMultiplier : (bSlowActive ? SlowMultiplier : 1.0f);
		MovementComponent->MaxSpeed = MoveSpeed * FMath::Max(0.0f, SpeedScale);
	}

	ClampActorToBounds();
}

void AOrbitWalkingPawn::MoveForward(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddMovementInput(GetActorForwardVector(), Value);
}

void AOrbitWalkingPawn::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddMovementInput(GetActorRightVector(), Value);
}

void AOrbitWalkingPawn::LookYaw(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddControllerYawInput(Value * LookSensitivityYaw);
}

void AOrbitWalkingPawn::LookPitchInput(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	LookPitch = FMath::Clamp(LookPitch + (Value * LookSensitivityPitch), MinPitch, MaxPitch);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(FRotator(LookPitch, PC->GetControlRotation().Yaw, 0.0f));
	}
}

void AOrbitWalkingPawn::MoveUp(float Value)
{
	if (!bEnableVerticalMovement || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FVector VerticalVector = bUseCameraRelativeVerticalMovement && CameraComponent
		? CameraComponent->GetUpVector()
		: FVector::UpVector;

	AddMovementInput(VerticalVector, Value);
}

void AOrbitWalkingPawn::OnSprintPressed()
{
	SetSprintActive(true);
}

void AOrbitWalkingPawn::OnSprintReleased()
{
	SetSprintActive(false);
}

void AOrbitWalkingPawn::OnSlowPressed()
{
	SetSlowWalkActive(true);
}

void AOrbitWalkingPawn::OnSlowReleased()
{
	SetSlowWalkActive(false);
}

void AOrbitWalkingPawn::SetSprintActive(bool bActive)
{
	bSprintActive = bActive;
	if (bSprintActive)
	{
		bSlowActive = false;
	}
}

void AOrbitWalkingPawn::SetSlowWalkActive(bool bActive)
{
	bSlowActive = bActive;
	if (bSlowActive)
	{
		bSprintActive = false;
	}
}


void AOrbitWalkingPawn::ConfigureMovementBounds(bool bEnableBounds, AActor* InBoundsActor, float InBoundsPadding)
{
	bClampToBounds = bEnableBounds;
	BoundsActor = InBoundsActor;
	BoundsPadding = FMath::Max(0.0f, InBoundsPadding);
}

void AOrbitWalkingPawn::ClampActorToBounds()
{
	if (!bClampToBounds || !BoundsActor)
	{
		return;
	}

	const UBoxComponent* BoundsBox = BoundsActor->FindComponentByClass<UBoxComponent>();
	if (!BoundsBox)
	{
		return;
	}

	const FVector Center = BoundsBox->GetComponentLocation();
	FVector Extent = BoundsBox->GetScaledBoxExtent() - FVector(BoundsPadding);
	Extent.X = FMath::Max(0.0f, Extent.X);
	Extent.Y = FMath::Max(0.0f, Extent.Y);
	Extent.Z = FMath::Max(0.0f, Extent.Z);

	const FVector Current = GetActorLocation();
	const FVector Clamped(
		FMath::Clamp(Current.X, Center.X - Extent.X, Center.X + Extent.X),
		FMath::Clamp(Current.Y, Center.Y - Extent.Y, Center.Y + Extent.Y),
		FMath::Clamp(Current.Z, Center.Z - Extent.Z, Center.Z + Extent.Z));

	if (!Current.Equals(Clamped, 0.01f))
	{
		SetActorLocation(Clamped, false, nullptr, ETeleportType::TeleportPhysics);
	}
}
