# THREATEXEC

THREATEXEC provides runtime Bezier editing tools for Unreal Engine.

## Feature overview

- Runtime control-point interaction (hover, select, drag).
- 2D and 3D curve actor support.
- Curve-set import/export through a single JSON document.
- Runtime edit controls for visibility, snapping, and sampling.
- De Casteljau proof visualization and debug utilities.
- Fade widget infrastructure for transitions and loading feedback.

## Repository layout

- `THREATEXEC/` — Main Unreal module source.
- `THREATEXEC/Public/` — Public headers for runtime systems.
- `THREATEXEC/Private/` — Runtime implementation and automation specs.
- `Tools/` — Utility scripts (including DCC export helpers).

## Core runtime classes

- `ABezierCurve2DActor` / `ABezierCurve3DActor` — Editable Bezier actors.
- `ABezierEditPlayerController` — Input and hit-testing for control points.
- `UBezierEditSubsystem` — Focused/all routing for runtime operations.
- `ABezierCurveSetActor` — Bulk import/export and spawned curve tracking.
- `ABezierDebugActor` / `ABezierDebugHUD` — Runtime debug controls.

---

## Step-by-step setup in a brand-new Unreal project

### 1) Create a new Unreal C++ project

1. Open Unreal Engine and create a new **Games > Blank** C++ project.
2. Close the editor after the initial project files are generated.

### 2) Paste the source files inside the Source folder

Add .cpp & .h to the following:

1. .cpp files inside a Private folder
2. .h files inside a Public folder
3. Test files inside Private/Tests
4. Regenerate project files from the `.uproject` context menu.

### 4) Build and open editor

1. Open the generated solution/workspace.
2. Build the editor target (for example: `MyGameEditor Development`).
3. Launch Unreal Editor from the IDE or the `.uproject` file.

### 5) Configure runtime Bezier editing

1. Set the active GameMode PlayerController class to `ABezierEditPlayerController` (or a derived Blueprint).
2. In **Project Settings > Input (Legacy)**, add action mapping `Secondary` bound to Right Mouse Button.
3. Ensure control-point material reads `PerInstanceCustomData` indices `0..2` for RGB hover/selection states.
4. Place an `ABezierCurveSetActor` in the level for batch import/export and curve tracking.
5. Enable runtime editing using either:
   - `UBezierEditSubsystem::All_SetEditInteractionEnabled(true, true, true)`
   - or `ABezierCurveSetActor::UI_SetEditInteractionEnabledForAll(...)`
---

## Test suite guide

THREATEXEC includes Unreal automation specs under `YOURPROJECT/Private/Tests/`:

- `BezierSpec.cpp`
- `BezierIntegrationSpec.cpp`
- `BezierUIAutomationSpec.cpp`

### Run tests from Unreal Editor

1. Open **Tools > Session Frontend > Automation**.
2. Filter tests by `ThreatExec`.
3. Run all filtered tests.
4. Review pass/fail output in the Automation panel.

### Run tests from command line (UnrealEditor-Cmd)

Example command (Windows path style shown; adapt paths/target names as needed):

```bash
UnrealEditor-Cmd.exe <PathToProject>.uproject -unattended -nop4 -nullrhi -ExecCmds="Automation RunTests ThreatExec; Quit"
```

Recommended additions for CI environments:

```bash
-testexit="Automation Test Queue Empty" -log -ReportExportPath="<ReportOutputFolder>"
```

Automation output artifacts are commonly written to `Saved/Automation/`.

---

## Curve-set JSON schema (summary)

```json
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
      "control": [[x, y, z], [x, y, z]]
    }
  ]
}
```

## Additional notes

- Focused-only APIs require an active focused editable actor.
