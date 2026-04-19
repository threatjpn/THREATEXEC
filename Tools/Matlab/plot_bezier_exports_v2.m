function plot_bezier_exports_v2(baseDir)
% PLOT_BEZIER_EXPORTS_V2  Plot THREATEXEC Bezier exports from old or new JSON formats.
%
% Supports:
%   1) Legacy UE single-curve exports:
%      - control.json / samples.json / proof.json
%      - control3d.json / samples3d.json / proof3d.json
%
%   2) New 3ds Max curve-set export:
%      - curve_set.json
%
% Usage:
%   plot_bezier_exports_v2
%   plot_bezier_exports_v2('C:\path\to\Saved\Bezier')
%
% Notes:
%   - New curve_set.json contains sampled control arrays per curve.
%   - This viewer plots each curve in 2D or 3D based on its "space" field.
%   - Closed curves are drawn with a closing segment for readability.

    % ---------------------------------------------------------------------
    % Resolve folder
    % ---------------------------------------------------------------------
    if ~exist('baseDir','var') || isempty(baseDir)
        defaultDir = fullfile(getenv('USERPROFILE'), 'Documents', 'Unreal Projects', ...
                              'THREATEXEC', 'Saved', 'Bezier');
        if isfolder(defaultDir)
            baseDir = defaultDir;
        else
            d = uigetdir(pwd, 'Select your Saved/Bezier folder');
            if isequal(d, 0)
                disp('Canceled.');
                return;
            end
            baseDir = d;
        end
    end

    % ---------------------------------------------------------------------
    % Prefer new curve_set.json if present
    % ---------------------------------------------------------------------
    curveSetPath = fullfile(baseDir, 'curve_set.json');
    if exist(curveSetPath, 'file')
        data = tryReadJSON(curveSetPath);
        if ~isempty(data) && isfield(data, 'curves')
            plotCurveSet(data, curveSetPath);
            return;
        end
    end

    % ---------------------------------------------------------------------
    % Fallback to legacy exports
    % ---------------------------------------------------------------------
    plotLegacyExports(baseDir);
end

% ======================================================================
% New curve_set.json plotting
% ======================================================================
function plotCurveSet(data, sourcePath)
    curves = data.curves;
    if ~iscell(curves)
        curves = num2cell(curves);
    end

    % Split 2D and 3D curves
    is2D = false(numel(curves), 1);
    for i = 1:numel(curves)
        c = curves{i};
        if isstruct(c) && isfield(c, 'space')
            is2D(i) = strcmpi(string(c.space), "2D");
        end
    end

    idx2 = find(is2D);
    idx3 = find(~is2D);

    if ~isempty(idx2)
        figure('Name', 'Bezier Curve Set 2D'); clf; hold on; axis equal;
        grid on; box on;
        title(buildTitle('curve\_set.json - 2D curves', data, sourcePath));
        cmap = lines(max(numel(idx2), 1));
        labels = cell(numel(idx2), 1);

        for k = 1:numel(idx2)
            c = curves{idx2(k)};
            P = pointsToMatrix(c.control, 2);
            if logicalField(c, 'closed') && ~isempty(P)
                P = [P; P(1,:)];
            end
            plot(P(:,1), P(:,2), '-', 'LineWidth', 1.4, 'Color', cmap(k,:));
            plot(P(:,1), P(:,2), '.', 'MarkerSize', 10, 'Color', cmap(k,:));
            labels{k} = curveLabel(c, idx2(k));
        end
        legend(labels, 'Interpreter', 'none', 'Location', 'bestoutside');
        hold off;
    end

    if ~isempty(idx3)
        figure('Name', 'Bezier Curve Set 3D'); clf; hold on; axis equal; axis vis3d;
        grid on; box on;
        title(buildTitle('curve\_set.json - 3D curves', data, sourcePath));
        view(3);
        cmap = lines(max(numel(idx3), 1));
        labels = cell(numel(idx3), 1);

        for k = 1:numel(idx3)
            c = curves{idx3(k)};
            P = pointsToMatrix(c.control, 3);
            if logicalField(c, 'closed') && ~isempty(P)
                P = [P; P(1,:)];
            end
            plot3(P(:,1), P(:,2), P(:,3), '-', 'LineWidth', 1.4, 'Color', cmap(k,:));
            plot3(P(:,1), P(:,2), P(:,3), '.', 'MarkerSize', 10, 'Color', cmap(k,:));
            labels{k} = curveLabel(c, idx3(k));
        end
        legend(labels, 'Interpreter', 'none', 'Location', 'bestoutside');
        hold off;
    end

    if isempty(idx2) && isempty(idx3)
        warning('curve_set.json was found, but no plottable curves were detected.');
    end
