#include "OrbitCameraPlayerController.h"

#include "OrbitCameraBase.h"
#include "OrbitCameraManagerBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"

AOrbitCameraPlayerController::AOrbitCameraPlayerController()
{
	bShowMouseCursor = true;
}

void AOrbitCameraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureDefaultInputMappings();
	EnsureReferences();

	if (OrbitCameraRef)
	{
		SetViewTargetWithBlend(OrbitCameraRef, 0.0f);
	}
}

void AOrbitCameraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindAction(OrbitDragActionName, IE_Pressed, this, &AOrbitCameraPlayerController::OnOrbitDragPressed);
	InputComponent->BindAction(OrbitDragActionName, IE_Released, this, &AOrbitCameraPlayerController::OnOrbitDragReleased);
	InputComponent->BindAction(PanDragActionName, IE_Pressed, this, &AOrbitCameraPlayerController::OnPanDragPressed);
	InputComponent->BindAction(PanDragActionName, IE_Released, this, &AOrbitCameraPlayerController::OnPanDragReleased);
	InputComponent->BindAction(ToggleWalkActionName, IE_Pressed, this, &AOrbitCameraPlayerController::OnToggleWalkPressed);
	InputComponent->BindAction(NextCameraActionName, IE_Pressed, this, &AOrbitCameraPlayerController::OnNextCameraPressed);
	InputComponent->BindAction(PreviousCameraActionName, IE_Pressed, this, &AOrbitCameraPlayerController::OnPreviousCameraPressed);

	InputComponent->BindAxis(OrbitYawAxisName, this, &AOrbitCameraPlayerController::OnOrbitYaw);
	InputComponent->BindAxis(OrbitPitchAxisName, this, &AOrbitCameraPlayerController::OnOrbitPitch);
	InputComponent->BindAxis(PanXAxisName, this, &AOrbitCameraPlayerController::OnPanX);
	InputComponent->BindAxis(PanYAxisName, this, &AOrbitCameraPlayerController::OnPanY);
	InputComponent->BindAxis(ZoomAxisName, this, &AOrbitCameraPlayerController::OnZoom);
}

void AOrbitCameraPlayerController::OnOrbitDragPressed()
{
	bOrbitDrag = true;
}

void AOrbitCameraPlayerController::OnOrbitDragReleased()
{
	bOrbitDrag = false;
}

void AOrbitCameraPlayerController::OnPanDragPressed()
{
	bPanDrag = true;
}

void AOrbitCameraPlayerController::OnPanDragReleased()
{
	bPanDrag = false;
}

void AOrbitCameraPlayerController::OnToggleWalkPressed()
{
	EnsureReferences();
	if (!OrbitManagerRef)
	{
		return;
	}

	if (OrbitManagerRef->ActiveMode == EOrbitCameraMode::Orbit)
	{
		OrbitManagerRef->EnterWalkingMode(true);
	}
	else
	{
		OrbitManagerRef->ExitWalkingMode(true);
	}
}

void AOrbitCameraPlayerController::OnNextCameraPressed()
{
	EnsureReferences();
	if (!OrbitManagerRef)
	{
		return;
	}

	if (OrbitManagerRef->ActiveMode == EOrbitCameraMode::Walking)
	{
		OrbitManagerRef->ExitWalkingMode(true);
	}

	OrbitManagerRef->TransitionToNextCamera(EOrbitCameraTransition::OC_Transition);
	OrbitCameraRef = OrbitManagerRef->ActiveOrbitCamera;
}

void AOrbitCameraPlayerController::OnPreviousCameraPressed()
{
	EnsureReferences();
	if (!OrbitManagerRef)
	{
		return;
	}

	if (OrbitManagerRef->ActiveMode == EOrbitCameraMode::Walking)
	{
		OrbitManagerRef->ExitWalkingMode(true);
	}

	OrbitManagerRef->TransitionToPreviousCamera(EOrbitCameraTransition::OC_Transition);
	OrbitCameraRef = OrbitManagerRef->ActiveOrbitCamera;
}

void AOrbitCameraPlayerController::OnOrbitYaw(float Value)
{
	if (!bOrbitDrag || FMath::IsNearlyZero(Value))
	{
		return;
	}

	EnsureReferences();
	if (OrbitCameraRef)
	{
		OrbitCameraRef->AddOrbitInput(Value, 0.0f);
	}
}

