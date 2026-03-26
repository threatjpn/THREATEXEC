// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraBase.h"

//Unreal includes
#include "Engine/Selection.h"
#include "CineCameraComponent.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"

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
	ClampOrbitRootToBounds();
}

void AOrbitCameraBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClampOrbitRootToBounds();
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
