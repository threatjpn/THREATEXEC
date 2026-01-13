// BezierDebugHUD.h : Minimal runtime overlay for Bezier debug toggles
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BezierDebugHUD.generated.h"

class ABezierDebugActor;

UCLASS()
class THREATEXEC_API ABezierDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABezierDebugHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

protected:
	UPROPERTY(EditAnywhere, Category="Bezier|Debug")
	bool bShowOverlay = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug")
	float LineHeight = 16.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug")
	float TextScale = 1.0f;

private:
	UPROPERTY()
	TWeakObjectPtr<ABezierDebugActor> DebugActor;

	void BindInput();
	void ToggleOverlay();
	void ToggleEditMode();
	void ToggleControlPoints();
	void ToggleStrip();
	void ToggleSnapToGrid();
	void ToggleShowGrid();
	void CycleGridSize();
	void ToggleLockToXY();
	void ToggleForcePlanar();
	void TogglePulseDebug();
	void TogglePulseControlPoints();
	void TogglePulseStrip();
	void ToggleMouseTraceDebug();

	void ApplyAndRefresh();
	ABezierDebugActor* ResolveDebugActor();
	void DrawLineText(float& Y, const FString& Text);
	void DrawRightAlignedText(float& Y, const FString& Text);
};