end

function txt = buildTitle(prefix, data, sourcePath)
    meta = {};
    if isfield(data, 'version')
        meta{end+1} = sprintf('version=%s', valueToText(data.version)); %#ok<AGROW>
    end
    if isfield(data, 'point_space')
        meta{end+1} = sprintf('point_space=%s', valueToText(data.point_space)); %#ok<AGROW>
    end
    if isfield(data, 'scale')
        meta{end+1} = sprintf('scale=%s', valueToText(data.scale)); %#ok<AGROW>
    end

    [~, name, ext] = fileparts(sourcePath);
    srcName = [name ext];
    if isempty(meta)
        txt = sprintf('%s (%s)', prefix, srcName);
    else
        txt = sprintf('%s (%s | %s)', prefix, srcName, strjoin(meta, ', '));
    end
end

function label = curveLabel(c, idx)
    parts = {sprintf('#%d', idx)};
    if isfield(c, 'name')
        parts{end+1} = valueToText(c.name); %#ok<AGROW>
    end
    if isfield(c, 'sampling_mode')
        parts{end+1} = sprintf('mode=%s', valueToText(c.sampling_mode)); %#ok<AGROW>
    end
    if isfield(c, 'sample_count')
        parts{end+1} = sprintf('n=%s', valueToText(c.sample_count)); %#ok<AGROW>
    end
    label = strjoin(parts, ' | ');
end

