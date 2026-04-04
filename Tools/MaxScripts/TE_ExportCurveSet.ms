-- ============================================================================
-- TE_ExportCurveSet.ms
-- Export selected 3ds Max spline shapes into curve_set.json format for
-- Unreal BezierCurveSetActor import.
--
-- Includes per-curve metadata expected by latest importer:
--   - source_profile: "max_sampled"
--   - sampling_mode: "arc_length"
--   - sample_count: <samplesPerSpline>
-- ============================================================================

fn TE_JsonEscape s =
(
    if s == undefined then return ""
    local r = s as string
    r = substituteString r "\\" "\\\\"
    r = substituteString r "\"" "\\\""
    r = substituteString r "\r" "\\r"
    r = substituteString r "\n" "\\n"
    r = substituteString r "\t" "\\t"
    r
)

fn TE_FloatStr f =
(
    local ci = dotNetObject "System.Globalization.CultureInfo" "en-US"
    local d  = dotNetObject "System.Double" (f as float)
    (dotNetClass "System.String").Format ci "{0:0.000000}" d
)

fn TE_IntStr i =
(
    (i as integer) as string
)

fn TE_Join arr delim =
(
    local out = ""
    for i = 1 to arr.count do
    (
        out += arr[i]
        if i < arr.count do out += delim
    )
    out
)

fn TE_CmPerSystemUnit =
(
    1.0 / (units.decodeValue "1cm")
)

fn TE_PointToArrayStr p is2D: scaleCmPerUnit: cmPerSystemUnit: =
(
    local pc = p * cmPerSystemUnit

    -- Axis mapping kept to match existing Unreal setup.
    local x = (pc.y as float) / scaleCmPerUnit
    local y = (pc.x as float) / scaleCmPerUnit
    local z = (pc.z as float) / scaleCmPerUnit

    if is2D then
        ("[" + (TE_FloatStr x) + "," + (TE_FloatStr y) + "]")
    else
        ("[" + (TE_FloatStr x) + "," + (TE_FloatStr y) + "," + (TE_FloatStr z) + "]")
)

fn TE_GetSamplePoint shp splineIndex param useWorld: =
(
    local p = interpCurve3D shp splineIndex param
    if useWorld then p = p * shp.objectTransform
    p
)

fn TE_BuildSampledPointArray shp splineIndex sampleCount useWorld: closedSpline: =
(
    local pts = #()
    local finalCount = amax 2 sampleCount

    if closedSpline then
    (
        for i = 0 to (finalCount - 1) do
        (
            local t = (i as float) / (finalCount as float)
            append pts (TE_GetSamplePoint shp splineIndex t useWorld:useWorld)
        )
    )
    else
    (
        for i = 0 to (finalCount - 1) do
        (
            local t = (i as float) / ((finalCount - 1) as float)
            append pts (TE_GetSamplePoint shp splineIndex t useWorld:useWorld)
        )
    )

    pts
)

fn TE_ExportSelectedCurveSet outPath scaleCmPerUnit:100.0 useWorld:true treat2DIfFlat:true flatEps:1e-6 samplesPerSpline:80 =
(
    local curvesJson = #()
    local sel = selection as array
    local cmPerSystemUnit = TE_CmPerSystemUnit()

    if sel.count == 0 then
    (
        messageBox "Select at least one spline shape first." title:"TE Export"
        return false
    )

    for obj in sel do
    (
        if (superClassOf obj == Shape) and (isKindOf obj SplineShape) do
        (
            local ns = numSplines obj
            for s = 1 to ns do
            (
                local closedSpline = isClosed obj s
                local sampledPts = TE_BuildSampledPointArray obj s samplesPerSpline useWorld:useWorld closedSpline:closedSpline
                if sampledPts.count < 2 do continue

                local allFlat = true
                for p in sampledPts while allFlat do
                (
                    if (abs p.z) > flatEps then allFlat = false
                )

                local is2D = (treat2DIfFlat and allFlat)
                local spaceStr = if is2D then "2D" else "3D"
                local closedStr = if closedSpline then "true" else "false"

                local ctrlParts = #()
                for p in sampledPts do
                (
                    append ctrlParts (TE_PointToArrayStr p is2D:is2D scaleCmPerUnit:scaleCmPerUnit cmPerSystemUnit:cmPerSystemUnit)
                )
                local controlJson = "[" + (TE_Join ctrlParts ",") + "]"

                local cname = TE_JsonEscape (obj.name + "_" + (s as string))
                local curveJson =
                (
                    "{\"name\":\"" + cname +
                    "\",\"space\":\"" + spaceStr +
                    "\",\"closed\":" + closedStr +
                    ",\"source_profile\":\"max_sampled\"" +
                    ",\"sampling_mode\":\"arc_length\"" +
                    ",\"sample_count\":" + (TE_IntStr samplesPerSpline) +
                    ",\"control\":" + controlJson + "}"
                )

                append curvesJson curveJson
            )
        )
    )

    local pointSpaceStr = if useWorld then "world" else "local"

    local fullJson =
    (
        "{\"version\":2" +
        ",\"point_space\":\"" + pointSpaceStr + "\"" +
        ",\"scale\":" + (TE_FloatStr scaleCmPerUnit) +
        ",\"curves\":[" + (TE_Join curvesJson ",") + "]}"
    )

    local f = createFile outPath
    if f == undefined then
    (
        messageBox ("Failed to create file:\n" + outPath) title:"TE Export"
        return false
    )

    format "%" fullJson to:f
    close f

    messageBox ("Exported " + (curvesJson.count as string) + " curve(s) to:\n" + outPath) title:"TE Export"
    true
)

rollout TE_ExportCurveSet "TE Export Curve Set" width:420
(
    group "Output"
    (
        editText edtPath "File" text:"C:\\Temp\\curve_set.json" width:340
        button btnBrowse "Browse..." width:70
    )

    group "Options"
    (
        spinner spnScale "UE cm-per-control-unit" type:#float range:[0.001, 100000.0, 100.0] fieldwidth:90
        checkbox chkWorld "Export in World Space" checked:true
        checkbox chk2D "If flat, export as 2D" checked:true
        spinner spnEps "Flat epsilon" type:#float range:[0.0, 1.0, 0.000001] scale:0.000001 fieldwidth:120
        spinner spnSamples "Samples per spline" type:#integer range:[2, 1000, 80] fieldwidth:90
    )

    button btnExport "Export Selected Splines" height:34

    on btnBrowse pressed do
    (
        local p = getSaveFileName caption:"Save curve_set.json" types:"JSON (*.json)|*.json|All|*.*|"
        if p != undefined do edtPath.text = p
    )

    on btnExport pressed do
    (
        TE_ExportSelectedCurveSet edtPath.text \
            scaleCmPerUnit:spnScale.value \
            useWorld:chkWorld.checked \
            treat2DIfFlat:chk2D.checked \
            flatEps:spnEps.value \
            samplesPerSpline:spnSamples.value
    )
)

createDialog TE_ExportCurveSet
