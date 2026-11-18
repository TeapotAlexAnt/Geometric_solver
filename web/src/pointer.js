import { api } from "./editor-api.js";
import { sketch, renderSketch, screenToWorld } from "./sketch-view.js";
import { uiState } from "./editor-state.js";
import { renderAll } from "./panels.js";
import { tryApplyConstraintFromSelection } from "./tools.js";

export function bindPointerEvents() {
  sketch.addEventListener("pointerdown", (e) => {
    if (!e.isPrimary) return;
    const rect = sketch.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;

    if (e.button === 1 || e.button === 2 || e.altKey) {
      sketch.setPointerCapture(e.pointerId);
      uiState.isPanning = true;
      uiState.panLastScreen = { x: sx, y: sy };
      return;
    }
    if (e.button !== 0) return;

    const world = screenToWorld(sx, sy);
    const hitRadiusWorld = 11 / uiState.camera.scale;
    const hitPt = api.queryPoint(world.x, world.y, hitRadiusWorld);
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
      let pt = hitPt;
      if (!pt) {
        pt = api.findPoint(api.createPoint(world.x, world.y));
      }
      if (uiState.segmentStartPointId === null) {
        uiState.segmentStartPointId = pt.id;
      } else if (uiState.segmentStartPointId !== pt.id) {
        api.createSegment(uiState.segmentStartPointId, pt.id);
        uiState.segmentStartPointId = null;
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
      if (!e.shiftKey) {
        uiState.selectedPointIds = [];
        uiState.selectedSegmentIds = [];
      }
      uiState.isPanning = true;
      uiState.panLastScreen = { x: sx, y: sy };
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

    const hitPt = api.queryPoint(world.x, world.y, 11 / uiState.camera.scale);
    const hitSeg = !hitPt ? api.querySegment(world.x, world.y, 8 / uiState.camera.scale) : null;
    const pointId = hitPt?.id ?? null;
    const segmentId = hitSeg?.id ?? null;
    const hoverChanged = pointId !== uiState.hoveredPointId || segmentId !== uiState.hoveredSegmentId;
    uiState.hoveredPointId = pointId;
    uiState.hoveredSegmentId = segmentId;
    if (hoverChanged || uiState.segmentStartPointId !== null) renderSketch();
  });

  function endDrag(e) {
    if (!e.isPrimary) return;
    uiState.draggingPointId = null;
    uiState.draggingSegmentId = null;
    uiState.isPanning = false;
  }

  sketch.addEventListener("pointerup", endDrag);
  sketch.addEventListener("pointercancel", endDrag);
  sketch.addEventListener("lostpointercapture", endDrag);
  sketch.addEventListener("pointerleave", () => {
    uiState.hoveredPointId = null;
    uiState.hoveredSegmentId = null;
    renderSketch();
  });

  sketch.addEventListener("contextmenu", e => e.preventDefault());

  sketch.addEventListener("wheel", (e) => {
    e.preventDefault();
    const factor = e.deltaY < 0 ? 1.12 : 0.89;
    uiState.camera.scale = Math.max(0.25, Math.min(8.0, uiState.camera.scale * factor));
    document.getElementById("zoomLabel").textContent = `${Math.round((uiState.camera.scale / 1.8) * 100)}%`;
    renderSketch();
  }, { passive: false });
}