% ======================================================================
% Legacy plotting
% ======================================================================
function plotLegacyExports(baseDir)
    % ------------------------------ 2D ----------------------------------
    ctl2   = tryReadJSON(fullfile(baseDir, 'control.json'));
    smp2   = tryReadJSON(fullfile(baseDir, 'samples.json'));
    proof2 = tryReadJSON(fullfile(baseDir, 'proof.json'));

    if ~isempty(ctl2) || ~isempty(smp2) || ~isempty(proof2)
        figure('Name', 'Bezier 2D'); clf; hold on; axis equal;
        grid on; box on; title('Bezier 2D: control + samples + proof');
    end

    if ~isempty(ctl2) && isfield(ctl2, 'control')
        P = pointsToMatrix(ctl2.control, 2);
        plot(P(:,1), P(:,2), 'o-', 'Color', [0.7 0.7 0.7], 'LineWidth', 1.0, 'MarkerSize', 6);
    end
    if ~isempty(smp2) && isfield(smp2, 'samples')
        S = pointsToMatrix(smp2.samples, 2);
        plot(S(:,1), S(:,2), '.', 'MarkerSize', 14);
    end
    if ~isempty(proof2) && isfield(proof2, 'levels')
        L = proof2.levels;
        if ~iscell(L)
            L = num2cell(L);
        end
        cmap = lines(numel(L));
        for k = 1:numel(L)
            Lk = pointsToMatrix(L{k}, 2);
            c = chooseProofColor(k, numel(L), cmap);
            plot(Lk(:,1), Lk(:,2), '-', 'LineWidth', 1.0, 'Color', c);
            plot(Lk(:,1), Lk(:,2), 's', 'MarkerSize', 6, 'Color', c, 'MarkerFaceColor', c);
        end
        legend({'Control', 'Samples', 'Levels (incl. P(t))'}, 'Location', 'best');
    end
    hold off;

    % ------------------------------ 3D ----------------------------------
    ctl3   = tryReadJSON(fullfile(baseDir, 'control3d.json'));
    smp3   = tryReadJSON(fullfile(baseDir, 'samples3d.json'));
    proof3 = tryReadJSON(fullfile(baseDir, 'proof3d.json'));

    if ~isempty(ctl3) || ~isempty(smp3) || ~isempty(proof3)
        figure('Name', 'Bezier 3D'); clf; hold on; grid on; box on; axis vis3d; axis equal;
        title('Bezier 3D: control + samples + proof');
    end

    if ~isempty(ctl3) && isfield(ctl3, 'control')
        P3 = pointsToMatrix(ctl3.control, 3);
        plot3(P3(:,1), P3(:,2), P3(:,3), 'o-', 'Color', [0.7 0.7 0.7], 'LineWidth', 1.0, 'MarkerSize', 6);
    end
    if ~isempty(smp3) && isfield(smp3, 'samples')
        S3 = pointsToMatrix(smp3.samples, 3);
        plot3(S3(:,1), S3(:,2), S3(:,3), '.', 'MarkerSize', 14);
    end
    if ~isempty(proof3) && isfield(proof3, 'levels')
        L3 = proof3.levels;
        if ~iscell(L3)
            L3 = num2cell(L3);
        end
        cmap = lines(numel(L3));
        for k = 1:numel(L3)
            Lk = pointsToMatrix(L3{k}, 3);
            c = chooseProofColor(k, numel(L3), cmap);
            plot3(Lk(:,1), Lk(:,2), Lk(:,3), '-', 'LineWidth', 1.0, 'Color', c);
            plot3(Lk(:,1), Lk(:,2), Lk(:,3), 's', 'MarkerSize', 6, 'Color', c, 'MarkerFaceColor', c);
        end
        legend({'Control', 'Samples', 'Levels (incl. P(t))'}, 'Location', 'best');
    end
    hold off;

    if isempty(ctl2) && isempty(smp2) && isempty(proof2) && ...
       isempty(ctl3) && isempty(smp3) && isempty(proof3)
        warning('No supported JSON files were found in: %s', baseDir);
    end
end

function c = chooseProofColor(k, n, cmap)
    if k == n
        c = [1 0 0];
    else
        c = cmap(k,:);
    end
end

% ======================================================================
% Helpers
% ======================================================================
function S = tryReadJSON(path)
    if exist(path, 'file')
        txt = fileread(path);
        S = jsondecode(txt);
    else
        S = [];
    end
end

function M = pointsToMatrix(a, dims)
    if isempty(a)
        M = [];
        return;
    end

    if iscell(a)
        if isempty(a)
            M = zeros(0, dims);
            return;
        end
        a = vertcat(a{:});
    end

    M = double(a);
    if size(M, 2) ~= dims
        error('Expected Nx%d array, got Nx%d.', dims, size(M,2));
    end
end

function tf = logicalField(s, fieldName)
    tf = false;
    if ~isstruct(s) || ~isfield(s, fieldName)
        return;
    end
    v = s.(fieldName);
    if islogical(v)
        tf = v;
    elseif isnumeric(v)
        tf = v ~= 0;
    elseif ischar(v) || isstring(v)
        txt = lower(strtrim(char(v)));
        tf = any(strcmp(txt, {'true','1','yes'}));
    end
end

function txt = valueToText(v)
    if isstring(v)
        txt = char(v);
    elseif ischar(v)
        txt = v;
    elseif isnumeric(v) || islogical(v)
        txt = num2str(v);
    else
        txt = '<complex>';
    end
end
