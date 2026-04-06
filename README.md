# THREATEXEC

Runtime Bezier editing tools for Unreal Engine, including:
* In-game control point editing (hover/select/drag).
* Curve-set import/export (single JSON file).
* Runtime batch controls (show/hide, snapping, sizes, colors).
* Runtime sampling controls (mode/count) and proof visualization.
* Debug helpers (trace overlays, toggle settings).

---

## Core runtime actors and subsystems

### `ABezierCurve3DActor` / `ABezierCurve2DActor`
* Curve actors with runtime-edit APIs (show/hide control points, strip visuals, snap/grid, etc).
* Control-point hover/selected colors are driven by **PerInstanceCustomData** (RGB indices 0..2).
  * Your control-point material must read PerInstanceCustomData to see hover/selected color changes.

### `ABezierEditPlayerController`
* Handles hover, select, and drag for control points.
* Uses **Visibility** traces to hit control point instanced meshes.
* Default input binding uses action name **`Secondary`** (right-click).
* Debug trace output can be toggled with `bDebugTrace` or `SetDebugTrace(...)`.

### `UBezierEditSubsystem`
* Focused vs. all routing for edit actions.
* Batch runtime edit toggles (`All_*` functions).
* Curve-set IO helpers (`All_ExportCurveSetJson`, `All_ImportCurveSetJson`).
* Focused curve tools for mirror/cycle, closed loop, sampling controls, and proof visualization.
* Auto-focus safety: `SetAutoFocusFirstEditable(true)` and `Focus_EnsureFocused()` to guard focused-only actions.

### `ABezierCurveSetActor`
* Imports/exports a **single JSON** containing all curves.
* Tracks spawned curve actors and provides batch runtime helpers.
* Optional autosave support and backup writing on export.

### `ABezierDebugActor`
* In-world debug toggle actor for controller trace debug, curve visuals, snapping, and IO.
* Place in your level to quickly verify behavior without editing blueprints.

### `ABezierDebugHUD`
* Minimal in-game overlay with hotkeys to toggle edit/debug features.
* Set your GameMode HUD class to `ABezierDebugHUD` and press **F7** to show/hide.

---

## Setup in Unreal Editor (Runtime)

### 1) Set the PlayerController
Set your GameMode’s PlayerController to `ABezierEditPlayerController` (or a BP child).

### 2) Input mapping (Legacy Input)
Add an **Action Mapping** named `Secondary` bound to Right Mouse Button.
* This matches the controller’s default action name for selection.

### 3) Control-point material (required for hover/selected colors)
Your control-point material must read **PerInstanceCustomData**:
* Index 0 → R
* Index 1 → G
* Index 2 → B

If the material doesn’t read these values, hover/selected colors won’t show even though the data updates.

### 4) Enable runtime editing
At runtime, enable edit mode and control point visibility:
* `UBezierEditSubsystem::All_SetEditInteractionEnabled(true, true, true)`
  * or call the equivalent `ABezierCurveSetActor::UI_SetEditInteractionEnabledForAll(...)`

If you rely on focused-only calls, ensure a focused curve exists (hover/click a control point).

---

## Sampling & proof visualization (runtime)

Both 2D and 3D curve actors expose runtime sampling settings:
* **Sampling mode** (`Parametric` or `Arc Length`)
* **Sample count** (drives strip sampling and debug points)
* **Proof T** (0..1) for De Casteljau proof levels
* **Show sample points** toggle
* **Show De Casteljau levels** toggle

### Focused curve API (UMG friendly)
Use these `UBezierEditSubsystem` calls to drive the currently focused curve:
* `Focus_SetSamplingMode(EBezierSamplingMode Mode)`
* `Focus_GetSamplingMode()`
* `Focus_SetSampleCount(int32 Count)`
* `Focus_GetSampleCount()`
* `Focus_SetProofT(double T)`
* `Focus_GetProofT()`
* `Focus_SetShowSamplePoints(bool bShow)`
* `Focus_GetShowSamplePoints()`
* `Focus_SetShowDeCasteljauLevels(bool bShow)`
* `Focus_GetShowDeCasteljauLevels()`

### Per-actor API (direct)
On `ABezierCurve2DActor` / `ABezierCurve3DActor`, you can call:
* `UI_SetSamplingMode(...)`, `UI_GetSamplingMode()`
* `UI_SetSampleCount(...)`, `UI_GetSampleCount()`
* `UI_SetProofT(...)`, `UI_GetProofT()`
* `UI_SetShowSamplePoints(...)`, `UI_GetShowSamplePoints()`
* `UI_SetShowDeCasteljauLevels(...)`, `UI_GetShowDeCasteljauLevels()`

---

