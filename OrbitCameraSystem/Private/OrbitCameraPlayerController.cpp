#include "OrbitCameraPlayerController.h"

#include "OrbitCameraBase.h"
#include "OrbitCameraManagerBase.h"
#include "Kismet/GameplayStatics.h"

AOrbitCameraPlayerController::AOrbitCameraPlayerController()
{
	bShowMouseCursor = true;
}

void AOrbitCameraPlayerController::BeginPlay()
{
	Super::BeginPlay();
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
