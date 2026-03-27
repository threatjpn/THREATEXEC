#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OrbitCameraManagerBase.h"
#include "OrbitCameraPlayerController.generated.h"

class AOrbitCameraBase;
class AOrbitCameraManagerBase;
class UInputSettings;

UCLASS(Blueprintable)
class ORBITCAMERASYSTEM_API AOrbitCameraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AOrbitCameraPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName OrbitDragActionName = TEXT("OrbitDrag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName PanDragActionName = TEXT("PanDrag");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName ToggleWalkActionName = TEXT("ToggleWalk");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName NextCameraActionName = TEXT("NextOrbitCamera");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName PreviousCameraActionName = TEXT("PreviousOrbitCamera");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName OrbitYawAxisName = TEXT("OrbitYaw");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName OrbitPitchAxisName = TEXT("OrbitPitch");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName PanXAxisName = TEXT("PanX");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName PanYAxisName = TEXT("PanY");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	FName ZoomAxisName = TEXT("Zoom");

	// Keeps Left Mouse Button unbound from orbit-controller default actions so gameplay/UI click remains free.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	bool bKeepLeftClickUnbound = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	TObjectPtr<AOrbitCameraBase> OrbitCameraRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	TObjectPtr<AOrbitCameraManagerBase> OrbitManagerRef = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	bool bOrbitDrag = false;
	bool bPanDrag = false;
	float PanXValue = 0.0f;
	float PanYValue = 0.0f;

	void OnOrbitDragPressed();
	void OnOrbitDragReleased();
	void OnPanDragPressed();
	void OnPanDragReleased();
	void OnToggleWalkPressed();
	void OnNextCameraPressed();
	void OnPreviousCameraPressed();

	void OnOrbitYaw(float Value);
	void OnOrbitPitch(float Value);
	void OnPanX(float Value);
	void OnPanY(float Value);
	void OnZoom(float Value);

	void EnsureReferences();
	void EnsureDefaultInputMappings();
	void AddDefaultActionMapping(UInputSettings* InputSettings, const FName& ActionName, const struct FInputActionKeyMapping& Mapping, bool& bMappingsChanged) const;
	void AddDefaultAxisMapping(UInputSettings* InputSettings, const FName& AxisName, const struct FInputAxisKeyMapping& Mapping, bool& bMappingsChanged) const;
	void RemoveLeftMouseActionMapping(UInputSettings* InputSettings, const FName& ActionName, bool& bMappingsChanged) const;
};
