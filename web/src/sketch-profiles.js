// Closed, unbranched profiles share endpoints through the solver's coincidence groups.
// Coordinates alone cannot distinguish a connection from two overlapping loose ends.
export function closedProfiles(segments, groups) {
  const vertex = new Map();
  for (const ids of groups) for (const id of ids) vertex.set(id, ids[0]);
  const neighbors = new Map();
  const connect = (a, b) => {
    if (!neighbors.has(a)) neighbors.set(a, []);
    neighbors.get(a).push(b);
  };
  for (const segment of segments) {
    const a = vertex.get(segment.p1_id), b = vertex.get(segment.p2_id);
    if (a === b) continue;
    connect(a, b);
    connect(b, a);
  }

  // A loose extension at a corner does not open the contour.
  const tips = [...neighbors.keys()].filter(id => neighbors.get(id).length === 1);
  while (tips.length) {
    const id = tips.pop();
    const [other] = neighbors.get(id) ?? [];
    neighbors.delete(id);
    if (!neighbors.has(other)) continue;
    const remaining = neighbors.get(other).filter(n => n !== id);
    neighbors.set(other, remaining);
    if (remaining.length === 1) tips.push(other);
  }

  const contours = [];
  const visited = new Set();
  for (const start of neighbors.keys()) {
    let current = start, previous = null;
    const path = [];
    while (!visited.has(current) && neighbors.get(current)?.length === 2) {
      visited.add(current);
      path.push(current);
      const [a, b] = neighbors.get(current);
      [previous, current] = [current, a === previous ? b : a];
    }
    if (current === start && path.length >= 3) contours.push(path);
  }
  return contours;
}
