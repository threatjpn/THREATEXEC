#include "BezierDebugHUD.h"

#include "BezierDebugActor.h"
#include "BezierEditPlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	static const TCHAR* TE_VisualPriorityToText(const EBezierVisualPriority Priority)
	{
		switch (Priority)
		{
		case EBezierVisualPriority::World: return TEXT("WORLD");
		case EBezierVisualPriority::Foreground: return TEXT("FOREGROUND");
		case EBezierVisualPriority::Overlay: return TEXT("OVERLAY");
		default: return TEXT("UNKNOWN");
		}
	}
}

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

	ABezierDebugActor* Debug = ResolveAndSyncDebugActor();
	if (!Debug)
	{
		DrawLineText(/*Y*/LineHeight, TEXT("Bezier Debug HUD: No BezierDebugActor found."));
		return;
	}

	float Y = 20.0f;
	DrawLineText(Y, TEXT("Bezier Debug HUD (F7 to toggle overlay)"));
	DrawLineText(Y, FString::Printf(TEXT("E: EditMode [%s]"), Debug->bEnableEditMode ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("V: Actor Visible [%s]"), Debug->bActorVisibleInGame ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("B: Hide Visuals When Not Editing [%s]"), Debug->bHideVisualsWhenNotEditing ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("C: Control Points [%s]"), Debug->bShowControlPoints ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("S: Strip [%s]"), Debug->bShowStrip ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("O: Control Polygon / Debug Lines [%s]"), Debug->bShowControlPolygon ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("M: Sample Points [%s]"), Debug->bShowSamplePoints ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("J: De Casteljau Levels [%s]"), Debug->bShowDeCasteljauLevels ? TEXT("ON") : TEXT("OFF")));
	const bool bGridVisible = Debug->bShowGrid || Debug->bSnapToGrid;
	DrawLineText(Y, FString::Printf(TEXT("G: Show Grid [%s]"), bGridVisible ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("N: Snap To Grid [%s]"), Debug->bSnapToGrid ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("H: Cycle Grid Size (Current %.1f)"), Debug->GridSizeCm));
	DrawLineText(Y, FString::Printf(TEXT("L: Lock To XY [%s]"), Debug->bLockToLocalXY ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("P: Force Planar [%s]"), Debug->bForcePlanar ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("D: Pulse Debug Lines [%s]"), Debug->bPulseDebugLines ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("U: Pulse Control Points [%s]"), Debug->bPulseControlPoints ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("I: Pulse Strip [%s]"), Debug->bPulseStrip ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("Y: Override Pulse Settings [%s]"), Debug->bOverridePulseSettings ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("T: Trace Debug [%s]"), Debug->bEnableMouseTraceDebug ? TEXT("ON") : TEXT("OFF")));
	DrawLineText(Y, FString::Printf(TEXT("F10: Visual Priority [%s] (Bias %d)"), TE_VisualPriorityToText(Debug->VisualPriority), Debug->VisualPriorityBias));
	DrawLineText(Y, FString::Printf(TEXT("Undo Steps: %d  | Ctrl+Z / Ctrl+Y"), Debug->UndoMaxSteps));
	DrawLineText(Y, TEXT("K: Apply Debug Settings"));

	if (Debug->bEnableMouseTraceDebug)
	{
		if (const ABezierEditPlayerController* EditPC = Cast<ABezierEditPlayerController>(PlayerOwner))
		{
			const FString Message = EditPC->GetDebugLastMessage();
			if (!Message.IsEmpty())
			{
				float RightY = 20.0f;
				DrawRightAlignedText(RightY, Message);
			}
		}
	}
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

	InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &ABezierDebugHUD::ToggleOverlay);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ABezierDebugHUD::ToggleEditMode);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ABezierDebugHUD::ToggleControlPoints);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ABezierDebugHUD::ToggleStrip);
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ABezierDebugHUD::ToggleShowGrid);
	InputComponent->BindKey(EKeys::N, IE_Pressed, this, &ABezierDebugHUD::ToggleSnapToGrid);
	InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ABezierDebugHUD::CycleGridSize);
	InputComponent->BindKey(EKeys::L, IE_Pressed, this, &ABezierDebugHUD::ToggleLockToXY);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ABezierDebugHUD::ToggleForcePlanar);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ABezierDebugHUD::TogglePulseDebug);
	InputComponent->BindKey(EKeys::U, IE_Pressed, this, &ABezierDebugHUD::TogglePulseControlPoints);
	InputComponent->BindKey(EKeys::I, IE_Pressed, this, &ABezierDebugHUD::TogglePulseStrip);
	// Bind plain Y debug toggle, but never consume the key so Ctrl+Y can still reach redo handlers.
	FInputKeyBinding OverridePulseChord(FInputChord(EKeys::Y, false, false, false, false), IE_Pressed);
	OverridePulseChord.KeyDelegate.BindDelegate(this, &ABezierDebugHUD::ToggleOverridePulseSettings);
	OverridePulseChord.bConsumeInput = false;
	InputComponent->KeyBindings.Add(MoveTemp(OverridePulseChord));
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ABezierDebugHUD::ToggleMouseTraceDebug);
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ABezierDebugHUD::ToggleActorVisible);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ABezierDebugHUD::ToggleHideWhenNotEditing);
	InputComponent->BindKey(EKeys::O, IE_Pressed, this, &ABezierDebugHUD::ToggleControlPolygon);
	InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ABezierDebugHUD::ToggleShowSamplePoints);
	InputComponent->BindKey(EKeys::J, IE_Pressed, this, &ABezierDebugHUD::ToggleShowDeCasteljauLevels);
	InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &ABezierDebugHUD::CycleVisualPriority);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ABezierDebugHUD::ApplyAndRefresh);
}