void AOrbitCameraPlayerController::OnOrbitPitch(float Value)
{
	if (!bOrbitDrag || FMath::IsNearlyZero(Value))
	{
		return;
	}

	EnsureReferences();
	if (OrbitCameraRef)
	{
		OrbitCameraRef->AddOrbitInput(0.0f, Value);
	}
}

void AOrbitCameraPlayerController::OnPanX(float Value)
{
	PanXValue = Value;
	if (!bPanDrag)
	{
		return;
	}

	EnsureReferences();
	if (OrbitCameraRef)
	{
		OrbitCameraRef->AddPanInput(PanXValue, PanYValue);
	}
}

void AOrbitCameraPlayerController::OnPanY(float Value)
{
	PanYValue = Value;
	if (!bPanDrag)
	{
		return;
	}

	EnsureReferences();
	if (OrbitCameraRef)
	{
		OrbitCameraRef->AddPanInput(PanXValue, PanYValue);
	}
}

void AOrbitCameraPlayerController::OnZoom(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	EnsureReferences();
	if (OrbitCameraRef)
	{
		OrbitCameraRef->AddZoomInput(Value);
	}
}

void AOrbitCameraPlayerController::EnsureReferences()
{
	if (!OrbitManagerRef)
	{
		OrbitManagerRef = Cast<AOrbitCameraManagerBase>(UGameplayStatics::GetActorOfClass(this, AOrbitCameraManagerBase::StaticClass()));
	}

	if (!OrbitCameraRef)
	{
		if (OrbitManagerRef && OrbitManagerRef->ActiveOrbitCamera)
		{
			OrbitCameraRef = OrbitManagerRef->ActiveOrbitCamera;
		}
		else
		{
			OrbitCameraRef = Cast<AOrbitCameraBase>(UGameplayStatics::GetActorOfClass(this, AOrbitCameraBase::StaticClass()));
		}
	}
}

