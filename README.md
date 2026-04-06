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
  * `UI_LoadDemoCurveSetJson()` to load `DemoCurveSetFile`.
  * `UI_SaveExportedCurveSetSnapshot()` to save numbered snapshots (`exported_curve_set_N.json`).
  * `UI_ListCurveSetJsonFiles()` to enumerate `.json` files in the Bezier folder for UMG list widgets.
  * `UI_ImportCurveSetJsonByFileName(FileName)` to import a user-selected filename from that list.
  * `UI_SaveCurveSetJsonAs(FileName)` to save using a user-provided name in that same folder.

---

## UMG implementation guide: scrollable load list + save-as textbox

This is the recommended Blueprint graph flow to build the widget you described.

### 1) Widget variables
Create these variables in your UMG widget (for example `WBP_BezierFileMenu`):
* `CurveSetActor` (`BezierCurveSetActor` object reference)
* `FileNames` (`String Array`)
* `SelectedFileName` (`String`)
* `ListView_Files` (your `ListView`/`ScrollBox` reference)
* `EditableText_SaveName` (text entry for save name)

### 2) Resolve `CurveSetActor` at runtime
In `Event Construct` (or when opening the menu):
1. `Get All Actors Of Class` (`BezierCurveSetActor`)
2. `Get` index 0
3. `Set CurveSetActor`
4. Call `RefreshFileList` (custom event)

> Tip: if you have multiple set actors, store a specific reference on your HUD/Controller and pass it into the widget instead of `GetAllActorsOfClass`.

### 3) Build `RefreshFileList` custom event
`RefreshFileList` graph:
1. Branch: `IsValid(CurveSetActor)`
2. `CurveSetActor -> UI_ListCurveSetJsonFiles(true)`
3. Set `FileNames`
4. Clear your list widget (`Clear List Items` on `ListView` or clear children if using `ScrollBox`)
5. Loop through `FileNames` and add one row/button per filename

For each row/button, bind:
* label text = filename
* `OnClicked` => set `SelectedFileName` and optionally highlight selected row

### 4) Load button graph
`OnClicked(LoadButton)`:
1. Branch: `IsValid(CurveSetActor)`
2. Branch: `SelectedFileName` is not empty
3. `CurveSetActor -> UI_ImportCurveSetJsonByFileName(SelectedFileName)` (returns bool)
4. If `true`: optionally close menu / show success text
5. If `false`: show error text (“Couldn’t import file”)

### 5) Save button graph
`OnClicked(SaveButton)`:
1. Read text from `EditableText_SaveName`
2. Convert to string
3. Branch: `IsValid(CurveSetActor)`
4. `CurveSetActor -> UI_SaveCurveSetJsonAs(SaveNameString, false)` (returns bool)
5. If `true`: call `RefreshFileList` so new file appears in the scroll list
6. If `false`: show error text (“Invalid save name” or “Save failed”)

### 6) Optional quality-of-life graph hooks
* `OnTextCommitted` for save name field:
  * if commit method is Enter, trigger save.
* Default save name:
  * on open, prefill text box with `curve_set` or timestamp-based string.
* Sort toggle:
  * pass `false` to `UI_ListCurveSetJsonFiles(false)` for reverse order (newest by naming convention first if your names encode timestamps/indexes).

### 7) Path behavior (important)
The file APIs resolve relative paths under `Project/Saved/<IOPathAbsolute>`, with `Bezier` as default if empty.
So by default, your widget is browsing and writing in:
* `Project/Saved/Bezier`

### 8) Filename behavior (important)
`UI_SaveCurveSetJsonAs` / `UI_ImportCurveSetJsonByFileName` sanitize input:
* trims whitespace
* strips directory parts
* forces `.json` extension if missing
* removes invalid filename characters

So a user can type `MyTrack01` and it saves as `MyTrack01.json` in the Bezier folder.

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
