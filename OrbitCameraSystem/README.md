# OrbitCameraSystem (Plugin README)

> This README is specific to the `OrbitCameraSystem` plugin/module and is intentionally separate from the project-level README.

## What this plugin provides

`OrbitCameraSystem` adds three primary runtime classes:

- `AOrbitCameraBase` - orbit/cinematic camera actor with runtime smoothing, transitions, DOF focus behaviors, and optional bounds clamping.
- `AOrbitCameraManagerBase` - lightweight manager pawn for switching between orbit cameras and walking mode.
- `AOrbitWalkingPawn` - simple movement pawn for walk/fly style exploration with optional bounds clamping.

---

## Quick setup (Unreal Editor)

### 1) Add the plugin/module to your project
Place this plugin under:

- `YourProject/Plugins/<YourPluginName>/Source/OrbitCameraSystem`

Then regenerate project files and build the project.

### 2) Place an orbit camera actor
In your level, add a Blueprint child of `AOrbitCameraBase` (recommended) or place `AOrbitCameraBase` directly.

### 3) (Optional) Add camera bounds
If you want camera clamping:

- Add an actor with a `UBoxComponent` (your bounds volume actor).
- On your orbit camera:
  - set `bClampToBounds = true`
  - assign `CameraBoundsActor`
  - set `BoundsPadding`

### 4) Add a camera manager pawn
Place or spawn a `AOrbitCameraManagerBase` (or BP child) in the level.

On manager:

- assign `ActiveOrbitCamera` (optional; if empty it auto-finds one at BeginPlay)
- optionally assign `WalkingPawnClass` (defaults to `AOrbitWalkingPawn` if unset)

### 5) View target setup
For runtime camera control, set player view target to your orbit camera or use manager helper calls:

- `TransitionToCamera(...)`
- `EnterWalkingMode(...)`
- `ExitWalkingMode(...)`

---

## Input bindings (legacy input)

### Walking pawn defaults
`AOrbitWalkingPawn` expects these axis mappings:

- `MoveForward`
- `MoveRight`
- `Turn`
- `LookUp`

Add those in **Project Settings -> Input**.

### Orbit camera input
`AOrbitCameraBase` exposes callable methods you can bind from BP/PC input code:

- `AddOrbitInput(float DeltaYaw, float DeltaPitch)`
- `AddPanInput(float Right, float Up)`
- `AddZoomInput(float DeltaZoom)`

---

## Orbit camera runtime features

## 1) Smoothing and comfort
Use `ComfortProfile` to quickly pick motion behavior:

- `Cinematic`
- `Comfort`
- `Snappy`
- `Custom`

Or call `ApplyComfortProfile(...)` at runtime.

Advanced tuning is available via `InputTuning`:

- orbit/pan/zoom sensitivity
- max angular velocity
- angular acceleration
- look-ahead strength/max distance
- boundary damping strength

## 2) Transitions
Use:

- `StartTransitionToDefinition(...)`
- `CancelTransition(...)`
- `GetCurrentDefinition()`

Transition settings are stored in `FOrbitTransitionParams`:

- `Duration`
- `Easing` (`Linear`, `EaseInOut`, `CubicInOut`, `ExpoInOut`, `CustomCurve`)
- optional `CustomCurve`
- optional focus-distance blending

## 3) Focus / DOF behavior
`FocusBehavior` modes:

- `OC_FocusOrigin`
- `OC_AutoFocus`
- `OC_TargetActor`
- `Off`

Useful settings:

- `FocusOffset`
- `AutoFocus_TraceRadius`
- `MinAutoFocusDistance` / `MaxAutoFocusDistance`
- `AutoFocus_InterpolationSpeedIn` / `AutoFocus_InterpolationSpeedOut`
- `FocusDeadzone`
- `FocusSwitchThreshold`
- `FocusTargetHoldSeconds`
- `FocusTargetActor`

Optional debug visuals:

- `bDrawDebugFocus`

## 4) Collision soft-solve
Orbit camera includes a soft camera collision pass that retracts target distance when obstructions are detected between orbit root and desired camera position.

## 5) Bounds clamping
Orbit camera bounds clamping is separate from walking mode bounds.

Orbit camera fields:

- `bClampToBounds`
- `CameraBoundsActor`
- `BoundsPadding`

---

## Manager + walking mode

`AOrbitCameraManagerBase` provides high-level mode switching:

- `TransitionToCamera(AOrbitCameraBase* TargetCamera, EOrbitCameraTransition TransitionType)`
- `EnterWalkingMode(bool bMatchCurrentCamera)`
- `ExitWalkingMode(bool bMatchWalkCameraToOrbit)`

When entering walking mode, manager can copy the current orbit camera transform to the walking pawn and pass bounds settings through.

### Walking pawn bounds
`AOrbitWalkingPawn` supports runtime bounds via:

- `ConfigureMovementBounds(bool bEnableBounds, AActor* InBoundsActor, float InBoundsPadding)`

---

## Blueprint integration pattern (recommended)

1. Create BP child classes:
   - `BP_OrbitCamera` from `AOrbitCameraBase`
   - `BP_OrbitManager` from `AOrbitCameraManagerBase`
   - `BP_OrbitWalkingPawn` from `AOrbitWalkingPawn`

2. Put `BP_OrbitCamera` and `BP_OrbitManager` in level.

3. On BeginPlay (PC or manager BP):
   - Set view target to orbit camera.
   - Optionally apply comfort profile.

4. Bind input:
   - mouse drag -> `AddOrbitInput`
   - middle drag -> `AddPanInput`
   - mouse wheel -> `AddZoomInput`
   - key toggle -> `EnterWalkingMode`/`ExitWalkingMode`

---

## Troubleshooting

### Build fails after adding files
- Regenerate VS project files from `.uproject`.
- Rebuild with `Development Editor | Win64`.

### No walking movement
- Ensure axis mappings (`MoveForward`, `MoveRight`, `Turn`, `LookUp`) exist.

### Camera not clamping
- Verify bounds actor has a `UBoxComponent`.
- Ensure `bClampToBounds` is enabled and actor reference is set.

### Autofocus is jittery
- Increase `FocusDeadzone`.
- Increase `FocusTargetHoldSeconds`.
- Increase `FocusSwitchThreshold`.

---

## Notes

- This plugin README is intentionally independent of the root project README.
- Use this as the canonical setup/usage reference for the orbit camera plugin module.
