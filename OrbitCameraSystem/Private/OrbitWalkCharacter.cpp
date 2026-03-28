#include "OrbitWalkCharacter.h"

#include "OrbitCameraManagerBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AOrbitWalkCharacter::AOrbitWalkCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	WalkCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("WalkCamera"));
	WalkCamera->SetupAttachment(GetCapsuleComponent());
	WalkCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	WalkCamera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AOrbitWalkCharacter::InitializeFromManager(
	AOrbitCameraManagerBase* InManager,
	float InWalkSpeed,
	float InSprintMultiplier,
	float InMinPitch,
	float InMaxPitch,
	bool bInAllowVerticalMovement,
	FKey InToggleWalkOutKey,
	FKey InMoveForwardKey,
	FKey InMoveBackwardKey,
	FKey InMoveRightKey,
	FKey InMoveLeftKey,
	FKey InMoveUpKey,
	FKey InMoveDownKey,
	FKey InSprintKey)
{
	OwningManager = InManager;
	WalkSpeed = InWalkSpeed;
	SprintMultiplier = InSprintMultiplier;
	MinPitch = InMinPitch;
	MaxPitch = InMaxPitch;
	bAllowVerticalMovement = bInAllowVerticalMovement;

	ToggleWalkOutKey = InToggleWalkOutKey;
	MoveForwardKey = InMoveForwardKey;
	MoveBackwardKey = InMoveBackwardKey;
	MoveRightKey = InMoveRightKey;
	MoveLeftKey = InMoveLeftKey;
	MoveUpKey = InMoveUpKey;
	MoveDownKey = InMoveDownKey;
	SprintKey = InSprintKey;

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MovementMode = bAllowVerticalMovement ? EMovementMode::MOVE_Flying : EMovementMode::MOVE_Walking;
}

void AOrbitWalkCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindKey(ToggleWalkOutKey, IE_Pressed, this, &AOrbitWalkCharacter::ToggleWalkModeFromCharacter);

	PlayerInputComponent->BindKey(MoveForwardKey, IE_Pressed, this, &AOrbitWalkCharacter::SetMoveForwardPressed);
	PlayerInputComponent->BindKey(MoveForwardKey, IE_Released, this, &AOrbitWalkCharacter::SetMoveForwardReleased);
	PlayerInputComponent->BindKey(MoveBackwardKey, IE_Pressed, this, &AOrbitWalkCharacter::SetMoveBackwardPressed);
	PlayerInputComponent->BindKey(MoveBackwardKey, IE_Released, this, &AOrbitWalkCharacter::SetMoveBackwardReleased);
	PlayerInputComponent->BindKey(MoveRightKey, IE_Pressed, this, &AOrbitWalkCharacter::SetMoveRightPressed);
	PlayerInputComponent->BindKey(MoveRightKey, IE_Released, this, &AOrbitWalkCharacter::SetMoveRightReleased);
	PlayerInputComponent->BindKey(MoveLeftKey, IE_Pressed, this, &AOrbitWalkCharacter::SetMoveLeftPressed);
	PlayerInputComponent->BindKey(MoveLeftKey, IE_Released, this, &AOrbitWalkCharacter::SetMoveLeftReleased);
	PlayerInputComponent->BindKey(MoveUpKey, IE_Pressed, this, &AOrbitWalkCharacter::SetMoveUpPressed);
	PlayerInputComponent->BindKey(MoveUpKey, IE_Released, this, &AOrbitWalkCharacter::SetMoveUpReleased);
	PlayerInputComponent->BindKey(MoveDownKey, IE_Pressed, this, &AOrbitWalkCharacter::SetMoveDownPressed);
	PlayerInputComponent->BindKey(MoveDownKey, IE_Released, this, &AOrbitWalkCharacter::SetMoveDownReleased);
	PlayerInputComponent->BindKey(SprintKey, IE_Pressed, this, &AOrbitWalkCharacter::SetSprintPressed);
	PlayerInputComponent->BindKey(SprintKey, IE_Released, this, &AOrbitWalkCharacter::SetSprintReleased);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AOrbitWalkCharacter::OnLookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AOrbitWalkCharacter::OnLookPitch);
}

void AOrbitWalkCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMovementInput();
}

void AOrbitWalkCharacter::SetMoveForwardPressed() { bMoveForwardPressed = true; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveForwardReleased() { bMoveForwardPressed = false; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveBackwardPressed() { bMoveBackwardPressed = true; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveBackwardReleased() { bMoveBackwardPressed = false; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveRightPressed() { bMoveRightPressed = true; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveRightReleased() { bMoveRightPressed = false; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveLeftPressed() { bMoveLeftPressed = true; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveLeftReleased() { bMoveLeftPressed = false; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveUpPressed() { bMoveUpPressed = true; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveUpReleased() { bMoveUpPressed = false; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveDownPressed() { bMoveDownPressed = true; UpdateMovementInput(); }
void AOrbitWalkCharacter::SetMoveDownReleased() { bMoveDownPressed = false; UpdateMovementInput(); }

void AOrbitWalkCharacter::SetSprintPressed()
{
	bSprintPressed = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * SprintMultiplier;
}

void AOrbitWalkCharacter::SetSprintReleased()
{
	bSprintPressed = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AOrbitWalkCharacter::ToggleWalkModeFromCharacter()
{
	if (OwningManager)
	{
		OwningManager->ToggleWalkOutMode();
	}
}

void AOrbitWalkCharacter::OnLookYaw(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddControllerYawInput(Value);
}

void AOrbitWalkCharacter::OnLookPitch(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddControllerPitchInput(-Value);
	if (Controller)
	{
		FRotator Rot = Controller->GetControlRotation();
		Rot.Pitch = FMath::ClampAngle(Rot.Pitch, MinPitch, MaxPitch);
		Controller->SetControlRotation(Rot);
	}
}

void AOrbitWalkCharacter::UpdateMovementInput()
{
	const float ForwardValue = (bMoveForwardPressed ? 1.0f : 0.0f) - (bMoveBackwardPressed ? 1.0f : 0.0f);
	const float RightValue = (bMoveRightPressed ? 1.0f : 0.0f) - (bMoveLeftPressed ? 1.0f : 0.0f);
	const float UpValue = (bMoveUpPressed ? 1.0f : 0.0f) - (bMoveDownPressed ? 1.0f : 0.0f);

	if (!FMath::IsNearlyZero(ForwardValue))
	{
		AddMovementInput(GetActorForwardVector(), ForwardValue);
	}
	if (!FMath::IsNearlyZero(RightValue))
	{
		AddMovementInput(GetActorRightVector(), RightValue);
	}
	if (bAllowVerticalMovement && !FMath::IsNearlyZero(UpValue))
	{
		AddMovementInput(FVector::UpVector, UpValue);
	}
}
