import createGeomSolver from "@geom-solver";
import { RestrictionMeta } from "./restriction-meta.js";

let backend;
let snapshot;

export async function initEditorApi() {
  backend = await createGeomSolver();
}

function update(method, ...args) {
  const result = backend[method](...args);
  snapshot = undefined;
  return result;
}

function getAllPrimitives() {
  if (snapshot) return snapshot;

  const state = JSON.parse(backend.getAllPrimitives());
  const points = state.primitives.filter(item => item.type === "point");
  const pointById = new Map(points.map(point => [point.id, point]));
  const segments = state.primitives.filter(item => item.type === "line").map(segment => ({
    ...segment,
    p1_ref: pointById.get(segment.p1_id),
    p2_ref: pointById.get(segment.p2_id)
  }));

  const details = [];
  const { final_residuals: residuals, final_gradients: gradients } = state.solver;
  const restrictions = state.restrictions.map(item => {
    const count = RestrictionMeta[item.type].eqs;
    const row = details.length;
    const residual = Math.max(...residuals.slice(row, row + count).map(Math.abs), 0);
    const restriction = { ...item, residual };
    for (let index = row; index < row + count; index++) {
      details.push({
        restriction,
        eqIndex: index + 1,
        f_val: residuals[index],
        df_row: gradients[index]
      });
    }
    return restriction;
  });

  snapshot = {
    ...state,
    points,
    segments,
    restrictions,
    solver: { ...state.solver, eq_details: details }
  };
  return snapshot;
}

export const api = {
  getAllPrimitives,
  findPoint: id => getAllPrimitives().points.find(point => point.id === id),
  findSegment: id => getAllPrimitives().segments.find(segment => segment.id === id),
  createPoint: (x, y) => update("createPoint", x, y, `P${getAllPrimitives().points.length + 1}`),
  createSegment: (start, end) => update("createSegment", start, end, `L${getAllPrimitives().segments.length + 1}`),
  deletePrimitive: id => update("deletePrimitive", id),
  movePoint: (id, x, y) => update("movePoint", id, x, y),
  moveSegment: (id, dx, dy) => update("moveSegment", id, dx, dy),
  queryPoint: (x, y, radius) => api.findPoint(backend.queryPoint(x, y, radius)),
  querySegment: (x, y, radius) => api.findSegment(backend.querySegment(x, y, radius)),
  applyRestriction: (type, points, segments, value = 0, value2 = 0) =>
    update("applyRestriction", RestrictionMeta[type].code, points, segments, value, value2),
  updateRestriction: (id, value, value2) => update("updateRestriction", id, value, value2),
  removeRestriction: id => update("removeRestriction", id),
  solveSketch: () => update("solveSketch"),
  clearSketch: () => update("clearSketch"),
  serializeSketch: () => backend.serializeSketch(),
  loadSketch(data) {
    const error = update("loadSketch", data);
    if (error) throw new Error(error);
  }
};
