import { api } from "./editor-api.js";
import { uiState } from "./editor-state.js";
import { RestrictionMeta } from "./restriction-meta.js";
import { mathMarkup } from "./math.js";
import { closedProfiles } from "./sketch-profiles.js";
import { positionDimensionEditor } from "./dimension-editor.js";
import { icon } from "./icons.js";

const ns = "http://www.w3.org/2000/svg";
export const sketch = document.getElementById("cadSketch");
const grid = document.getElementById("sketchGrid");
const geometry = document.getElementById("sketchGeometry");
const rulers = document.getElementById("sketchRulers");
const viewport = { width: 0, height: 0 };
const profilesBySnapshot = new WeakMap();
const ink = "#e9edf4";
const relationInk = "#94c6eb";
const accent = "#ffc66e";

function svg(tag, attributes = {}, text) {
  const element = document.createElementNS(ns, tag);
  for (const [name, value] of Object.entries(attributes)) element.setAttribute(name, value);
  if (text !== undefined) element.textContent = text;
  return element;
}

function line(a, b, stroke, width = 1, attributes = {}) {
  return svg("line", { x1: a.x, y1: a.y, x2: b.x, y2: b.y, stroke, "stroke-width": width, ...attributes });
}

function label(x, y, text, attributes = {}) {
  return svg("text", { x, y, fill: ink, "font-size": 11, ...attributes }, text);
}

function origin() {
  return { x: viewport.width / 2 + uiState.camera.x, y: viewport.height / 2 + uiState.camera.y };
}

function worldToScreen({ x, y }) {
  const center = origin();
  return { x: center.x + x * uiState.camera.scale, y: center.y - y * uiState.camera.scale };
}

export function screenToWorld(sx, sy, snap = uiState.snapToGrid) {
  const center = origin();
  let x = (sx - center.x) / uiState.camera.scale;
  let y = (center.y - sy) / uiState.camera.scale;
  if (snap) {
    x = Math.round(x / 10) * 10;
    y = Math.round(y / 10) * 10;
  }
  return { x, y };
}

export function zoomBy(factor, sx = viewport.width / 2, sy = viewport.height / 2) {
  const camera = uiState.camera;
  const center = origin();
  const scale = Math.max(0.25, Math.min(8, camera.scale * factor));
  camera.x += (sx - center.x) * (1 - scale / camera.scale);
  camera.y += (sy - center.y) * (1 - scale / camera.scale);
  camera.scale = scale;
  renderSketch();
}

export function fitSketch() {
  const { width, height } = viewport;
  if (!width || !height) return;
  const { points } = api.getAllPrimitives();
  uiState.camera = { x: 0, y: 0, scale: 1.8 };
  if (points.length) {
    let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
    for (const p of points) {
      minX = Math.min(minX, p.x);
      maxX = Math.max(maxX, p.x);
      minY = Math.min(minY, p.y);
      maxY = Math.max(maxY, p.y);
    }
    const scale = Math.max(0.25, Math.min(5, (width - 100) * 0.76 / Math.max(1, maxX - minX),
      (height - 95) * 0.72 / Math.max(1, maxY - minY)));
    uiState.camera = { x: -(minX + maxX) / 2 * scale + 8, y: (minY + maxY) / 2 * scale + 6, scale };
  }
  renderSketch();
}

export function observeSketchSize() {
  new ResizeObserver(([{ contentRect: { width, height } }]) => {
    if (!width || !height) return;
    const first = !viewport.width;
    Object.assign(viewport, { width, height });
    sketch.setAttribute("viewBox", `0 0 ${width} ${height}`);
    if (first) fitSketch();
    else renderSketch();
  }).observe(sketch);
}