void AOrbitCameraPlayerController::EnsureDefaultInputMappings()
{
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (!InputSettings)
	{
		return;
	}

	bool bMappingsChanged = false;

	if (bKeepLeftClickUnbound)
	{
		RemoveLeftMouseActionMapping(InputSettings, OrbitDragActionName, bMappingsChanged);
		RemoveLeftMouseActionMapping(InputSettings, PanDragActionName, bMappingsChanged);
		RemoveLeftMouseActionMapping(InputSettings, ToggleWalkActionName, bMappingsChanged);
		RemoveLeftMouseActionMapping(InputSettings, NextCameraActionName, bMappingsChanged);
		RemoveLeftMouseActionMapping(InputSettings, PreviousCameraActionName, bMappingsChanged);
	}

	// Orbit mode mappings
	AddDefaultActionMapping(InputSettings, OrbitDragActionName, FInputActionKeyMapping(OrbitDragActionName, EKeys::RightMouseButton), bMappingsChanged);
	AddDefaultActionMapping(InputSettings, PanDragActionName, FInputActionKeyMapping(PanDragActionName, EKeys::MiddleMouseButton), bMappingsChanged);
	AddDefaultActionMapping(InputSettings, ToggleWalkActionName, FInputActionKeyMapping(ToggleWalkActionName, EKeys::Tab), bMappingsChanged);
	AddDefaultActionMapping(InputSettings, NextCameraActionName, FInputActionKeyMapping(NextCameraActionName, EKeys::PageDown), bMappingsChanged);
	AddDefaultActionMapping(InputSettings, PreviousCameraActionName, FInputActionKeyMapping(PreviousCameraActionName, EKeys::PageUp), bMappingsChanged);

	AddDefaultAxisMapping(InputSettings, OrbitYawAxisName, FInputAxisKeyMapping(OrbitYawAxisName, EKeys::MouseX, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, OrbitPitchAxisName, FInputAxisKeyMapping(OrbitPitchAxisName, EKeys::MouseY, -1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, PanXAxisName, FInputAxisKeyMapping(PanXAxisName, EKeys::MouseX, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, PanYAxisName, FInputAxisKeyMapping(PanYAxisName, EKeys::MouseY, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, ZoomAxisName, FInputAxisKeyMapping(ZoomAxisName, EKeys::MouseWheelAxis, 1.0f), bMappingsChanged);

	// Walking mode mappings
	AddDefaultAxisMapping(InputSettings, TEXT("MoveForward"), FInputAxisKeyMapping(TEXT("MoveForward"), EKeys::W, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("MoveForward"), FInputAxisKeyMapping(TEXT("MoveForward"), EKeys::S, -1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("MoveRight"), FInputAxisKeyMapping(TEXT("MoveRight"), EKeys::D, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("MoveRight"), FInputAxisKeyMapping(TEXT("MoveRight"), EKeys::A, -1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("MoveUp"), FInputAxisKeyMapping(TEXT("MoveUp"), EKeys::E, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("MoveUp"), FInputAxisKeyMapping(TEXT("MoveUp"), EKeys::Q, -1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("Turn"), FInputAxisKeyMapping(TEXT("Turn"), EKeys::MouseX, 1.0f), bMappingsChanged);
	AddDefaultAxisMapping(InputSettings, TEXT("LookUp"), FInputAxisKeyMapping(TEXT("LookUp"), EKeys::MouseY, -1.0f), bMappingsChanged);

	AddDefaultActionMapping(InputSettings, TEXT("WalkSprint"), FInputActionKeyMapping(TEXT("WalkSprint"), EKeys::LeftShift), bMappingsChanged);
	AddDefaultActionMapping(InputSettings, TEXT("WalkSlow"), FInputActionKeyMapping(TEXT("WalkSlow"), EKeys::LeftControl), bMappingsChanged);

	if (bMappingsChanged)
	{
		InputSettings->SaveKeyMappings();
		InputSettings->ForceRebuildKeymaps();
	}
}

void AOrbitCameraPlayerController::AddDefaultActionMapping(UInputSettings* InputSettings, const FName& ActionName, const FInputActionKeyMapping& Mapping, bool& bMappingsChanged) const
{
	if (!InputSettings)
	{
		return;
	}

	TArray<FInputActionKeyMapping> ExistingMappings;
	InputSettings->GetActionMappingByName(ActionName, ExistingMappings);
	const bool bExists = ExistingMappings.ContainsByPredicate([&Mapping](const FInputActionKeyMapping& Existing)
	{
		return Existing.Key == Mapping.Key
			&& Existing.bShift == Mapping.bShift
			&& Existing.bCtrl == Mapping.bCtrl
			&& Existing.bAlt == Mapping.bAlt
			&& Existing.bCmd == Mapping.bCmd;
	});

	if (!bExists)
	{
		InputSettings->AddActionMapping(Mapping, false);
		bMappingsChanged = true;
	}
}

void AOrbitCameraPlayerController::AddDefaultAxisMapping(UInputSettings* InputSettings, const FName& AxisName, const FInputAxisKeyMapping& Mapping, bool& bMappingsChanged) const
{
	if (!InputSettings)
	{
		return;
	}

	TArray<FInputAxisKeyMapping> ExistingMappings;
	InputSettings->GetAxisMappingByName(AxisName, ExistingMappings);
	const bool bExists = ExistingMappings.ContainsByPredicate([&Mapping](const FInputAxisKeyMapping& Existing)
	{
		return Existing.Key == Mapping.Key && FMath::IsNearlyEqual(Existing.Scale, Mapping.Scale);
	});

	if (!bExists)
	{
		InputSettings->AddAxisMapping(Mapping, false);
		bMappingsChanged = true;
	}
}

void AOrbitCameraPlayerController::RemoveLeftMouseActionMapping(UInputSettings* InputSettings, const FName& ActionName, bool& bMappingsChanged) const
{
	if (!InputSettings)
	{
		return;
	}

	TArray<FInputActionKeyMapping> ExistingMappings;
	InputSettings->GetActionMappingByName(ActionName, ExistingMappings);
	for (const FInputActionKeyMapping& Existing : ExistingMappings)
	{
		if (Existing.Key == EKeys::LeftMouseButton)
		{
			InputSettings->RemoveActionMapping(Existing, false);
			bMappingsChanged = true;
		}
	}
}
