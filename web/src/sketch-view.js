import { api } from "./editor-api.js";
import { uiState } from "./editor-state.js";

const namespace = "http://www.w3.org/2000/svg";
export const sketch = document.getElementById("cadSketch");
const grid = document.getElementById("sketchGrid");
const gridLines = document.getElementById("sketchGridLines");
const geometry = document.getElementById("sketchGeometry");
const viewport = { width: 0, height: 0 };

function origin() {
  return {
    x: viewport.width / 2 + uiState.camera.x,
    y: viewport.height / 2 + uiState.camera.y
  };
}

function worldToScreen({ x, y }) {
  const center = origin();
  return {
    x: center.x + x * uiState.camera.scale,
    y: center.y - y * uiState.camera.scale
  };
}

export function screenToWorld(sx, sy) {
  const center = origin();
  let x = (sx - center.x) / uiState.camera.scale;
  let y = (center.y - sy) / uiState.camera.scale;
  if (uiState.snapToGrid) {
    x = Math.round(x / 10) * 10;
    y = Math.round(y / 10) * 10;
  }
  return { x, y };
}

export function observeSketchSize() {
  const observer = new ResizeObserver(([{ contentRect }]) => {
    const { width, height } = contentRect;
    if (width === 0 || height === 0) return;
    viewport.width = width;
    viewport.height = height;
    sketch.setAttribute("viewBox", `0 0 ${width} ${height}`);
    renderSketch();
  });
  observer.observe(sketch);
}

function svg(tag, attributes, text) {
  const element = document.createElementNS(namespace, tag);
  for (const [name, value] of Object.entries(attributes)) {
    element.setAttribute(name, value);
  }
  if (text !== undefined) element.textContent = text;
  return element;
}

function line(start, end, stroke, width = 1, attributes = {}) {
  return svg("line", {
    x1: start.x, y1: start.y, x2: end.x, y2: end.y,
    stroke, "stroke-width": width, ...attributes
  });
}

function label(x, y, text, fill, attributes = {}) {
  return svg("text", { x, y, fill, "font-size": 11, ...attributes }, text);
}

function badge(x, y, text, border, fill = "#f8fafc", width = 16, height = 16) {
  const group = svg("g", {});
  group.append(
    svg("rect", {
      x: x - width / 2, y: y - height / 2, width, height,
      rx: 4, fill: "#0f172a", "fill-opacity": 0.92, stroke: border, "stroke-width": 1.2
    }),
    label(x, y + 0.5, text, fill, {
      "font-weight": 600, "font-size": 10, "text-anchor": "middle", "dominant-baseline": "central"
    })
  );
  return group;
}

export function renderSketch() {
  const { width, height } = viewport;
  if (!width || !height) return;

  const { x, y } = origin();
  const scale = uiState.camera.scale;
  const gridStep = scale > 1.1 ? 10 : (scale > 0.4 ? 25 : 50);
  const step = gridStep * scale;
  grid.setAttribute("x", x % step);
  grid.setAttribute("y", y % step);
  grid.setAttribute("width", step);
  grid.setAttribute("height", step);
  gridLines.setAttribute("d", `M ${step} 0 H 0 V ${step}`);

  const frame = document.createDocumentFragment();
  frame.append(
    line({ x: 0, y }, { x: width, y }, "#475569d9", 1.5),
    line({ x, y: 0 }, { x, y: height }, "#475569d9", 1.5),
    label(width - 18, y - 6, "X", "#64748b"),
    label(x + 6, 16, "Y", "#64748b"),
    label(x + 5, y + 14, "(0,0)", "#64748b")
  );

  const { points, segments, restrictions } = api.getAllPrimitives();
  const screenPoints = new Map(points.map(point => [point.id, worldToScreen(point)]));
  for (const restriction of restrictions) {
    if (restriction.type === "POINT_ON_LINE") {
      const [point, start] = restriction.p_ids.map(id => screenPoints.get(id));
      frame.append(line(start, point, "#22d3ee73", 1.2, { "stroke-dasharray": "4 4" }));
    }
  }
  for (const restriction of restrictions) {
    if (restriction.type === "DISTANCE") {
      const [start, end] = restriction.p_ids.map(id => screenPoints.get(id));
      drawDistance(frame, start, end, restriction.value);
    } else if (restriction.type === "ANGLE") {
      const positions = restriction.p_ids.map(id => screenPoints.get(id));
      const x = positions.reduce((sum, point) => sum + point.x, 0) / positions.length;
      const y = positions.reduce((sum, point) => sum + point.y, 0) / positions.length;
      const text = `∠ ${restriction.value.toFixed(1)}°`;
      frame.append(badge(x, y, text, "#ec4899", "#f472b6", text.length * 6 + 10, 18));
    }
  }

  const pending = uiState.segmentStart;
  const segmentStart = pending?.id ? points.find(point => point.id === pending.id) : pending;
  if (uiState.activeTool === "segment" && segmentStart) {
    const previewEnd = points.find(point => point.id === uiState.hoveredPointId) ?? uiState.mouseWorld;
    frame.append(line(worldToScreen(segmentStart), worldToScreen(previewEnd), "#38bdf8", 2, {
      "stroke-dasharray": "6 4", "data-segment-preview": ""
    }));
  }

  for (const segment of segments) drawSegment(frame, segment, screenPoints, restrictions);
  const labelOffsets = new Map();
  for (const point of points) {
    const key = `${point.x},${point.y}`;
    const offset = labelOffsets.get(key) ?? 0;
    drawPoint(frame, point, screenPoints.get(point.id), restrictions, offset);
    labelOffsets.set(key, offset + 13);
  }
  geometry.replaceChildren(frame);
}

