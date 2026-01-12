#include "BezierDebugHUD.h"

#include "BezierDebugActor.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

ABezierDebugHUD::ABezierDebugHUD()
{
}

void ABezierDebugHUD::BeginPlay()
{
	Super::BeginPlay();
	BindInput();
}

void ABezierDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowOverlay || !Canvas)
	{
		return;
	}

	ABezierDebugActor* Debug = ResolveDebugActor();
	if (!Debug)
	{
		DrawLineText(/*Y*/LineHeight, TEXT("Bezier Debug HUD: No BezierDebugActor found."));
		return;
	}

	float Y = 20.0f;
	DrawLineText(Y, TEXT("Bezier Debug HUD (F1 to toggle overlay)"));
	DrawLineText(Y, FString::Printf(TEXT("E: EditMode [%s]"), Debug->bEnableEditMode ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("C: Control Points [%s]"), Debug->bShowControlPoints ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("S: Strip [%s]"), Debug->bShowStrip ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("G: Show Grid [%s]"), Debug->bShowGrid ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("N: Snap To Grid [%s]"), Debug->bSnapToGrid ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("H: Cycle Grid Size (Current %.1f)"), Debug->GridSizeCm));
	DrawLineText(Y, FString::Printf(TEXT("L: Lock To XY [%s]"), Debug->bLockToLocalXY ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("P: Force Planar [%s]"), Debug->bForcePlanar ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("D: Pulse Debug Lines [%s]"), Debug->bPulseDebugLines ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("T: Trace Debug [%s]"), Debug->bEnableMouseTraceDebug ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, TEXT("K: Apply Debug Settings"));
}

void ABezierDebugHUD::BindInput()
{
	if (!PlayerOwner)
	{
		return;
	}

	EnableInput(PlayerOwner);
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ABezierDebugHUD::ToggleOverlay);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ABezierDebugHUD::ToggleEditMode);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ABezierDebugHUD::ToggleControlPoints);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ABezierDebugHUD::ToggleStrip);
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ABezierDebugHUD::ToggleShowGrid);
	InputComponent->BindKey(EKeys::N, IE_Pressed, this, &ABezierDebugHUD::ToggleSnapToGrid);
	InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ABezierDebugHUD::CycleGridSize);
	InputComponent->BindKey(EKeys::L, IE_Pressed, this, &ABezierDebugHUD::ToggleLockToXY);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ABezierDebugHUD::ToggleForcePlanar);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ABezierDebugHUD::TogglePulseDebug);
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ABezierDebugHUD::ToggleMouseTraceDebug);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ABezierDebugHUD::ApplyAndRefresh);
}

void ABezierDebugHUD::ToggleOverlay()
{
	bShowOverlay = !bShowOverlay;
}

void ABezierDebugHUD::ToggleEditMode()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bEnableEditMode = !Debug->bEnableEditMode;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleControlPoints()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bShowControlPoints = !Debug->bShowControlPoints;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleStrip()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bShowStrip = !Debug->bShowStrip;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleSnapToGrid()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bSnapToGrid = !Debug->bSnapToGrid;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleShowGrid()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bShowGrid = !Debug->bShowGrid;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::CycleGridSize()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->GridSizeCm = FMath::Max(0.1f, Debug->GridSizeCm + 5.0f);
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleLockToXY()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bLockToLocalXY = !Debug->bLockToLocalXY;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleForcePlanar()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bForcePlanar = !Debug->bForcePlanar;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::TogglePulseDebug()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bPulseDebugLines = !Debug->bPulseDebugLines;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleMouseTraceDebug()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->bEnableMouseTraceDebug = !Debug->bEnableMouseTraceDebug;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ApplyAndRefresh()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		Debug->ApplyDebugSettings();
	}
}

ABezierDebugActor* ABezierDebugHUD::ResolveDebugActor()
{
	if (DebugActor.IsValid())
	{
		return DebugActor.Get();
	}

	if (!GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<ABezierDebugActor> It(GetWorld()); It; ++It)
	{
		DebugActor = *It;
		return DebugActor.Get();
	}

	return nullptr;
}

void ABezierDebugHUD::DrawLineText(float& Y, const FString& Text)
{
	const FVector2D Pos(20.0f, Y);
	Canvas->SetDrawColor(TextColor.ToFColor(true));
	Canvas->DrawText(GEngine->GetSmallFont(), Text, Pos.X, Pos.Y, TextScale, TextScale);
	Y += LineHeight;
}
