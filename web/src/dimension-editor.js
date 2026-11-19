import { api } from "./editor-api.js";
import { uiState } from "./editor-state.js";

const editor = document.getElementById("dimensionEditor");
const input = document.getElementById("dimensionValue");
const sketch = document.getElementById("cadSketch");
let editingId = null;
let fallback;

function closeEditor() {
  editingId = null;
  editor.hidden = true;
  input.setCustomValidity("");
  input.removeAttribute("aria-invalid");
}

export function positionDimensionEditor() {
  if (editingId === null) return;
  if (editingId !== uiState.selectedRestrictionId) {
    closeEditor();
    return;
  }
  const sheet = sketch.getBoundingClientRect();
  const label = sketch.querySelector(`[data-restriction-id="${editingId}"] .dimension`);
  const box = label?.getBoundingClientRect();
  const anchor = box ? { x: box.left + box.width / 2 - sheet.left, y: box.top + box.height / 2 - sheet.top } : fallback;
  if (!anchor || !sheet.width || !sheet.height) {
    closeEditor();
    return;
  }
  const left = Math.min(anchor.x - editor.offsetWidth / 2, sheet.width - editor.offsetWidth - 2);
  const top = Math.min(anchor.y - editor.offsetHeight / 2, sheet.height - editor.offsetHeight - 2);
  editor.style.left = `${Math.max(2, left)}px`;
  editor.style.top = `${Math.max(2, top)}px`;
}

export function openDimensionEditor(id, pointer) {
  if (id == null) return false;
  const r = api.getAllPrimitives().restrictions.find(item => item.id === id);
  if (!r || (r.type !== "DISTANCE" && r.type !== "ANGLE")) return false;
  editingId = id;
  fallback = pointer;
  input.value = String(r.value);
  input.setAttribute("aria-label", r.type === "ANGLE" ? "Угол в градусах" : "Длина в миллиметрах");
  document.getElementById("dimensionUnit").textContent = r.type === "ANGLE" ? "°" : "мм";
  editor.hidden = false;
  positionDimensionEditor();
  if (editor.hidden) return false;
  input.focus({ preventScroll: true });
  input.select();
  return true;
}

export function bindDimensionEditor(renderAll) {
  input.addEventListener("input", () => {
    input.setCustomValidity("");
    input.removeAttribute("aria-invalid");
  });
  editor.addEventListener("submit", e => {
    e.preventDefault();
    const r = api.getAllPrimitives().restrictions.find(item => item.id === editingId);
    if (!r || r.id !== uiState.selectedRestrictionId) {
      closeEditor();
      return;
    }
    const raw = input.value.trim().replace(",", ".");
    const value = Number(raw);
    let error = "";
    if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?$/i.test(raw) || !Number.isFinite(value)) {
      error = "Введите конечное число.";
    } else if (r.type === "DISTANCE" && value < 1 && value !== r.value) {
      error = "Минимальная длина — 1 мм.";
    }
    if (error) {
      input.setCustomValidity(error);
      input.setAttribute("aria-invalid", "true");
      input.focus();
      input.reportValidity();
      return;
    }
    if (r.value !== value) api.updateRestriction(r.id, value, r.value2);
    closeEditor();
    renderAll();
    sketch.focus({ preventScroll: true });
  });
  editor.addEventListener("keydown", e => {
    if (e.key === "Escape") {
      e.preventDefault();
      closeEditor();
      sketch.focus({ preventScroll: true });
    }
  });
  editor.addEventListener("focusout", e => {
    if (!editor.contains(e.relatedTarget)) closeEditor();
  });
  document.addEventListener("pointerdown", e => {
    if (!editor.hidden && !editor.contains(e.target)) closeEditor();
  }, true);
}
