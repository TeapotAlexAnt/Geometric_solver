import { api } from "./editor-api.js";
import { renderSketch } from "./sketch-view.js";
import { uiState } from "./editor-state.js";
import { RestrictionMeta } from "./restriction-meta.js";
import { icon } from "./icons.js";
import { updateToolDisplay } from "./tools.js";
import { mathMarkup, numberTex } from "./math.js";

const htmlEntities = { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" };

function escapeHtml(text) {
  return text.replace(/[&<>"']/g, char => htmlEntities[char]);
}

function variableTex(index) {
  const axis = index % 2 === 0 ? "x" : "y";
  return "\\Delta " + axis + "_{" + (Math.floor(index / 2) + 1) + "}";
}

function renderGradient(row) {
  if (!row.df_row.length) return mathMarkup("0");
  if (row.df_row.some(([, value]) => !Number.isFinite(value))) return "—";
  const terms = row.df_row.map(([index, value]) => mathMarkup(
    "\\frac{\\partial f_{" + row.eqIndex + "}}{\\partial (" + variableTex(index) + ")}="
    + numberTex(value, 3)
  ));
  return '<div class="math-gradient">' + terms.join("") + "</div>";
}

function formatSketch(json) {
  return JSON.stringify(JSON.parse(json), null, 2);
}

export function renderAll() {
  const state = api.getAllPrimitives();
  if (!state.restrictions.some(r => r.id === uiState.selectedRestrictionId)) uiState.selectedRestrictionId = null;
  updateToolDisplay();
  renderSketch();
  updateHeaderStatus();
  renderRestrictionsBrowser();
  renderPrimitivesBrowser();
  updateJsonPanel();
  if (!document.getElementById("viewMatrix").classList.contains("hidden")) renderMatrixInspector();
  updateSelectionBadge();
}

function updateHeaderStatus() {
  const state = api.getAllPrimitives();
  const { dof, totalVars, totalEqs } = state.dofInfo;
  const res = state.solver;

  document.getElementById("autoSolveToggle").checked = res.auto_solve;
  document.getElementById("dofValue").textContent = dof;
  document.getElementById("iterValue").textContent = res.current ? res.iterations : "—";
  document.getElementById("residualValue").innerHTML = res.current && Number.isFinite(res.max_residual)
    ? mathMarkup(numberTex(res.max_residual, 2))
    : "—";
  const dimension = res.last_jacobian.length;
  document.getElementById("slaeDimBadge").innerHTML =
    res.current ? `${dimension} × ${dimension} ${mathMarkup(`(m=${totalEqs},\\; n=${totalVars})`)}` : "—";

  const eqBadge = document.getElementById("equilibriumBadge");
  if (!res.auto_solve) {
    eqBadge.textContent = "Автопересчёт выключен";
    eqBadge.dataset.state = "paused";
  } else if (res.converged || state.restrictions.length === 0) {
    eqBadge.textContent = "Равновесие";
    eqBadge.dataset.state = "ready";
  } else {
    eqBadge.textContent = "Конфликт";
    eqBadge.dataset.state = "error";
  }

  const defBadge = document.getElementById("modelDefBadge");
  if (state.points.length === 0) {
    defBadge.textContent = "Пустой эскиз";
  } else if (dof === 0) {
    defBadge.textContent = "Полностью определён";
  } else if (dof > 0) {
    defBadge.textContent = `Недоопределён`;
  } else {
    defBadge.textContent = `Переопределён`;
  }

  document.getElementById("countRestrictions").textContent = state.restrictions.length;
  document.getElementById("countPrimitives").textContent = state.standalonePoints.length + state.segments.length;
}

function describeRestrictionTargets(r) {
  const pLabels = r.p_ids.map(pid => api.findPoint(pid)?.label || `P${pid}`);
  const sLabels = r.seg_ids.map(sid => api.findSegment(sid)?.label || `L${sid}`);
  if (r.type === "COINCIDENT" || r.type === "DISTANCE") {
    return sLabels.length ? `${sLabels[0]} (${pLabels.join(" – ")})` : pLabels.join(" ↔ ");
  }
  if (r.type === "PARALLEL" || r.type === "PERPENDICULAR" || r.type === "ANGLE") {
    return sLabels.length >= 2 ? `${sLabels[0]}, ${sLabels[1]}` : pLabels.join(", ");
  }
  if (r.type === "VERTICAL" || r.type === "HORIZONTAL") {
    return sLabels.length ? `${sLabels[0]} (${pLabels.join("–")})` : pLabels.join("–");
  }
  if (r.type === "POINT_ON_LINE") {
    return `${pLabels[0]} на ${sLabels[0] || (pLabels[1] + "–" + pLabels[2])}`;
  }
  if (r.type === "FIX_POINT") {
    return `${pLabels[0]} (${r.value.toFixed(1)}, ${r.value2.toFixed(1)})`;
  }
  return pLabels.join(", ");
}

export function selectRestriction(id, reveal = false) {
  uiState.selectedRestrictionId = id;
  uiState.selectedPointIds = [];
  uiState.selectedSegmentIds = [];
  uiState.segmentStart = null;
  uiState.activeConstraintTool = null;
  uiState.activeTool = "select";
  document.getElementById("rightTabRestrictions").click();
  renderAll();
  const row = document.querySelector(`[data-select-restriction="${id}"]`);
  if (reveal) row?.parentElement.scrollIntoView({ block: "nearest" });
  else row?.focus({ preventScroll: true });
}

function renderRestrictionsBrowser() {
  const list = document.getElementById("restrictionsList");
  const { restrictions } = api.getAllPrimitives();
  if (!restrictions.length) {
    list.innerHTML = '<p class="empty">Ограничений пока нет. Выберите связь на панели инструментов.</p>';
    return;
  }
  const collapsed = new Map([...list.querySelectorAll("details")].map(el => [el.dataset.group, !el.open]));
  const groups = new Map([["Размеры", []], ["Геометрические связи", []], ["Совпадения", []]]);
  for (const r of restrictions) {
    const dimension = r.type === "DISTANCE" || r.type === "ANGLE";
    const group = dimension ? "Размеры" : r.type === "COINCIDENT" ? "Совпадения" : "Геометрические связи";
    groups.get(group).push(r);
  }
  function row(r) {
    const selected = r.id === uiState.selectedRestrictionId;
    const name = RestrictionMeta[r.type].name;
    const displayName = r.type === "DISTANCE" ? "Длина" : r.type === "ANGLE" ? "Угол" : name.replace(/ (отрезков|отрезка|точки)$/, "");
    const value = r.type === "DISTANCE" ? `${Number(r.value.toFixed(2))} мм` : r.type === "ANGLE" ? `${Number(r.value.toFixed(2))}°` : "";
    const field = (label, attr, value) =>
      `<label>${label}<input type="number" step="any" value="${value}" data-${attr}="${r.id}"></label>`;
    let detail = "";
    if (selected) {
      let inputs = "";
      if (r.type === "DISTANCE" || r.type === "ANGLE") {
        inputs = field(r.type === "ANGLE" ? "Угол, °" : "Длина, мм", "edit-restriction", r.value);
      } else if (r.type === "FIX_POINT") {
        inputs = field("X, мм", "edit-fix-x", r.value) + field("Y, мм", "edit-fix-y", r.value2);
      }
      const residual = r.residual === null ? "—" : mathMarkup(numberTex(r.residual, 2));
      detail = `<div class="restriction-detail">${inputs}<footer><span>Невязка ${residual}</span>
        <button data-remove-restriction="${r.id}">Снять связь</button></footer></div>`;
    }
    return `<div class="restriction-row${selected ? " selected" : ""}"><button class="restriction-select" data-select-restriction="${r.id}" aria-expanded="${selected}">
      ${icon(r.type)}<span><strong>${displayName}</strong><small>${escapeHtml(describeRestrictionTargets(r))}</small></span><b>${value}</b></button>${detail}</div>`;
  }
  list.innerHTML = [...groups].filter(([, items]) => items.length).map(([name, items]) => {
    const closed = collapsed.get(name) ?? name === "Совпадения";
    const open = !closed || items.some(r => r.id === uiState.selectedRestrictionId);
    return `<details data-group="${name}" ${open ? "open" : ""}><summary>${icon("chevron")}${name}<small>${items.length}</small></summary>${items.map(row).join("")}</details>`;
  }).join("");
}

function renderPrimitivesBrowser() {
  const { standalonePoints, segments } = api.getAllPrimitives();
  const row = (p, value) => `<div class="object-row">${icon(p.type === "point" ? "point" : "segment")}
    <span><strong>${escapeHtml(p.label)}</strong><small>${value}</small></span>
    <button data-remove-primitive="${p.id}" aria-label="Удалить ${escapeHtml(p.label)}">Удалить</button></div>`;
  const rows = standalonePoints.map(p => row(p, `${p.x.toFixed(1)}, ${p.y.toFixed(1)} мм`));
  for (const s of segments) {
    const a = s.p1_ref, b = s.p2_ref;
    rows.push(row(s, `${escapeHtml(a.label)}–${escapeHtml(b.label)} · ${Math.hypot(a.x - b.x, a.y - b.y).toFixed(1)} мм`));
  }
  document.getElementById("primitivesList").innerHTML = rows.join("") || '<p class="empty">В эскизе пока нет объектов.</p>';
}

function updateJsonPanel() {
  const area = document.getElementById("jsonSerializerArea");
  if (document.activeElement !== area) {
    area.value = formatSketch(api.serializeSketch());
  }
}

function renderMatrixInspector() {
  const state = api.getAllPrimitives();
  const { totalVars, totalEqs } = state.dofInfo;
  const { current, variableLabels, eq_details: details, last_jacobian: A, last_rhs: b } = state.solver;
  const merged = current ? state.points.length * 2 - totalVars : 0;
  document.getElementById("matrixStatsPill").innerHTML =
    `${mathMarkup(`n=${totalVars},\\; m=${totalEqs}`)}${merged ? ` · Объединено координат: ${merged}` : ""}`;

  const eqContainer = document.getElementById("analyticalEquationsTable");
  const matContainer = document.getElementById("jacobianMatrixContainer");
  if (!current) {
    eqContainer.innerHTML = '<div class="text-xs text-slate-400 p-3">После последней правки нужен пересчёт.</div>';
    matContainer.innerHTML = '<div class="text-xs text-slate-400 p-3">Нажмите «Пересчитать» во вкладке «Эскиз».</div>';
    return;
  }
  if (details.length === 0) {
    eqContainer.innerHTML = `<div class="text-xs text-slate-400 p-3">Нет активных уравнений системы Лагранжа.</div>`;
  } else {
    let rowsHtml = "";
    for (const item of details) {
      const meta = RestrictionMeta[item.restriction.type];
      rowsHtml += `
        <tr class="border-b border-slate-800/80">
          <td class="py-2 px-3 text-amber-300">${mathMarkup(`f_{${item.eqIndex}}`)}</td>
          <td class="py-2 px-3 text-slate-200 font-medium">${meta.name} (${escapeHtml(describeRestrictionTargets(item.restriction))})</td>
          <td class="py-2 px-3 text-slate-200">${mathMarkup(item.tex)}</td>
          <td class="py-2 px-3 text-teal-200">${renderGradient(item)}</td>
          <td class="py-2 px-3 text-right ${Number.isFinite(item.f_val) && Math.abs(item.f_val) < 1e-3 ? 'text-emerald-400' : 'text-rose-400'}">${Number.isFinite(item.f_val) ? mathMarkup(numberTex(item.f_val, 2)) : "—"}</td>
        </tr>`;
    }
    eqContainer.innerHTML = `
      <table class="w-full text-left border-collapse text-xs">
        <thead>
          <tr class="border-b border-slate-700 text-slate-400 bg-slate-950/60">
            <th class="py-2 px-3">Ур.</th>
            <th class="py-2 px-3">Ограничение</th>
            <th class="py-2 px-3">Уравнение ${mathMarkup("f_j=0")}</th>
            <th class="py-2 px-3">Частные производные ${mathMarkup("\\partial f_j / \\partial(\\Delta p_i)")}</th>
            <th class="py-2 px-3 text-right">Невязка</th>
          </tr>
        </thead>
        <tbody>${rowsHtml}</tbody>
      </table>`;
  }

  // Отрисовка матрицы СЛАУ A и правой части b
  if (A.length === 0) {
    matContainer.innerHTML = `<div class="text-xs text-slate-400 p-3">Для текущих ограничений решать СЛАУ не требуется.</div>`;
    return;
  }

  const variableCount = variableLabels.length * 2;
  const colLabels = Array.from({ length: variableCount }, (_, i) => variableTex(i));
  const multiplier = j => `\\lambda_{${j + 1}}`;
  for (let j = 0; j < totalEqs; j++) colLabels.push(multiplier(j));

  let thead = `<th class="py-1.5 px-2 bg-slate-950 border border-slate-800 text-slate-400">Строка (Ур.)</th>`;
  for (const [i, tex] of colLabels.entries()) {
    const point = i < variableCount ? escapeHtml(variableLabels[Math.floor(i / 2)]) : "";
    const label = point ? `<span class="math-point-label" title="${point}">${point}</span>` : "";
    thead += `<th class="py-1.5 px-2 bg-slate-950 border border-slate-800 text-sky-300 text-right">${mathMarkup(tex)}${label}</th>`;
  }
  thead += `<th class="py-1.5 px-2 bg-slate-950 border border-slate-800 text-amber-300 text-right">Правая часть ${mathMarkup("b")}</th>`;

  let tbody = "";
  for (let r = 0; r < A.length; r++) {
    const row = new Map(A[r]);
    const variable = r < totalEqs ? multiplier(r) : `(${colLabels[r - totalEqs]})`;
    const rowLabel = mathMarkup(`\\partial F/\\partial ${variable}`);
    tbody += `<tr><td class="py-1 px-2 border border-slate-800 text-slate-300 bg-slate-950/50">${rowLabel}</td>`;
    for (let c = 0; c < A.length; c++) {
      const val = row.has(c) ? row.get(c) : 0;
      const isNonZero = Math.abs(val) > 1e-8;
      const cellClass = isNonZero ? "text-emerald-300 font-semibold bg-emerald-950/20" : "text-slate-600";
      const coefficient = val === null ? "—" : isNonZero ? val.toFixed(2) : "0";
      tbody += `<td class="py-1 px-2 border border-slate-800 text-right ${cellClass}">${coefficient}</td>`;
    }
    const bVal = Number.isFinite(b[r]) ? b[r].toFixed(2) : "—";
    tbody += `<td class="py-1 px-2 border border-slate-800 text-right text-amber-300 font-semibold bg-amber-950/15">${bVal}</td></tr>`;
  }

  matContainer.innerHTML = `
    <table class="w-full border-collapse text-[11px] mono">
      <thead><tr>${thead}</tr></thead>
      <tbody>${tbody}</tbody>
    </table>`;
}

function updateSelectionBadge() {
  const badge = document.getElementById("selectionSummaryBadge");
  const pLabels = uiState.selectedPointIds.map(id => api.findPoint(id)?.label || `P${id}`);
  const sLabels = uiState.selectedSegmentIds.map(id => api.findSegment(id)?.label || `L${id}`);
  const all = [...pLabels, ...sLabels];
  const r = api.getAllPrimitives().restrictions.find(r => r.id === uiState.selectedRestrictionId);
  badge.textContent = r ? RestrictionMeta[r.type].name : all.length ? all.join(", ") : "";
}
