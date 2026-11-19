import { openDimensionEditor } from "./dimension-editor.js";
import { api } from "./editor-api.js";
import { sketch, renderSketch, screenToWorld, zoomBy } from "./sketch-view.js";
import { uiState } from "./editor-state.js";
import { renderAll, selectRestriction } from "./panels.js";
import { tryApplyConstraintFromSelection } from "./tools.js";

export function bindPointerEvents() {
  let lastBlankSelection = null;

  function startPan(x, y) {
    uiState.marquee = null;
    uiState.isPanning = true;
    uiState.panLastScreen = { x, y };
    sketch.classList.add("panning");
  }

  // A trackpad reports a double-click-and-hold as a native second mouse press.
  sketch.addEventListener("mousedown", e => {
    const box = uiState.marquee;
    if (e.button !== 0 || e.detail !== 2 || !box || e.shiftKey) return;
    if (lastBlankSelection?.snapshot === api.getAllPrimitives()) {
      uiState.selectedPointIds = lastBlankSelection.points;
      uiState.selectedSegmentIds = lastBlankSelection.segments;
      uiState.selectedRestrictionId = lastBlankSelection.restriction;
    }
    lastBlankSelection = null;
    startPan(box.startX, box.startY);
    renderAll();
    e.preventDefault();
  });

  sketch.addEventListener("keydown", e => {
    if (e.key === "Enter" && !e.repeat && openDimensionEditor(uiState.selectedRestrictionId)) e.preventDefault();
  });

  sketch.addEventListener("pointerdown", (e) => {
    if (!e.isPrimary) return;
    const rect = sketch.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;

    if (e.button === 1 || e.button === 2 || e.altKey) {
      sketch.setPointerCapture(e.pointerId);
      startPan(sx, sy);
      e.preventDefault();
      return;
    }
    if (e.button !== 0) return;
    sketch.focus({ preventScroll: true });
    const annotation = e.target.closest("[data-restriction-id]");
    if (annotation && uiState.activeTool === "select" && !uiState.activeConstraintTool) {
      const id = Number(annotation.dataset.restrictionId);
      // Re-rendering after selection replaces the SVG hit target between clicks.
      if (id === uiState.selectedRestrictionId && openDimensionEditor(id, { x: sx, y: sy })) {
        e.preventDefault();
        return;
      }
      selectRestriction(id, true);
      return;
    }
    const world = screenToWorld(sx, sy);
    const hitRadiusWorld = 11 / uiState.camera.scale;
    let hitPt = api.queryPoint(world.x, world.y, hitRadiusWorld);
    if (hitPt) {
      const selected = uiState.selectedPointIds;
      if ((uiState.activeConstraintTool || (uiState.activeTool === "select" && e.shiftKey)) &&
          selected.includes(hitPt.id)) {
        hitPt = api.queryPoint(world.x, world.y, hitRadiusWorld, selected) ?? hitPt;
      } else if (uiState.activeTool === "select" && !uiState.activeConstraintTool && !e.shiftKey) {
        const selectedAtHit = selected.map(api.findPoint).filter(point => point?.x === hitPt.x && point.y === hitPt.y);
        hitPt = selectedAtHit.at(-1) ?? hitPt;
      }
    }
    const hitSeg = !hitPt ? api.querySegment(world.x, world.y, 8 / uiState.camera.scale) : null;

    // Если активен режим выбора объектов для наложения ограничения
    if (uiState.activeConstraintTool) {
      if (hitPt && !uiState.selectedPointIds.includes(hitPt.id)) {
        uiState.selectedPointIds.push(hitPt.id);
      } else if (hitSeg && !uiState.selectedSegmentIds.includes(hitSeg.id)) {
        uiState.selectedSegmentIds.push(hitSeg.id);
      }
      tryApplyConstraintFromSelection(uiState.activeConstraintTool);
      renderAll();
      return;
    }

    if (uiState.activeTool === "point") {
      api.createPoint(world.x, world.y);
      renderAll();
      return;
    }

    if (uiState.activeTool === "segment") {
      const point = hitPt ?? world;
      const pending = uiState.segmentStart;
      const start = pending?.id ? api.findPoint(pending.id) : pending;
      if (!start) {
        uiState.segmentStart = point;
      } else if ((start.x !== point.x || start.y !== point.y) && api.createSegment(start, point) > 0) {
        uiState.segmentStart = null;
      }
      renderAll();
      return;
    }

    if (uiState.activeTool === "delete") {
      if (hitPt) {
        api.deletePrimitive(hitPt.id);
      } else if (hitSeg) {
        api.deletePrimitive(hitSeg.id);
      }
      renderAll();
      return;
    }

    // Режим select / drag (Pointer Usecase)
    sketch.setPointerCapture(e.pointerId);
    if (hitPt || hitSeg) uiState.selectedRestrictionId = null;
    if (hitPt) {
      if (e.shiftKey) {
        const idx = uiState.selectedPointIds.indexOf(hitPt.id);
        if (idx >= 0) uiState.selectedPointIds.splice(idx, 1);
        else uiState.selectedPointIds.push(hitPt.id);
      } else if (!uiState.selectedPointIds.includes(hitPt.id)) {
        uiState.selectedPointIds = [hitPt.id];
        if (!e.ctrlKey) uiState.selectedSegmentIds = [];
      }
      uiState.draggingPointId = hitPt.id;
    } else if (hitSeg) {
      if (e.shiftKey) {
        const idx = uiState.selectedSegmentIds.indexOf(hitSeg.id);
        if (idx >= 0) uiState.selectedSegmentIds.splice(idx, 1);
        else uiState.selectedSegmentIds.push(hitSeg.id);
      } else if (!uiState.selectedSegmentIds.includes(hitSeg.id)) {
        uiState.selectedSegmentIds = [hitSeg.id];
        if (!e.ctrlKey) uiState.selectedPointIds = [];
      }
      uiState.draggingSegmentId = hitSeg.id;
      uiState.dragLastWorld = world;
    } else {
      uiState.marquee = { startX: sx, startY: sy, x: sx, y: sy, hits: null, additive: e.shiftKey };
      return;
    }
    renderAll();
  });

  sketch.addEventListener("pointermove", (e) => {
    if (!e.isPrimary) return;
    const rect = sketch.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;
    const world = screenToWorld(sx, sy);
    uiState.mouseWorld = world;

    document.getElementById("cursorX").textContent = world.x.toFixed(1);
    document.getElementById("cursorY").textContent = world.y.toFixed(1);

    if (uiState.isPanning) {
      uiState.camera.x += sx - uiState.panLastScreen.x;
      uiState.camera.y += sy - uiState.panLastScreen.y;
      uiState.panLastScreen = { x: sx, y: sy };
      renderSketch();
      return;
    }

    if (uiState.marquee) {
      updateBox(sx, sy);
      renderSketch();
      return;
    }

    if (uiState.draggingPointId !== null) {
      api.movePoint(uiState.draggingPointId, world.x, world.y);
      renderAll();
      return;
    }

    if (uiState.draggingSegmentId !== null) {
      const dx = world.x - uiState.dragLastWorld.x;
      const dy = world.y - uiState.dragLastWorld.y;
      api.moveSegment(uiState.draggingSegmentId, dx, dy);
      uiState.dragLastWorld = world;
      renderAll();
      return;
    }

    const marker = e.target.closest("[data-restriction-id]");
    const hoveredId = marker ? Number(marker.dataset.restrictionId) : null;
    const annotationChanged = hoveredId !== uiState.hoveredRestrictionId;
    uiState.hoveredRestrictionId = hoveredId;
    const hitPt = !marker && api.queryPoint(world.x, world.y, 11 / uiState.camera.scale);
    const hitSeg = !marker && !hitPt ? api.querySegment(world.x, world.y, 8 / uiState.camera.scale) : null;
    const pointId = hitPt?.id ?? null;
    const segmentId = hitSeg?.id ?? null;
    const hoverChanged = pointId !== uiState.hoveredPointId || segmentId !== uiState.hoveredSegmentId;
    uiState.hoveredPointId = pointId;
    uiState.hoveredSegmentId = segmentId;
    if (annotationChanged || hoverChanged || uiState.segmentStart !== null) renderSketch();
  });

  function updateBox(x, y) {
    const box = uiState.marquee;
    box.x = x;
    box.y = y;
    if (!box.hits && Math.hypot(x - box.startX, y - box.startY) < 4) return;
    const start = screenToWorld(box.startX, box.startY, false);
    const end = screenToWorld(x, y, false);
    box.hits = new Set(api.queryBox(start, end));
  }

  function selectBox(box) {
    const state = api.getAllPrimitives();
    lastBlankSelection = box.hits ? null : {
      snapshot: state, points: uiState.selectedPointIds, segments: uiState.selectedSegmentIds,
      restriction: uiState.selectedRestrictionId
    };
    const points = new Set(box.additive ? uiState.selectedPointIds : []);
    const segments = new Set(box.additive ? uiState.selectedSegmentIds : []);
    if (box.hits) {
      for (const p of state.standalonePoints) if (box.hits.has(p.id)) points.add(p.id);
      for (const s of state.segments) if (box.hits.has(s.id)) segments.add(s.id);
    }
    uiState.selectedPointIds = [...points];
    uiState.selectedSegmentIds = [...segments];
    if (box.hits || !box.additive) uiState.selectedRestrictionId = null;
  }

  function endDrag(e) {
    if (!e.isPrimary) return;
    const box = uiState.marquee;
    if (box) {
      if (e.type === "pointerup") {
        const rect = sketch.getBoundingClientRect();
        updateBox(e.clientX - rect.left, e.clientY - rect.top);
        selectBox(box);
      }
      uiState.marquee = null;
      renderAll();
    }
    sketch.classList.remove("panning");
    uiState.draggingPointId = null;
    uiState.draggingSegmentId = null;
    uiState.isPanning = false;
  }

  sketch.addEventListener("pointerup", endDrag);
  sketch.addEventListener("pointercancel", endDrag);
  sketch.addEventListener("lostpointercapture", endDrag);
  sketch.addEventListener("pointerleave", () => {
    uiState.hoveredRestrictionId = null;
    uiState.hoveredPointId = null;
    uiState.hoveredSegmentId = null;
    renderSketch();
  });

  sketch.addEventListener("contextmenu", e => e.preventDefault());

  sketch.addEventListener("wheel", (e) => {
    e.preventDefault();
    if (uiState.marquee || uiState.isPanning) return;
    const factor = e.deltaY < 0 ? 1.12 : 0.89;
    const rect = sketch.getBoundingClientRect();
    zoomBy(factor, e.clientX - rect.left, e.clientY - rect.top);
  }, { passive: false });
}
