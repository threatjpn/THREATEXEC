# OrbitCameraSystem

Plugin-local guide for setting up and using the Orbit Camera + Walk-Out workflow.

---

## What is in this plugin

- `AOrbitCameraBase`
  - Orbit camera actor with rotation/zoom limits.
  - Focus modes (`FocusOrigin`, `AutoFocus`, `Off`).
  - Preset-driven DOF (`Cinematic`, `Gameplay`, `Portrait`, `Macro`, `Custom`, `Off`).
- `AOrbitCameraManagerBase`
  - Runtime manager for orbit control and optional walk-out/free-move mode.
  - Supports orbit->walk transitions and walk-only usage per level.

---

## Quick setup (Orbit + optional Walk-Out)

1. **Place one or more `AOrbitCameraBase` actors** in your level.
2. **Place one `AOrbitCameraManagerBase` actor** in your level.
3. In manager details, assign/select your active orbit camera flow as needed in BP/level script.
4. Press Play:
   - Orbit mode works as before.
   - Press `Tab` (default) to toggle walk-out mode.

### Make it work on BeginPlay (required)

`AOrbitCameraManagerBase` is a Pawn, so it must be possessed by Player 0 on play.

Use one of these patterns:

1. **GameMode route (recommended)**
   Set your custom GameMode `Default Pawn Class` to `AOrbitCameraManagerBase`.
2. **Level Blueprint route**
   `Event BeginPlay` -> `Get Player Controller(0)` -> `Possess` (target = OrbitCameraManager actor in level).

Then make sure manager `ActiveOrbitCamera` points to your placed `AOrbitCameraBase`.

### Default walk keybinds (manager)

These are coded directly in `AOrbitCameraManagerBase` and exposed as editable `FKey` properties:

- Toggle walk-out: `Tab`
- Move: `W / A / S / D`
- Vertical move: `E / Q`
- Sprint: `LeftShift`
- Look: Mouse X / Mouse Y axis

> You can change any of these in the manager details panel or in Blueprint defaults.

---

## Walk-only level setup (no orbit camera)

If you want a level to use only walk mode:

1. Place `AOrbitCameraManagerBase`.
2. Enable `bWalkModeOnly = true`.
3. (Optional) Enable `bStartInWalkOutMode = true` for immediate walk control at BeginPlay.

You can also control this from BP using:

- `SetWalkOutModeEnabled(bool)`
- `EnterWalkOutMode()`
- `ExitWalkOutMode()`
- `ToggleWalkOutMode()`

---

## Autofocus setup (recommended)

On `AOrbitCameraBase`:

1. Set `FocusBehavior = AutoFocus`.
2. Configure `FocusOnObjectTypes` (required for trace hits).
3. Start with defaults, then tune these only if needed:
   - `AutoFocus_TraceRadius`
   - `AutoFocus_TraceStartOffset`
   - `bUseAutoFocusMultiSample`
   - `AutoFocus_MultiSampleSpread`
   - `AutoFocus_DeadZone`
   - `AutoFocus_LostTargetHoldTime`
   - `bUseAdaptiveAutoFocusSpeed`
   - `AutoFocus_MinInterpolationSpeed` / `AutoFocus_MaxInterpolationSpeed`
   - `bAutoFocusFallbackToDistance`

### Autofocus behavior summary

- Multi-sample traces reduce noisy single-ray focus jumps.
- Median hit-distance selection rejects outlier hits.
- Dead-zone suppresses tiny focus oscillation.
- Lost-target hold avoids instant snapping on short misses.
- Adaptive interpolation speeds up big refocus changes and slows small ones.

---

## Bulletproof camera placement workflow (editor-friendly)

Use this every time to quickly get the exact angle you want:

1. Select `AOrbitCameraBase`.
2. Move actor in viewport to desired world position.
3. In Details panel, adjust:
   - `InitialYaw`, `InitialPitch`, `InitialDistance`, `InitialFocalLength`
   - or preview min/max endpoints with `YawPreview`, `PitchPreview`, `ZoomPreview`.
4. Click **`ApplyPlacementFromSettings`** (CallInEditor button) to force-refresh component placement.
5. If you manually rotated/moved internals and want to save that as new defaults, click **`CaptureCurrentPlacementAsInitial`**.

This workflow avoids stale editor state and ensures sliders always map to the actual orbit root/camera transform.

---

## DOF preset usage

On `AOrbitCameraBase`:

1. Enable `bEnableAdvancedDOF`.
2. Choose `DOFPreset`.
3. Tune preset structs (`CinematicDOF`, `GameplayDOF`, etc.) only when needed.
4. Use `bSmoothDOFTransitions` + `DOFTransitionSpeed` for blend behavior.

---

## Bounds options

### Orbit camera bounds

- `AOrbitCameraBase`:
  - `bClampToBounds`
  - `CameraBoundsActor` (must contain `UBoxComponent`)
  - `BoundsPadding`

### Walk bounds

- `AOrbitCameraManagerBase`:
  - `bEnableWalkBounds`
  - `WalkBoundsExtent`
  - `WalkBoundsActor` (optional actor with `UBoxComponent`)

---

## Common issues

- **Autofocus does nothing**
  - Usually `FocusOnObjectTypes` is empty or wrong object channels are selected.
- **Focus flickers on thin geometry**
  - Increase `AutoFocus_TraceRadius` slightly and/or enable multi-sample.
  - Raise `AutoFocus_DeadZone`.
- **Walk mode feels unconstrained**
  - Enable walk bounds and set `WalkBoundsExtent` or `WalkBoundsActor`.
