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

> `ActiveOrbitCamera` is instance-only by design. Set it on the placed manager actor in the level, not in BP class defaults.
> If left empty, manager can auto-find a camera at BeginPlay (`bAutoFindOrbitCameraIfUnset` / `AutoFindOrbitCameraTag`).

### Make it work on BeginPlay (required)

`AOrbitCameraManagerBase` is a Pawn, so it must be possessed by Player 0 on play.

Use one of these patterns:

1. **GameMode route (recommended)**
   Set your custom GameMode `Default Pawn Class` to `AOrbitCameraManagerBase`.
2. **Level Blueprint route**
   `Event BeginPlay` -> `Get Player Controller(0)` -> `Possess` (target = OrbitCameraManager actor in level).

Then make sure manager `ActiveOrbitCamera` points to your placed `AOrbitCameraBase`.
If it is empty, enable `bAutoFindOrbitCameraIfUnset` so the manager automatically binds to the first orbit camera in the level (or the first with `AutoFindOrbitCameraTag`).

For easiest setup, keep these enabled on manager:
- `bAutoPossessPlayer0OnBeginPlay`
- `bAutoManageViewTarget`

This makes play-in-editor automatically possess the manager and switch the camera view target correctly.

Walk-mode spawn behavior:
- Enable `bWalkSpawnFromManagerPlacement` if you want walk mode to start exactly where the manager actor is placed.

### Default walk keybinds (manager)

These are coded directly in `AOrbitCameraManagerBase` and exposed as editable `FKey` properties:

- Toggle walk-out: `Tab`
- Move: `W / A / S / D`
- Vertical move: `E / Q`
- Sprint: `LeftShift`
- Look: Mouse X / Mouse Y axis
- Orbit look (while in orbit mode): hold `Right Mouse Button` + Mouse X/Y
- Orbit pan (while in orbit mode): hold `Middle Mouse Button` + Mouse X/Y
- Orbit zoom (while in orbit mode): `Mouse Wheel`
- Debug menu toggle: `F4`
- Debug toggles while menu active:
  - `F5` autofocus traces
  - `F6` bounds debug draw
  - `F7` walk state diagnostics

> You can change any of these in the manager details panel or in Blueprint defaults.
> For softer motion, tune `OrbitLookYawSpeed`, `OrbitLookPitchSpeed`, `OrbitPanSpeed`, `OrbitZoomStep` and smoothing options (`bSmoothOrbitControls`, `OrbitLookSmoothingSpeed`, `OrbitPanSmoothingSpeed`, `OrbitZoomSmoothingSpeed`).
> Zoom now primarily changes camera distance. Enable `bOrbitZoomChangesFocalLength` only if you also want wheel zoom to modify focal length.

---

## Walk-only level setup (no orbit camera)

If you want a level to use only walk mode:

1. Place `AOrbitCameraManagerBase`.
2. Enable `bWalkModeOnly = true`.
3. (Optional) Enable `bStartInWalkOutMode = true` for immediate walk control at BeginPlay.
4. Recommended for character-like feel:
   - `WalkCharacterClass = OrbitWalkCharacter` (or your own `Character` subclass)
   - `bWalkPlanarMovement = true`
   - `bAllowWalkVerticalInput = false`
   - `bUseFirstPersonWalkModel = true`
   - Tune `WalkAcceleration` / `WalkBrakingDeceleration`

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

1. Enable `bEnableAdvancedDOFStack`.
2. Choose `AdvancedDOFPreset`.
3. Tune preset structs (`AdvancedCinematicDOF`, `AdvancedGameplayDOF`, etc.) only when needed.
4. Use `bSmoothAdvancedDOFTransitions` + `AdvancedDOFTransitionSpeed` for blend behavior.

---

## Bounds options

### Orbit camera bounds

- `AOrbitCameraBase`:
  - `bClampToBounds`
  - `CameraBoundsActor` (must contain `UBoxComponent`)
  - `BoundsPadding`
  - `bClampCameraComponentToBounds` (enable only if you want camera body clamping; can cause vertical push near floor/ceiling bounds)
  - `bSmoothCameraBoundsPush` + `CameraBoundsPushSmoothingSpeed` (soften camera push when camera-body clamping is enabled)

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
- **Walk mode does not feel like first-person walking**
  - Walk mode is CharacterMovement-based by default (`WalkCharacterClass`).
  - Enable `bUseFirstPersonWalkModel` for acceleration/braking movement.
  - Keep `bWalkPlanarMovement=true` and `bAllowWalkVerticalInput=false` for grounded motion.
  - Increase `WalkBrakingDeceleration` for snappier stops or lower it for softer glide.
- **I press Play and camera is wrong / not using orbit camera**
  - Make sure manager is possessed (or keep `bAutoPossessPlayer0OnBeginPlay` enabled).
  - Keep `bAutoManageViewTarget` enabled so view target swaps between orbit camera and walk mode automatically.
- **RMB / mouse wheel orbit controls do nothing**
  - Make sure you are in orbit mode (not walk-out mode).
  - Hold `OrbitLookHoldKey` (default RMB) while moving mouse for rotate.
  - Use `OrbitPanHoldKey` (default MMB) for pan and Mouse Wheel for zoom.
- **Walk mode toggles but WASD does not move**
  - Confirm `WalkCharacterClass` is valid (defaults to `OrbitWalkCharacter`).
  - Ensure your pawn/controller actually possesses the spawned walk character after pressing Tab.
- **Orbit movement is too fast / not smooth**
  - Lower `OrbitLookYawSpeed`, `OrbitLookPitchSpeed`, `OrbitPanSpeed`, `OrbitZoomStep`.
  - Enable `bSmoothOrbitControls` and raise smoothing speeds gradually until motion feels softer.
- **Middle-mouse pan feels inverted on Y**
  - Pan Y direction was updated so moving mouse up pans up.
- **Camera gets pushed up near floor bound**
  - Disable `bClampCameraComponentToBounds` on `AOrbitCameraBase` so only orbit root clamping is applied.
- **Bounds push jitters when orbit smoothing is low**
  - Keep `bSmoothCameraBoundsPush = true` and raise `CameraBoundsPushSmoothingSpeed`.
  - Keep orbit smoothing enabled and tune `OrbitPanSmoothingSpeed` / `OrbitLookSmoothingSpeed` together with bounds push speed.
