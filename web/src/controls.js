import { bindDimensionEditor } from "./dimension-editor.js";
import { api } from "./editor-api.js";
import { fitSketch, zoomBy } from "./sketch-view.js";
import { showToast, resetSelection, uiState } from "./editor-state.js";
import { renderAll, selectRestriction } from "./panels.js";
import { nameDocument, loadPresetSlide30, loadPresetRectangle, loadPresetAll9Constraints, loadPresetLinkage } from "./presets.js";
import { activateConstraintButton, setActiveTool } from "./tools.js";

function deleteSelection() {
  const { selectedRestrictionId, selectedSegmentIds, selectedPointIds } = uiState;
  if (selectedRestrictionId !== null) {
    api.removeRestriction(selectedRestrictionId);
  } else {
    const ids = [...selectedSegmentIds, ...selectedPointIds];
    if (!ids.length) return false;
    api.deletePrimitives(ids);
  }
  resetSelection();
  setActiveTool("select");
  return true;
}

export function bindControls() {
  bindDimensionEditor(renderAll);
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

  document.getElementById("autoSolveToggle").addEventListener("change", (e) => {
    api.setAutoSolve(e.target.checked);
    renderAll();
  });

  document.getElementById("btnSolveNow").addEventListener("click", () => {
    api.solveSketch();
    renderAll();
    showToast(api.getAllPrimitives().solver.converged
      ? "СЛАУ решена"
      : "Не удалось удовлетворить ограничения");
  });

  document.getElementById("btnResetView").addEventListener("click", fitSketch);
  document.getElementById("zoomIn").addEventListener("click", () => zoomBy(1.2));
  document.getElementById("zoomOut").addEventListener("click", () => zoomBy(1 / 1.2));

  document.getElementById("btnClearSketch").addEventListener("click", () => {
    api.clearSketch();
    resetSelection();
    nameDocument("Новый эскиз");
    setActiveTool("select");
    fitSketch();
    showToast("Рабочее поле очищено");
  });

  document.addEventListener("keydown", e => {
    if (document.getElementById("viewEditor").classList.contains("hidden") || e.defaultPrevented || e.isComposing) return;
    if (e.key === "Delete" || e.key === "Backspace") {
      if (e.repeat || e.ctrlKey || e.metaKey || e.altKey || e.shiftKey ||
          e.target.isContentEditable || e.target.closest("input, textarea, select, [role=textbox]") ||
          document.getElementById("sketchLibrary").matches(":popover-open") ||
          !document.getElementById("paramModal").classList.contains("hidden") ||
          uiState.draggingPointId !== null || uiState.draggingSegmentId !== null || uiState.isPanning || uiState.marquee) return;
      if (deleteSelection()) e.preventDefault();
      return;
    }
    if (e.key !== "Escape") return;
    if (uiState.marquee) {
      uiState.marquee = null;
      renderAll();
      return;
    }
    document.getElementById("paramModal").classList.add("hidden");
    uiState.pendingModalCallback = null;
    resetSelection();
    setActiveTool("select");
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
    const select = e.target.closest("[data-select-restriction]");
    if (select) selectRestriction(Number(select.dataset.selectRestriction));
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

    for (const [name, btn] of [["restrictions", rBtn], ["primitives", pBtn], ["json", jBtn]]) {
      if (name === tab) btn.setAttribute("aria-current", "true");
      else btn.removeAttribute("aria-current");
    }
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
      resetSelection();
      nameDocument("Импортированный эскиз");
      setActiveTool("select");
      fitSketch();
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

    for (const [name, id] of [["editor", "tabBtnEditor"], ["matrix", "tabBtnMatrix"], ["arch", "tabBtnArch"]]) {
      const btn = document.getElementById(id);
      if (name === tab) btn.setAttribute("aria-current", "page");
      else btn.removeAttribute("aria-current");
    }

    renderAll();
  }

  document.getElementById("tabBtnEditor").addEventListener("click", () => switchMainTab("editor"));
  document.getElementById("tabBtnMatrix").addEventListener("click", () => switchMainTab("matrix"));
  document.getElementById("tabBtnArch").addEventListener("click", () => switchMainTab("arch"));
}
