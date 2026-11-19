// A common 24-unit drafting grid keeps commands and annotation glyphs consistent.
const shapes = {
  select: '<path d="m5 3 14 10-7 1.4-3.4 6.1z"/>',
  point: '<path d="M12 2v5m0 10v5M2 12h5m10 0h5"/><circle cx="12" cy="12" r="2.5"/>',
  segment: '<path d="m5.5 18.5 13-13"/><path d="M2 17h4v4H2zM18 3h4v4h-4z"/>',
  delete: '<path d="M4 7h16M9 7V4h6v3M6.5 7l1 14h9l1-14M10 10v7m4-7v7"/>',
  COINCIDENT: '<circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="1.6" fill="currentColor"/><path d="M2 12h4m12 0h4"/>',
  DISTANCE: '<path d="M3 5v14m18-14v14M4 12h16M8 9l-4 3 4 3m8-6 4 3-4 3"/>',
  PARALLEL: '<path d="M4 19 14 4M10 20 20 5"/>',
  PERPENDICULAR: '<path d="M4 19h17M8 19V3m0 11h5v5"/>',
  VERTICAL: '<path d="M12 4v16M8 4h8M8 20h8"/>',
  HORIZONTAL: '<path d="M3 12h18M3 8v8m18-8v8"/>',
  ANGLE: '<path d="M21 19H3L17 5M11 19a8 8 0 0 0-2-5"/>',
  POINT_ON_LINE: '<path d="m3 19 6-4m6-4 6-5"/><circle cx="12" cy="13" r="3"/>',
  FIX_POINT: '<rect x="5" y="10" width="14" height="11" rx="1.5"/><path d="M8 10V7a4 4 0 0 1 8 0v3m-4 4v3"/>',
  sketch: '<path d="M3 4h18v16H3zM6 16l4-7 4 5 4-6"/>',
  matrix: '<path d="M6 3H3v18h3m12-18h3v18h-3m-10-9h8M8 8h8m-8 8h8m-5-10v12"/>',
  code: '<path d="m7 7-5 5 5 5m10-10 5 5-5 5m-4-14-2 18"/>',
  solve: '<path d="m14 2-9 12h6l-1 8 9-13h-6z"/>',
  grid: '<path d="M4 8h16M4 16h16M8 4v16m8-16v16"/>',
  fit: '<path d="M3 9V3h6m6 0h6v6m0 6v6h-6m-6 0H3v-6"/><path d="M8 8h8v8H8z"/>',
  plus: '<path d="M5 12h14m-7-7v14"/>', minus: '<path d="M5 12h14"/>',
  layers: '<path d="m2 8 10-5 10 5-10 5zM3 12l9 5 9-5M3 16l9 5 9-5"/>',
  chevron: '<path d="m8 10 4 4 4-4"/>',
  file: '<path d="M5 3h9l5 5v13H5zM14 3v6h5"/>',
  segmentExample: '<path d="m3 17 17-10"/><circle cx="3" cy="17" r="1.5"/><circle cx="20" cy="7" r="1.5"/>',
  rectangle: '<path d="M2 5h20v14H2z"/>',
  linkage: '<path d="m2 19 4-13 14-2 2 15z"/><circle cx="6" cy="6" r="1.5"/>',
  contour: '<path d="M2 7h13v11H2zM15 18l7-13"/>',
};
export function icon(name) {
  return `<svg class="icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${shapes[name]}</svg>`;
}
export function mountIcons() {
  document.querySelectorAll("[data-icon]").forEach(slot => { slot.innerHTML = icon(slot.dataset.icon); });
}