## Mirror & closed-loop helpers (focused curves)

Use the focused-only functions from `UBezierEditSubsystem` to drive UI buttons:
* `Focus_MirrorCurve()` cycles axis per press (X/Y for 2D, X/Y/Z for 3D).
  * The mirror cycle **resets after 10s** of inactivity.
  * `OnMirrorAxisCycleReset` can be bound in UMG to show a popup when it resets.
  * `Focus_GetMirrorAxisCycleSecondsRemaining()` returns time left until reset.
* `Focus_ToggleClosedLoop()` / `Focus_SetClosedLoop(bool)` / `Focus_IsClosedLoop()`
* `Focus_ReverseControlOrder()`
* `Focus_CenterCurve()`
* `Focus_DuplicateCurve()`
* `Focus_IsolateCurve(bool)`
* `Focus_ToggleIsolateCurve()`
* `Focus_EnsureFocused()` (auto-focuses the first editable if enabled)

---

## Curve-set import/export (single JSON)

### JSON format
```
{
  "version": 2,
  "point_space": "local",
  "scale": 100.0,
  "curves": [
    {
      "name": "CurveName",
      "space": "3D",
      "closed": false,
      "sampling_mode": "parametric",
      "sample_count": 64,
      "control": [[x,y,z], [x,y,z], ...]
    }
  ]
}
```

### Runtime usage
Place an `ABezierCurveSetActor` in the level:
* Set `IOPathAbsolute` and `CurveSetFile`.
* Set `ImportMode` to control how named curves merge:
  * `Replace All` (default)
  * `Replace By Name`
  * `Skip Existing`
  * `Append`
* Call:
  * `UI_ImportCurveSetJson()` to load and spawn curves.
  * `UI_ExportCurveSetJson()` to save all curves to one file.
  * `UI_SaveCurveSetJsonByFileName("my_save_name")` to save a user-named file from UMG.
  * `UI_LoadCurveSetJsonByFileName("my_save_name.json")` to load a selected file from UMG.
  * `UI_ListCurveSetJsonFiles(...)` to populate a true `ListView` with file rows.

### UMG ListView file menu flow (true ListView)
Recommended `WB_FileMenu` function split:
* `RefreshFileList`:
  * call `UI_ListCurveSetJsonFiles`.
  * clear list items.
  * add row objects/items from returned array.
* `SaveCurrentCurveSet`:
  * call `UI_SaveCurveSetJsonByFileName(EditableText_SaveName)`.
  * call `RefreshFileList`.
* `LoadSelectedFile`:
  * call `UI_LoadCurveSetJsonByFileName(SelectedFileName)`.
* `HandleRowSelected`:
  * set `SelectedFileName` and apply single-select visual style in row widget.

`UI_ListCurveSetJsonFiles` returns newest-first rows and includes:
* `FileName`
* `Timestamp` (formatted as `DD-MM-YYYY HH:MM:SS`)
* `FileSize` (KB/MB label)
* `FileSizeBytes` (raw bytes for custom formatting)

### Backup behavior
* Set `bWriteBackupOnExport = true`.
* `BackupCurveSetFile` defines the backup filename.
* On each export, the previous file is copied to the backup before saving.

### Autosave
* Enable `bEnableAutoSave`.
* Set `AutoSaveIntervalSeconds`.
* Optionally require edit mode (`bAutoSaveOnlyWhenEditing`).

---

## Debugging tools

### `ABezierDebugActor`
Drop a `BezierDebugActor` in the level and toggle settings in Details:
* `bEnableMouseTraceDebug` → on-screen hover/trace output.
* Visual toggles for control points/strip, sizing, colors, snapping.
* `bShowPivotAxes` → render pivot axes for selected control points (V in debug HUD).
* `ApplyDebugSettings()` to push settings.

### `ABezierDebugHUD` (overlay)
Set your GameMode’s **HUD Class** to `ABezierDebugHUD` and press **F7** in-game to toggle.
Hotkeys (press **K** to apply after changes):
* **E**: Edit mode
* **C**: Control points
* **S**: Strip
* **G**: Show grid
* **N**: Snap to grid
* **H**: Cycle grid size
* **L**: Lock to XY
* **P**: Force planar
* **V**: Pivot axes
* **D**: Pulse debug lines
* **U**: Pulse control points
* **I**: Pulse strip
* **T**: Trace debug
* **K**: Apply debug settings

---

## Common pitfalls
* **Hover/selected colors not changing:** ensure the control-point material reads PerInstanceCustomData (RGB 0..2).
* **No hover/selection:** make sure control points block **Visibility**, and `Secondary` action exists.
* **UI-only input:** use Game+UI input mode to keep controller trace updates.