function drawGrid(frame) {
  const { width, height } = viewport;
  const { x, y } = origin();
  const scale = uiState.camera.scale;
  const unit = scale >= 1.25 ? 10 : scale >= 0.5 ? 25 : 50;
  const step = unit * scale;
  for (const [key, value] of Object.entries({ x: x % step, y: y % step, width: step, height: step })) {
    grid.setAttribute(key, value);
  }

  const dash = { "stroke-dasharray": "7 3 1 3" };
  frame.append(line({ x, y: 0 }, { x, y: height }, "#52636e", 1, dash),
    line({ x: 0, y }, { x: width, y }, "#55616e", 1, dash));
  const ticks = document.createDocumentFragment();
  ticks.append(svg("path", { d: `M0 0H${width}V19H19V${height}H0Z`, fill: "#202630" }));
  for (const [horizontal, start, limit] of [[true, x, width], [false, y, height]]) {
    const majorUnit = [10, 20, 50, 100, 200, 500].find(size => size >= unit && size * scale >= 52);
    const spacing = majorUnit / unit;
    const first = Math.ceil((20 - start) / step);
    for (let pos = 20 + ((start - 20) % step + step) % step, i = 0; pos < limit; pos += step, i++) {
      const n = first + i, major = n % spacing === 0;
      ticks.append(svg("path", { d: horizontal ? `M${pos} 19v-${major ? 6 : 3}` : `M19 ${pos}h-${major ? 6 : 3}`, stroke: "#586675" }));
      if (major) ticks.append(label(horizontal ? pos + 3 : 0, horizontal ? 11 : 0,
        (horizontal ? n : -n) * unit, { "font-size": 9, fill: "#acb7c3",
          ...(!horizontal && { transform: `translate(11 ${pos - 3}) rotate(-90)` }) }));
    }
  }
  ticks.append(label(2, 12, "мм", { "font-size": 9 }));
  rulers.replaceChildren(ticks);
}

