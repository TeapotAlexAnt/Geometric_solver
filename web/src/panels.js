import { api } from "./editor-api.js";
import { renderSketch } from "./sketch-view.js";
import { uiState } from "./editor-state.js";
import { RestrictionMeta } from "./restriction-meta.js";

const htmlEntities = { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" };

function escapeHtml(text) {
  return text.replace(/[&<>"']/g, char => htmlEntities[char]);
}

function formatGradient(gradient, variableLabels) {
  return gradient.map(([index, value]) => {
    const axis = index % 2 === 0 ? "x" : "y";
    const label = escapeHtml(variableLabels[Math.floor(index / 2)]);
    return `∂f/∂Δ${axis}(${label}) = ${Number(value.toPrecision(5))}`;
  }).join(", ") || "0";
}

function formatSketch(json) {
  return JSON.stringify(JSON.parse(json), null, 2);
}

export function renderAll() {
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

  document.getElementById("dofValue").textContent = dof;
  document.getElementById("iterValue").textContent = res.iterations || 0;
  document.getElementById("residualValue").textContent = (res.max_residual || 0).toExponential(2);
  const dimension = res.last_jacobian.length;
  document.getElementById("slaeDimBadge").textContent = `${dimension} × ${dimension} (m=${totalEqs}, n=${totalVars})`;

  const eqBadge = document.getElementById("equilibriumBadge");
  if (res.converged || state.restrictions.length === 0) {
    eqBadge.textContent = "Равновесное";
    eqBadge.className = "font-semibold text-emerald-400";
  } else {
    eqBadge.textContent = "Неравновесное (Конфликт)";
    eqBadge.className = "font-semibold text-rose-400";
  }

  const defBadge = document.getElementById("modelDefBadge");
  if (state.points.length === 0) {
    defBadge.textContent = "(Пустой эскиз)";
  } else if (dof === 0) {
    defBadge.textContent = "(Полностью определена)";
  } else if (dof > 0) {
    defBadge.textContent = `(Недоопределена: своб. ${dof})`;
  } else {
    defBadge.textContent = `(Переопределена: избыт. ${-dof})`;
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
    return `${pLabels[0]} ∈ ${sLabels[0] || (pLabels[1] + "–" + pLabels[2])}`;
  }
  if (r.type === "FIX_POINT") {
    return `${pLabels[0]} (${r.value.toFixed(1)}, ${r.value2.toFixed(1)})`;
  }
  return pLabels.join(", ");
}

function renderRestrictionsBrowser() {
  const listEl = document.getElementById("restrictionsList");
  const restrictions = api.getAllPrimitives().restrictions;
  if (restrictions.length === 0) {
    listEl.innerHTML = `
      <div class="p-4 rounded-lg bg-slate-950/60 border border-slate-800/80 text-center text-xs text-slate-400">
        На эскиз пока не наложены геометрические ограничения.<br/>
        Выберите примитивы на холсте или нажмите любую из 9 кнопок ограничений слева.
      </div>`;
    return;
  }

  listEl.innerHTML = "";
  for (const r of restrictions) {
    const meta = RestrictionMeta[r.type];
    const card = document.createElement("div");
    card.className = "p-2.5 rounded-lg bg-slate-950/90 border border-slate-800 hover:border-slate-700 flex flex-col gap-1.5 text-xs";

    const topRow = document.createElement("div");
    topRow.className = "flex items-center justify-between gap-2";
    topRow.innerHTML = `
      <div class="flex items-center gap-2">
        <span class="w-5 h-5 rounded bg-teal-500/15 border border-teal-500/30 text-teal-300 flex items-center justify-center font-bold mono text-[11px]">${meta.badge}</span>
        <span class="font-semibold text-slate-100">${meta.name}</span>
      </div>
      <button data-remove-restriction="${r.id}" class="px-1.5 py-0.5 rounded bg-rose-500/15 hover:bg-rose-500/30 text-rose-300 text-[11px] transition" title="Снять ограничение">
        Снять ✕
      </button>
    `;
    card.appendChild(topRow);

    const targetRow = document.createElement("div");
    targetRow.className = "flex items-center justify-between text-[11px] text-slate-400";
    const resVal = Number(r.residual || 0);
    const resColor = resVal < 1e-3 ? "text-emerald-400" : "text-rose-400";
    targetRow.innerHTML = `
      <span>Объекты: <strong class="text-slate-200 mono">${escapeHtml(describeRestrictionTargets(r))}</strong></span>
      <span class="mono ${resColor}">|f|=${resVal.toExponential(1)}</span>
    `;
    card.appendChild(targetRow);

    // Редактируемые параметры прямо в браузере ограничений
    if (r.type === "DISTANCE" || r.type === "ANGLE") {
      const editRow = document.createElement("div");
      editRow.className = "flex items-center gap-2 mt-0.5 pt-1.5 border-t border-slate-800/80";
      const unit = r.type === "DISTANCE" ? "мм (d)" : "град (α°)";
      editRow.innerHTML = `
        <span class="text-[11px] text-slate-400">Параметр ${unit}:</span>
        <input type="number" step="any" value="${Number(r.value.toFixed(2))}" data-edit-restriction="${r.id}" class="w-24 px-2 py-0.5 rounded bg-slate-900 border border-slate-700 text-sky-300 mono text-xs focus:outline-none focus:border-teal-400" />
      `;
      card.appendChild(editRow);
    } else if (r.type === "FIX_POINT") {
      const editRow = document.createElement("div");
      editRow.className = "flex items-center gap-1.5 mt-0.5 pt-1.5 border-t border-slate-800/80 text-[11px]";
      editRow.innerHTML = `
        <span class="text-slate-400">x_c:</span>
        <input type="number" step="any" value="${Number(r.value.toFixed(1))}" data-edit-fix-x="${r.id}" class="w-16 px-1.5 py-0.5 rounded bg-slate-900 border border-slate-700 text-rose-300 mono text-xs" />
        <span class="text-slate-400">y_c:</span>
        <input type="number" step="any" value="${Number(r.value2.toFixed(1))}" data-edit-fix-y="${r.id}" class="w-16 px-1.5 py-0.5 rounded bg-slate-900 border border-slate-700 text-rose-300 mono text-xs" />
      `;
      card.appendChild(editRow);
    }

    listEl.appendChild(card);
  }
}

function renderPrimitivesBrowser() {
  const listEl = document.getElementById("primitivesList");
  const pts = api.getAllPrimitives().standalonePoints;
  const segs = api.getAllPrimitives().segments;
  if (pts.length === 0 && segs.length === 0) {
    listEl.innerHTML = `<div class="p-4 rounded-lg bg-slate-950/60 border border-slate-800 text-center text-xs text-slate-400">Эскиз пуст. Создайте точки или отрезки.</div>`;
    return;
  }

  let html = "";
  for (const p of pts) {
    html += `
      <div class="p-2 rounded-lg bg-slate-950/90 border border-slate-800 flex items-center justify-between gap-2 text-xs">
        <div class="flex items-center gap-2">
          <span class="px-1.5 py-0.5 rounded bg-sky-500/15 text-sky-300 mono font-bold">${escapeHtml(p.label)}</span>
          <span class="mono text-[11px] text-slate-300">(${p.x.toFixed(1)}, ${p.y.toFixed(1)})</span>
        </div>
        <button data-remove-primitive="${p.id}" class="px-1.5 py-0.5 rounded bg-rose-500/15 hover:bg-rose-500/30 text-rose-300 text-[11px]">Удалить</button>
      </div>`;
  }
  for (const s of segs) {
    const len = (s.p1_ref && s.p2_ref) ? Math.hypot(s.p2_ref.x - s.p1_ref.x, s.p2_ref.y - s.p1_ref.y) : 0;
    const p1Label = s.p1_ref ? s.p1_ref.label : `P${s.p1_id}`;
    const p2Label = s.p2_ref ? s.p2_ref.label : `P${s.p2_id}`;
    html += `
      <div class="p-2 rounded-lg bg-slate-950/90 border border-slate-800 flex items-center justify-between gap-2 text-xs">
        <div>
          <span class="px-1.5 py-0.5 rounded bg-indigo-500/15 text-indigo-300 mono font-bold mr-1.5">${escapeHtml(s.label)}</span>
          <span class="mono text-[11px] text-slate-300">${escapeHtml(p1Label)}–${escapeHtml(p2Label)} (L=${len.toFixed(1)} мм)</span>
        </div>
        <button data-remove-primitive="${s.id}" class="px-1.5 py-0.5 rounded bg-rose-500/15 hover:bg-rose-500/30 text-rose-300 text-[11px]">Удалить</button>
      </div>`;
  }
  listEl.innerHTML = html;
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
  const { variableLabels, eq_details: details, last_jacobian: A, last_rhs: b } = state.solver;
  const merged = state.points.length * 2 - totalVars;
  document.getElementById("matrixStatsPill").textContent =
    `n = ${totalVars}, m = ${totalEqs}${merged ? ` · Координат объединено: ${merged}` : ""}`;

  const eqContainer = document.getElementById("analyticalEquationsTable");
  if (details.length === 0) {
    eqContainer.innerHTML = `<div class="text-xs text-slate-400 p-3">Нет активных уравнений системы Лагранжа.</div>`;
  } else {
    let rowsHtml = "";
    for (const item of details) {
      const meta = RestrictionMeta[item.restriction.type];
      rowsHtml += `
        <tr class="border-b border-slate-800/80">
          <td class="py-2 px-3 mono text-amber-300 font-semibold">f_${item.eqIndex}</td>
          <td class="py-2 px-3 text-slate-200 font-medium">${meta.name} (${escapeHtml(describeRestrictionTargets(item.restriction))})</td>
          <td class="py-2 px-3 mono text-slate-300">${meta.formula}</td>
          <td class="py-2 px-3 mono text-teal-300">${formatGradient(item.df_row, variableLabels)}</td>
          <td class="py-2 px-3 mono text-right ${Math.abs(item.f_val) < 1e-3 ? 'text-emerald-400' : 'text-rose-400'}">${item.f_val.toExponential(2)}</td>
        </tr>`;
    }
    eqContainer.innerHTML = `
      <table class="w-full text-left border-collapse text-xs">
        <thead>
          <tr class="border-b border-slate-700 text-slate-400 bg-slate-950/60">
            <th class="py-2 px-3">Ур.</th>
            <th class="py-2 px-3">Ограничение</th>
            <th class="py-2 px-3">Аналитическое уравнение f_j = 0</th>
            <th class="py-2 px-3">Аналитические частные производные ∂f_j/∂Δp_i</th>
            <th class="py-2 px-3 text-right">Невязка f_j</th>
          </tr>
        </thead>
        <tbody>${rowsHtml}</tbody>
      </table>`;
  }

  // Отрисовка матрицы СЛАУ A и правой части b
  const matContainer = document.getElementById("jacobianMatrixContainer");
  if (A.length === 0) {
    matContainer.innerHTML = `<div class="text-xs text-slate-400 p-3">Для текущих ограничений решать СЛАУ не требуется.</div>`;
    return;
  }

  const colLabels = [];
  for (const label of variableLabels) {
    colLabels.push(`Δx(${label})`);
    colLabels.push(`Δy(${label})`);
  }
  for (let j = 1; j <= totalEqs; j++) {
    colLabels.push(`λ_${j}`);
  }

  let thead = `<th class="py-1.5 px-2 bg-slate-950 border border-slate-800 text-slate-400">Строка (Ур.)</th>`;
  for (const lbl of colLabels) {
    thead += `<th class="py-1.5 px-2 bg-slate-950 border border-slate-800 text-sky-300 text-right">${escapeHtml(lbl)}</th>`;
  }
  thead += `<th class="py-1.5 px-2 bg-slate-950 border border-slate-800 text-amber-300 text-right">Правая часть b</th>`;

  let tbody = "";
  for (let r = 0; r < A.length; r++) {
    const row = new Map(A[r]);
    const rowLabel = r < totalEqs ? `∂F/∂λ_${r + 1}` : `∂F/∂${colLabels[r - totalEqs]}`;
    tbody += `<tr><td class="py-1 px-2 border border-slate-800 font-semibold text-slate-300 bg-slate-950/50">${escapeHtml(rowLabel)}</td>`;
    for (let c = 0; c < A.length; c++) {
      const val = row.get(c) ?? 0;
      const isNonZero = Math.abs(val) > 1e-8;
      const cellClass = isNonZero ? "text-emerald-300 font-semibold bg-emerald-950/20" : "text-slate-600";
      tbody += `<td class="py-1 px-2 border border-slate-800 text-right ${cellClass}">${isNonZero ? val.toFixed(2) : "0"}</td>`;
    }
    const bVal = b[r] || 0;
    tbody += `<td class="py-1 px-2 border border-slate-800 text-right text-amber-300 font-semibold bg-amber-950/15">${bVal.toFixed(2)}</td></tr>`;
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
  badge.textContent = all.length ? `Выделено: ${all.join(", ")}` : "Выделено: нет (клик по точке/отрезку)";
}
