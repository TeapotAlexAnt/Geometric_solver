export const uiState = {
  activeTool: "select",         // 'select' | 'point' | 'segment' | 'delete'
  activeConstraintTool: null,   // null или тип ограничения при пошаговом выборе на холсте
  selectedRestrictionId: null,
  hoveredRestrictionId: null,
  selectedPointIds: [],
  selectedSegmentIds: [],
  hoveredPointId: null,
  hoveredSegmentId: null,
  segmentStart: null,
  draggingPointId: null,
  draggingSegmentId: null,
  dragLastWorld: { x: 0, y: 0 },
  marquee: null,
  isPanning: false,
  panLastScreen: { x: 0, y: 0 },
  camera: { x: 0, y: 0, scale: 1.8 }, // в пикселях на 1 мм
  mouseWorld: { x: 0, y: 0 },
  snapToGrid: false,
  pendingModalCallback: null
};

export function showToast(msg) {
  const el = document.getElementById("toastBanner");
  el.textContent = msg;
  el.classList.remove("hidden");
  clearTimeout(showToast._timer);
  showToast._timer = setTimeout(() => el.classList.add("hidden"), 3200);
}

export function resetSelection() {
  uiState.marquee = null;
  uiState.selectedRestrictionId = null;
  uiState.hoveredRestrictionId = null;
  uiState.hoveredSegmentId = null;
  uiState.hoveredPointId = null;
  uiState.selectedPointIds = [];
  uiState.selectedSegmentIds = [];
  uiState.segmentStart = null;
}
