import { api } from "./editor-api.js";
import { renderSketch } from "./sketch-view.js";
import { showToast, uiState } from "./editor-state.js";
import { renderAll } from "./panels.js";
import { loadPresetSlide30, loadPresetRectangle, loadPresetAll9Constraints, loadPresetLinkage } from "./presets.js";
import { activateConstraintButton, setActiveTool } from "./tools.js";

export function bindControls() {
  document.querySelectorAll(".tool-btn").forEach(btn => {
    btn.addEventListener("click", () => setActiveTool(btn.dataset.tool));
  });

  document.querySelectorAll(".constraint-btn").forEach(btn => {
    btn.addEventListener("click", () => activateConstraintButton(btn.dataset.constraint));
  });

  document.getElementById("presetSlide30").addEventListener("click", loadPresetSlide30);
  document.getElementById("presetRect").addEventListener("click", loadPresetRectangle);
  document.getElementById("presetAll9").addEventListener("click", loadPresetAll9Constraints);
  document.getElementById("presetLinkage").addEventListener("click", loadPresetLinkage);

  document.getElementById("snapGridToggle").addEventListener("change", (e) => {
    uiState.snapToGrid = e.target.checked;
  });

  document.getElementById("btnSolveNow").addEventListener("click", () => {
    api.solveSketch();
    renderAll();
    showToast("СЛАУ решена, геометрия эскиза обновлена");
  });

  document.getElementById("btnResetView").addEventListener("click", () => {
    uiState.camera = { x: 0, y: 0, scale: 1.8 };
    document.getElementById("zoomLabel").textContent = "100%";
    renderSketch();
  });

  document.getElementById("btnClearSketch").addEventListener("click", () => {
    api.clearSketch();
    uiState.selectedPointIds = [];
    uiState.selectedSegmentIds = [];
    uiState.segmentStartPointId = null;
    renderAll();
    showToast("Рабочее поле очищено");
  });

  // Модальное окно ввода параметров
  document.getElementById("paramModalCancel").addEventListener("click", () => {
    document.getElementById("paramModal").classList.add("hidden");
  });
  document.getElementById("paramModalCloseBtn").addEventListener("click", () => {
    document.getElementById("paramModal").classList.add("hidden");
  });
  document.getElementById("paramModalApplyBtn").addEventListener("click", () => {
    if (uiState.pendingModalCallback) uiState.pendingModalCallback();
  });
  document.getElementById("paramInput1").addEventListener("keydown", (e) => {
    if (e.key === "Enter" && uiState.pendingModalCallback) uiState.pendingModalCallback();
  });

  // Делегирование событий в браузере ограничений и примитивов
  document.getElementById("restrictionsList").addEventListener("click", (e) => {
    const btn = e.target.closest("[data-remove-restriction]");
    if (btn) {
      const rid = Number(btn.dataset.removeRestriction);
      api.removeRestriction(rid);
      renderAll();
      showToast("Ограничение снято");
    }
  });

  document.getElementById("restrictionsList").addEventListener("change", (e) => {
    const inp = e.target.closest("[data-edit-restriction]");
    if (inp) {
      const rid = Number(inp.dataset.editRestriction);
      const r = api.getAllPrimitives().restrictions.find(x => x.id === rid);
      const val = parseFloat(inp.value);
      if (r && !Number.isNaN(val)) {
        const value = r.type === "DISTANCE" ? Math.max(1, val) : val;
        api.updateRestriction(rid, value, r.value2);
        renderAll();
        showToast(`Параметр обновлен: ${value}`);
      }
    }
    const fixX = e.target.closest("[data-edit-fix-x]");
    if (fixX) {
      const rid = Number(fixX.dataset.editFixX);
      const r = api.getAllPrimitives().restrictions.find(x => x.id === rid);
      const val = parseFloat(fixX.value);
      if (r && !Number.isNaN(val)) {
        api.updateRestriction(rid, val, r.value2);
        renderAll();
      }
    }
    const fixY = e.target.closest("[data-edit-fix-y]");
    if (fixY) {
      const rid = Number(fixY.dataset.editFixY);
      const r = api.getAllPrimitives().restrictions.find(x => x.id === rid);
      const val = parseFloat(fixY.value);
      if (r && !Number.isNaN(val)) {
        api.updateRestriction(rid, r.value, val);
        renderAll();
      }
    }
  });

  document.getElementById("primitivesList").addEventListener("click", (e) => {
    const btn = e.target.closest("[data-remove-primitive]");
    if (btn) {
      const pid = Number(btn.dataset.removePrimitive);
      api.deletePrimitive(pid);
      renderAll();
      showToast("Примитив удален");
    }
  });

  // Переключение правых вкладок (Ограничения / Примитивы / JSON)
  function switchRightTab(tab) {
    const rBtn = document.getElementById("rightTabRestrictions");
    const pBtn = document.getElementById("rightTabPrimitives");
    const jBtn = document.getElementById("rightTabJson");
    const rPan = document.getElementById("panelRestrictions");
    const pPan = document.getElementById("panelPrimitives");
    const jPan = document.getElementById("panelJson");

    rPan.classList.toggle("hidden", tab !== "restrictions");
    pPan.classList.toggle("hidden", tab !== "primitives");
    jPan.classList.toggle("hidden", tab !== "json");

    rBtn.className = tab === "restrictions"
      ? "flex-1 py-1.5 px-2 text-xs font-semibold rounded-t-md bg-slate-800 text-teal-300 border-b-2 border-teal-400"
      : "flex-1 py-1.5 px-2 text-xs font-semibold rounded-t-md text-slate-400 hover:text-slate-200";
    pBtn.className = tab === "primitives"
      ? "flex-1 py-1.5 px-2 text-xs font-semibold rounded-t-md bg-slate-800 text-sky-300 border-b-2 border-sky-400"
      : "flex-1 py-1.5 px-2 text-xs font-semibold rounded-t-md text-slate-400 hover:text-slate-200";
    jBtn.className = tab === "json"
      ? "py-1.5 px-2.5 text-xs font-semibold rounded-t-md bg-slate-800 text-purple-300 border-b-2 border-purple-400 mono"
      : "py-1.5 px-2.5 text-xs font-semibold rounded-t-md text-slate-400 hover:text-slate-200 mono";
  }

  document.getElementById("rightTabRestrictions").addEventListener("click", () => switchRightTab("restrictions"));
  document.getElementById("rightTabPrimitives").addEventListener("click", () => switchRightTab("primitives"));
  document.getElementById("rightTabJson").addEventListener("click", () => switchRightTab("json"));

  document.getElementById("btnCopyJson").addEventListener("click", () => {
    const area = document.getElementById("jsonSerializerArea");
    area.select();
    document.execCommand("copy");
    showToast("JSON-представление эскиза скопировано");
  });

  document.getElementById("btnLoadJson").addEventListener("click", () => {
    try {
      const area = document.getElementById("jsonSerializerArea");
      api.loadSketch(JSON.parse(area.value));
      uiState.selectedPointIds = [];
      uiState.selectedSegmentIds = [];
      uiState.segmentStartPointId = null;
      renderAll();
      showToast("Эскиз загружен из JSON");
    } catch (err) {
      showToast("Ошибка разбора JSON: " + err.message);
    }
  });

  // Переключение главных вкладок (Рабочее поле / Матрица Якоби / Архитектура и C++)
  function switchMainTab(tab) {
    document.getElementById("viewEditor").classList.toggle("hidden", tab !== "editor");
    document.getElementById("viewMatrix").classList.toggle("hidden", tab !== "matrix");
    document.getElementById("viewArch").classList.toggle("hidden", tab !== "arch");

    document.getElementById("tabBtnEditor").className = tab === "editor"
      ? "px-3 py-1 rounded-md text-xs font-medium bg-blue-600 text-white transition"
      : "px-3 py-1 rounded-md text-xs font-medium text-slate-300 hover:text-white transition";
    document.getElementById("tabBtnMatrix").className = tab === "matrix"
      ? "px-3 py-1 rounded-md text-xs font-medium bg-blue-600 text-white transition"
      : "px-3 py-1 rounded-md text-xs font-medium text-slate-300 hover:text-white transition";
    document.getElementById("tabBtnArch").className = tab === "arch"
      ? "px-3 py-1 rounded-md text-xs font-medium bg-blue-600 text-white transition"
      : "px-3 py-1 rounded-md text-xs font-medium text-slate-300 hover:text-white transition";

    renderAll();
  }

  document.getElementById("tabBtnEditor").addEventListener("click", () => switchMainTab("editor"));
  document.getElementById("tabBtnMatrix").addEventListener("click", () => switchMainTab("matrix"));
  document.getElementById("tabBtnArch").addEventListener("click", () => switchMainTab("arch"));
}