const segmentBadges = {
  HORIZONTAL: ["H", "#10b981"],
  VERTICAL: ["V", "#10b981"],
  PARALLEL: ["∥", "#818cf8"],
  PERPENDICULAR: ["⊥", "#c084fc"]
};

function drawSegment(frame, segment, screenPoints, restrictions) {
  const start = screenPoints.get(segment.p1_id);
  const end = screenPoints.get(segment.p2_id);
  const selected = uiState.selectedSegmentIds.includes(segment.id);
  const hovered = uiState.hoveredSegmentId === segment.id;
  const group = svg("g", {
    "data-primitive-id": segment.id, "data-primitive-type": "line",
    "data-selected": selected, "stroke-linecap": "round"
  });

  if (selected || hovered) {
    group.append(line(start, end, selected ? "#38bdf859" : "#94a3b840", 9));
  }
  group.append(line(start, end, selected ? "#38bdf8" : (hovered ? "#93c5fd" : "#e2e8f0"), selected ? 3.2 : 2.4));

  const mx = (start.x + end.x) / 2;
  const my = (start.y + end.y) / 2;
  group.append(label(mx + 6, my - 6, segment.label, selected ? "#38bdf8" : "#94a3b8", {
    "font-family": "Inter, sans-serif", "font-weight": 600
  }));

  let index = 0;
  for (const restriction of restrictions) {
    const appearance = segmentBadges[restriction.type];
    if (!appearance) continue;
    const axisAligned = restriction.type === "HORIZONTAL" || restriction.type === "VERTICAL";
    const samePoints = axisAligned && restriction.p_ids[0] === segment.p1_id &&
      restriction.p_ids[1] === segment.p2_id;
    if (restriction.seg_ids.includes(segment.id) || samePoints) {
      group.append(badge(mx - 10 + index * 18, my + 14, ...appearance));
      index++;
    }
  }
  frame.append(group);
}

function drawPoint(frame, point, position, restrictions, labelOffset) {
  const { x, y } = position;
  const selected = uiState.selectedPointIds.includes(point.id);
  const hovered = uiState.hoveredPointId === point.id;
  const fixed = restrictions.some(r => r.type === "FIX_POINT" && r.p_ids[0] === point.id);
  const coincident = restrictions.some(r => r.type === "COINCIDENT" && r.p_ids.includes(point.id));
  const onLine = restrictions.some(r => r.type === "POINT_ON_LINE" && r.p_ids[0] === point.id);
  const group = svg("g", {
    "data-primitive-id": point.id, "data-primitive-type": point.type, "data-selected": selected
  });

  if (selected || hovered) {
    group.append(svg("circle", { cx: x, cy: y, r: 11, fill: selected ? "#38bdf84d" : "#94a3b840" }));
  }
  if (coincident) {
    group.append(svg("circle", { cx: x, cy: y, r: 8.5, fill: "none", stroke: "#fbbf24", "stroke-width": 1.8 }));
  }
  const endpoint = point.type === "endpoint";
  const fill = fixed ? "#f43f5e" : selected ? "#38bdf8" : endpoint ? "#a5b4fc" : "#f8fafc";
  const marker = { fill, stroke: "#0f172a", "stroke-width": 2 };
  group.append(
    endpoint
      ? svg("rect", { x: x - 5, y: y - 5, width: 10, height: 10, rx: 1, ...marker })
      : svg("circle", { cx: x, cy: y, r: 5.5, ...marker }),
    label(x + 8, y - 8 - labelOffset, `${point.label}(${point.x.toFixed(0)},${point.y.toFixed(0)})`,
      selected ? "#38bdf8" : "#cbd5e1", { "font-weight": 600 })
  );
  if (fixed) group.append(badge(x - 14, y + 14, "🔒", "#f43f5e"));
  if (onLine) group.append(badge(x + 8, y + 14, "∈", "#22d3ee"));
  frame.append(group);
}

function drawDistance(frame, start, end, distance) {
  const dx = end.x - start.x;
  const dy = end.y - start.y;
  const length = Math.hypot(dx, dy);
  if (length < 12) return;

  const nx = -dy / length;
  const ny = dx / length;
  const offset = (point, pixels) => ({ x: point.x + nx * pixels, y: point.y + ny * pixels });
  const first = offset(start, 24);
  const second = offset(end, 24);
  const text = `d = ${distance.toFixed(1)}`;

  frame.append(
    line(start, offset(start, 28), "#38bdf873"),
    line(end, offset(end, 28), "#38bdf873"),
    line(first, second, "#38bdf8", 1.4),
    badge((first.x + second.x) / 2, (first.y + second.y) / 2,
      text, "#0284c7", "#7dd3fc", text.length * 6 + 10, 18)
  );
}