void ABezierDebugHUD::ToggleOverlay()
{
	bShowOverlay = !bShowOverlay;
}

void ABezierDebugHUD::ToggleEditMode()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bEnableEditMode = !Debug->bEnableEditMode;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleControlPoints()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bShowControlPoints = !Debug->bShowControlPoints;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleStrip()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bShowStrip = !Debug->bShowStrip;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleSnapToGrid()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bSnapToGrid = !Debug->bSnapToGrid;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleShowGrid()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bShowGrid = !Debug->bShowGrid;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::CycleGridSize()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		static const TArray<float> GridSizes = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
		int32 NextIndex = 0;
		for (int32 i = 0; i < GridSizes.Num(); ++i)
		{
			if (FMath::IsNearlyEqual(Debug->GridSizeCm, GridSizes[i], 0.01f))
			{
				NextIndex = (i + 1) % GridSizes.Num();
				break;
			}
		}

		Debug->GridSizeCm = GridSizes[NextIndex];
		Debug->bShowGrid = true;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleLockToXY()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bLockToLocalXY = !Debug->bLockToLocalXY;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleForcePlanar()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bForcePlanar = !Debug->bForcePlanar;
		if (Debug->bForcePlanar && Debug->ForcePlanarAxis == EBezierPlanarAxis::None)
		{
			Debug->ForcePlanarAxis = EBezierPlanarAxis::XY;
		}
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::TogglePulseDebug()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bOverridePulseSettings = true;
		Debug->bPulseDebugLines = !Debug->bPulseDebugLines;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::TogglePulseControlPoints()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bOverridePulseSettings = true;
		Debug->bPulseControlPoints = !Debug->bPulseControlPoints;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::TogglePulseStrip()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bOverridePulseSettings = true;
		Debug->bPulseStrip = !Debug->bPulseStrip;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleOverridePulseSettings()
{
	if (PlayerOwner && (PlayerOwner->IsInputKeyDown(EKeys::LeftControl) || PlayerOwner->IsInputKeyDown(EKeys::RightControl)))
	{
		// Avoid conflicting with Ctrl+Y redo input.
		return;
	}

	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bOverridePulseSettings = !Debug->bOverridePulseSettings;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleMouseTraceDebug()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bEnableMouseTraceDebug = !Debug->bEnableMouseTraceDebug;
		ApplyAndRefresh();
	}
}


void ABezierDebugHUD::ToggleActorVisible()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bActorVisibleInGame = !Debug->bActorVisibleInGame;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleHideWhenNotEditing()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bHideVisualsWhenNotEditing = !Debug->bHideVisualsWhenNotEditing;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleControlPolygon()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bShowControlPolygon = !Debug->bShowControlPolygon;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleShowSamplePoints()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bShowSamplePoints = !Debug->bShowSamplePoints;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::ToggleShowDeCasteljauLevels()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		Debug->bShowDeCasteljauLevels = !Debug->bShowDeCasteljauLevels;
		ApplyAndRefresh();
	}
}

void ABezierDebugHUD::CycleVisualPriority()
{
	if (ABezierDebugActor* Debug = ResolveAndSyncDebugActor())
	{
		switch (Debug->VisualPriority)
		{
		case EBezierVisualPriority::World:
			Debug->VisualPriority = EBezierVisualPriority::Foreground;
			break;
		case EBezierVisualPriority::Foreground:
			Debug->VisualPriority = EBezierVisualPriority::Overlay;
			break;
		case EBezierVisualPriority::Overlay:
		default:
			Debug->VisualPriority = EBezierVisualPriority::World;
			break;
		}
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
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown)
	{
		DebugActor = nullptr;
		return nullptr;
	}

	if (DebugActor.IsValid() && DebugActor->GetWorld() == World)
	{
		return DebugActor.Get();
	}

	DebugActor = nullptr;

	for (TActorIterator<ABezierDebugActor> It(World); It; ++It)
	{
		DebugActor = *It;
		return DebugActor.Get();
	}

	return nullptr;
}

ABezierDebugActor* ABezierDebugHUD::ResolveAndSyncDebugActor()
{
	if (ABezierDebugActor* Debug = ResolveDebugActor())
	{
		if (UWorld* World = Debug->GetWorld(); IsValid(World) && !World->bIsTearingDown)
		{
			Debug->SyncFromWorldState();
		}
		return Debug;
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

void ABezierDebugHUD::DrawRightAlignedText(float& Y, const FString& Text)
{
	if (!Canvas || !GEngine) return;

	const UFont* Font = GEngine->GetSmallFont();
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(Font, Text, TextWidth, TextHeight);

	TextWidth *= TextScale;
	TextHeight *= TextScale;

	const float X = FMath::Max(0.0f, Canvas->SizeX - TextWidth - 20.0f);
	const FVector2D Pos(X, Y);
	Canvas->SetDrawColor(TextColor.ToFColor(true));
	Canvas->DrawText(Font, Text, Pos.X, Pos.Y, TextScale, TextScale);
	Y += LineHeight;
}