function midpoint(a, b) {
  return { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
}

// Lay out screen-space labels independently of model coordinates and zoom.
function placeLabel(preferred, width, occupied, maxRings) {
  const height = 20;
  // Search around the anchor before pushing a stack of coincident links farther out.
  const free = (x, y) => !occupied.some(box => Math.abs(x - box.x) < (width + box.width) / 2 + 3 &&
    Math.abs(y - box.y) < (height + box.height) / 2 + 2);
  if (free(preferred.x, preferred.y)) {
    occupied.push({ ...preferred, width, height });
    return preferred;
  }
  for (let ring = 1; ring <= maxRings; ring++) {
    for (const [dx, dy] of [[1, 0], [-1, 0], [0, -1], [0, 1], [1, -1], [-1, 1]]) {
      const x = preferred.x + dx * (width + 4) * ring;
      const y = preferred.y + dy * (height + 3) * ring;
      if (!free(x, y)) continue;
      occupied.push({ x, y, width, height });
      return { x, y };
    }
  }
}

// Project the two direction vectors onto their intersection so the arc measures
// the same directed segments as the solver, including when an extension is needed.
function drawAngle(group, [a, b, c, d]) {
  const u = { x: b.x - a.x, y: b.y - a.y }, v = { x: d.x - c.x, y: d.y - c.y };
  const cross = u.x * v.y - u.y * v.x;
  const lengthU = Math.hypot(u.x, u.y), lengthV = Math.hypot(v.x, v.y);
  const fallback = midpoint(midpoint(a, b), midpoint(c, d));
  if (Math.abs(cross) < 1e-5 * lengthU * lengthV || !lengthU || !lengthV) return fallback;
  const t = ((c.x - a.x) * v.y - (c.y - a.y) * v.x) / cross;
  const vertex = { x: a.x + t * u.x, y: a.y + t * u.y };
  if (Math.hypot(vertex.x - fallback.x, vertex.y - fallback.y) > lengthU + lengthV) return fallback;
  const radius = Math.max(18, Math.min(58, lengthU * .33, lengthV * .33));
  const start = { x: vertex.x + radius * u.x / lengthU, y: vertex.y + radius * u.y / lengthU };
  const end = { x: vertex.x + radius * v.x / lengthV, y: vertex.y + radius * v.y / lengthV };
  const sweep = cross > 0 ? 1 : 0;
  const dArc = `M${start.x} ${start.y}A${radius} ${radius} 0 0 ${sweep} ${end.x} ${end.y}`;
  const extension = { "stroke-dasharray": "3 3" };
  group.append(line(vertex, start, "currentColor", .8, extension), line(vertex, end, "currentColor", .8, extension),
    svg("path", { d: dArc, fill: "none", stroke: "currentColor" }),
    svg("path", { d: dArc, fill: "none", stroke: "transparent", "stroke-width": 9, class: "dim-hit" }));
  const bisector = Math.atan2(u.y, u.x) + Math.atan2(cross, u.x * v.x + u.y * v.y) / 2;
  return { x: vertex.x + (radius + 15) * Math.cos(bisector), y: vertex.y + (radius + 15) * Math.sin(bisector) };
}

// Measure the available space on the drawing, including short segments in large sketches.
function showAnnotation(r, positions, edgeLengths) {
  const scale = uiState.camera.scale;
  const [a, b, c, d] = r.p_ids.map(id => positions.get(id));
  const span = (p, q) => Math.hypot(q.x - p.x, q.y - p.y);
  if (r.type === "DISTANCE") return scale >= 0.62 && span(a, b) >= 66;
  if (r.type === "ANGLE") return scale >= 1 && Math.min(span(a, b), span(c, d)) >= 90;
  if (r.type === "FIX_POINT") return scale >= 1.1;
  if (r.type === "COINCIDENT") {
    const length = Math.min(...r.p_ids.map(id => edgeLengths.get(id) ?? Infinity));
    return scale >= 3 && length >= 100;
  }
  if (scale < 1.65) return false;
  if (r.type === "POINT_ON_LINE") return span(b, c) >= 90;
  return Math.min(span(a, b), d ? span(c, d) : Infinity) >= 80;
}

function drawAnnotation(lines, labels, r, positions, occupied, sketchCenter, edges, selected, hovered) {
  const pts = r.p_ids.map(id => positions.get(id));
  const [a, b = a] = pts;
  const dim = r.type === "DISTANCE", angle = r.type === "ANGLE";
  let anchor = midpoint(a, b);
  let preferred = { x: anchor.x + 12, y: anchor.y - 18 };
  const attributes = { class: "annotation", "data-restriction-id": r.id,
    "data-selected": selected, "data-hovered": hovered };
  const group = svg("g", attributes);
  let dimensionLine;

  if (dim) {
    const dx = b.x - a.x, dy = b.y - a.y, length = Math.hypot(dx, dy);
    const nx = length ? -dy / length : 0, ny = length ? dx / length : 1;
    let side = (anchor.x - sketchCenter.x) * nx + (anchor.y - sketchCenter.y) * ny < 0 ? -1 : 1;
    const offset = (point, distance) => ({ x: point.x + nx * side * distance, y: point.y + ny * side * distance });
    // Prefer the outside of the profile, unless another edge crosses the dimension there.
    const crosses = direction => edges.filter(([p, q]) => {
      const signed = point => (point.x - a.x) * nx + (point.y - a.y) * ny - direction * 36;
      const sp = signed(p), sq = signed(q);
      if (sp * sq >= 0) return false;
      const fraction = sp / (sp - sq);
      const along = ((p.x + fraction * (q.x - p.x) - a.x) * dx +
        (p.y + fraction * (q.y - p.y) - a.y) * dy) / (length || 1);
      return along > 2 && along < length - 2;
    }).length;
    if (crosses(side) > crosses(-side)) side = -side;
    const start = offset(a, 36), end = offset(b, 36);
    anchor = midpoint(start, end);
    preferred = anchor;
    dimensionLine = line(start, end, "currentColor");
    group.append(line(a, offset(a, 42), "currentColor", 0.8), line(b, offset(b, 42), "currentColor", 0.8),
      dimensionLine, line(start, end, "transparent", 9, { class: "dim-hit" }));
    if (length > 12) {
      for (const [point, sign] of [[start, 1], [end, -1]]) {
        const tx = dx / length * sign * 7, ty = dy / length * sign * 7;
        group.append(svg("path", { d: `M${point.x} ${point.y}l${tx + nx * 1.7} ${ty + ny * 1.7}l${-nx * 3.4} ${-ny * 3.4}Z`, fill: "currentColor" }));
      }
    }
  } else if (angle) {
    anchor = drawAngle(group, pts);
    preferred = anchor;
  } else if (r.type === "PARALLEL" && pts.length >= 4) {
    anchor = midpoint(pts[2], pts[3]);
    preferred = { x: anchor.x + 20, y: anchor.y + 15 };
  } else if (r.type === "PERPENDICULAR") {
    // Mark the corner when the constrained segments meet.
    const corner = pts.slice(0, 2).find(p => pts.slice(2).some(q => Math.hypot(p.x - q.x, p.y - q.y) < 1));
    if (corner) {
      const far = (p, q) => Math.hypot(p.x - corner.x, p.y - corner.y) > 1 ? p : q;
      const toward = p => {
        const length = Math.max(1, Math.hypot(p.x - corner.x, p.y - corner.y));
        return { x: (p.x - corner.x) / length * 14, y: (p.y - corner.y) / length * 14 };
      };
      const u = toward(far(a, b)), v = toward(far(pts[2], pts[3]));
      anchor = { x: corner.x + u.x + v.x, y: corner.y + u.y + v.y };
      group.append(svg("path", { d: `M${corner.x + u.x} ${corner.y + u.y}L${anchor.x} ${anchor.y}l${-u.x} ${-u.y}`, fill: "none", stroke: "currentColor" }));
      preferred = { x: anchor.x + u.x + v.x, y: anchor.y + u.y + v.y };
    }
  } else if (r.type === "POINT_ON_LINE" || r.type === "FIX_POINT" || r.type === "COINCIDENT") {
    anchor = a;
    preferred = r.type === "FIX_POINT" ? { x: a.x - 18, y: a.y + 24 } : { x: a.x - 17, y: a.y - 15 };
  }
  const value = dim || angle ? Number(r.value.toFixed(2)) + (angle ? "°" : "") : "";
  const width = value ? Math.max(36, value.length * 8 + 12) : 21;
  if (preferred.x < -width || preferred.x > viewport.width + width ||
      preferred.y < -20 || preferred.y > viewport.height + 20) {
    // A long dimension can cross the viewport even with its number off screen.
    if (group.childElementCount) lines.append(group);
    return;
  }
  const at = placeLabel(preferred, width, occupied, selected ? Infinity : uiState.camera.scale >= 3 ? 2 : 1);
  if (!at) return;
  if (dimensionLine && at.x === anchor.x && at.y === anchor.y) {
    // Interrupt the dimension stroke at its reading, leaving the underlying profile visible.
    const length = Math.hypot(b.x - a.x, b.y - a.y);
    if (length) {
      const ux = (b.x - a.x) / length, uy = (b.y - a.y) / length;
      const gap = Math.min(width / (2 * Math.abs(ux)), 10 / Math.abs(uy), length / 2);
      const start = { x: anchor.x - ux * length / 2, y: anchor.y - uy * length / 2 };
      const end = { x: anchor.x + ux * length / 2, y: anchor.y + uy * length / 2 };
      dimensionLine.replaceWith(line(start, { x: anchor.x - ux * gap, y: anchor.y - uy * gap }, "currentColor"),
        line({ x: anchor.x + ux * gap, y: anchor.y + uy * gap }, end, "currentColor"));
    }
  }
  if (group.childElementCount) lines.append(group);
  if (Math.hypot(at.x - anchor.x, at.y - anchor.y) > 26) {
    group.append(line(anchor, at, "currentColor", 0.8));
    if (!group.parentNode) lines.append(group);
  }
  const mark = svg("g", attributes);
  mark.append(svg("title", {}, value ? "Двойной щелчок — изменить размер" : RestrictionMeta[r.type].name));
  const box = svg("g", { transform: `translate(${at.x} ${at.y})`, class: value ? "annotation-label dimension" : "annotation-label" });
  box.append(svg("rect", { x: -width / 2, y: -10, width, height: 20, rx: 3 }));
  if (value) {
    const formula = svg("foreignObject", { x: -width / 2, y: -10, width, height: 20 });
    const number = document.createElement("span");
    number.className = "sketch-math-badge";
    const amount = Number(r.value.toFixed(2));
    number.innerHTML = mathMarkup(angle ? `${amount}^{\\circ}` : String(amount));
    formula.append(number);
    box.append(formula);
  } else {
    const glyph = svg("g", { transform: "translate(-7 -7)" });
    glyph.innerHTML = icon(r.type);
    glyph.firstElementChild.setAttribute("width", 14);
    glyph.firstElementChild.setAttribute("height", 14);
    box.append(glyph);
  }
  mark.append(box);
  labels.append(mark);
}

export function renderSketch() {
  if (!viewport.width || !viewport.height) return;
  const snapshot = api.getAllPrimitives();
  const { points, segments, restrictions } = snapshot;
  const box = uiState.marquee;
  const hits = box?.hits;
  const keepSelection = !hits || box.additive;
  const selectedSegments = new Set(keepSelection ? uiState.selectedSegmentIds : []);
  const selectedPoints = new Set(keepSelection ? uiState.selectedPointIds : []);
  const crossing = box && box.x < box.startX;
  const previewInk = crossing ? "#84d5ba" : "#91c4ff";
  const selected = hits ? null : restrictions.find(r => r.id === uiState.selectedRestrictionId);
  const hovered = hits ? null : restrictions.find(r => r.id === uiState.hoveredRestrictionId);
  sketch.dataset.pickAnnotations = uiState.activeTool === "select" && !uiState.activeConstraintTool;
  document.getElementById("zoomLabel").textContent = `${Math.round(uiState.camera.scale / 1.8 * 100)}%`;
  const frame = document.createDocumentFragment();
  drawGrid(frame);
  const positions = new Map(points.map(point => [point.id, worldToScreen(point)]));
  let profiles = profilesBySnapshot.get(snapshot);
  if (!profiles) {
    profiles = closedProfiles(segments, snapshot.solver.point_groups);
    profilesBySnapshot.set(snapshot, profiles);
  }
  const outline = profiles.map(ids => "M" + ids.map(id => {
    const { x, y } = positions.get(id);
    return `${x} ${y}`;
  }).join("L") + "Z").join(" ");
  if (outline) frame.append(svg("path", { d: outline, class: "closed-profile", "fill-rule": "evenodd" }));
  const occupied = [];
  const edges = [];
  const edgeLengths = new Map();
  for (const s of segments) {
    const a = positions.get(s.p1_id), b = positions.get(s.p2_id);
    edges.push([a, b]);
    const pixels = Math.hypot(b.x - a.x, b.y - a.y);
    edgeLengths.set(s.p1_id, pixels);
    edgeLengths.set(s.p2_id, pixels);
    const inBox = hits?.has(s.id);
    const highlight = inBox ? previewInk : accent;
    const active = inBox || selectedSegments.has(s.id) || selected?.seg_ids.includes(s.id);
    const hover = !hits && (uiState.hoveredSegmentId === s.id || hovered?.seg_ids.includes(s.id));
    const g = svg("g", { "data-primitive-id": s.id, "data-primitive-type": "line", "data-selected": !!active });
    g.append(line(a, b, active ? highlight : hover ? relationInk : ink, active || hover ? 3 : 2));
    const mid = midpoint(a, b);
    if (active || hover || (pixels >= 110 && uiState.camera.scale >= 2.4)) {
      g.append(label(mid.x + 4, mid.y - 9, s.label, { class: "primitive-label", fill: active ? highlight : "#a7b5c3" }));
    }
    frame.append(g);
  }
  const pending = uiState.segmentStart;
  if (uiState.activeTool === "segment" && pending) {
    const end = positions.get(uiState.hoveredPointId) ?? worldToScreen(uiState.mouseWorld);
    frame.append(line(positions.get(pending.id) ?? worldToScreen(pending), end, accent, 1.8, {
      "stroke-dasharray": "6 4", "data-segment-preview": ""
    }));
  }
  for (const p of points) {
    const { x, y } = positions.get(p.id);
    const inBox = hits?.has(p.id);
    const highlight = inBox ? previewInk : accent;
    const picked = inBox || selectedPoints.has(p.id);
    const active = picked || selected?.p_ids.includes(p.id);
    const hover = !hits && (uiState.hoveredPointId === p.id || hovered?.p_ids.includes(p.id));
    const g = svg("g", { "data-primitive-id": p.id, "data-primitive-type": p.type, "data-selected": !!active });
    if (active || hover) g.append(svg("circle", {
      cx: x, cy: y, r: 7, fill: "none", stroke: active ? highlight : relationInk, "stroke-width": 1
    }));
    const style = { fill: active ? highlight : hover ? "#ffd08c" : "#27303b", stroke: active ? highlight : ink, "stroke-width": 1.5 };
    const markerSize = uiState.camera.scale < 1.3 && !active && !hover ? 1.5 : 2.7;
    g.append(p.type === "endpoint" ? svg("rect", { x: x - markerSize, y: y - markerSize, width: markerSize * 2, height: markerSize * 2, ...style }) :
      svg("circle", { cx: x, cy: y, r: 3.5, ...style }));
    if ((!hits && uiState.hoveredPointId === p.id) || picked || (p.type === "point" && uiState.camera.scale >= 1.3)) {
      g.append(label(x + 7, y - 8, p.label, { class: "primitive-label" }));
    }
    frame.append(g);
    occupied.push({ x, y, width: 10, height: 10 });
  }
  const center = points.length ? {
    x: points.reduce((sum, p) => sum + positions.get(p.id).x, 0) / points.length,
    y: points.reduce((sum, p) => sum + positions.get(p.id).y, 0) / points.length
  } : origin();
  const labels = document.createDocumentFragment();
  const isDimension = r => r.type === "DISTANCE" || r.type === "ANGLE";
  // Give dimensions their nominal placement and paint every label above the hit lines.
  const visible = restrictions.filter(r => r !== selected && showAnnotation(r, positions, edgeLengths));
  const ordered = [...visible.filter(isDimension), ...visible.filter(r => !isDimension(r))];
  if (selected) ordered.unshift(selected);
  for (const r of ordered) {
    drawAnnotation(frame, labels, r, positions, occupied, center, edges, r === selected, r === hovered);
  }
  frame.append(labels);
  if (hits) {
    frame.append(svg("rect", {
      class: crossing ? "selection-box crossing" : "selection-box",
      x: Math.min(box.startX, box.x), y: Math.min(box.startY, box.y),
      width: Math.abs(box.x - box.startX), height: Math.abs(box.y - box.startY)
    }));
  }
  geometry.replaceChildren(frame);
  positionDimensionEditor();
}
