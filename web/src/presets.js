import { api } from "./editor-api.js";
import { showToast, resetSelection } from "./editor-state.js";
import { fitSketch } from "./sketch-view.js";
import { setActiveTool } from "./tools.js";

export function nameDocument(title, presetId) {
  document.getElementById("documentTitle").textContent = title;
  document.getElementById("sketchLibrary").hidePopover();
  document.querySelectorAll(".example").forEach(button => {
    if (button.id === presetId) button.setAttribute("aria-current", "true");
    else button.removeAttribute("aria-current");
  });
}

function loadPreset(sketch, presetId, notify = true) {
  api.loadSketch(sketch);
  resetSelection();
  const title = document.querySelector(`#${presetId} span`).firstChild.textContent;
  nameDocument(title, presetId);
  setActiveTool("select");
  fitSketch();
  if (notify) showToast(`Открыт эскиз «${title}»`);
}

export function loadPresetSlide30() {
  loadPreset({
    points: [
      {id: 1, x: -60, y: -20, label: "P1"},
      {id: 2, x: 35, y: 35, label: "P2"}
    ],
    segments: [
      {id: 3, p1_id: 1, p2_id: 2, label: "L1"}
    ],
    restrictions: [
      {id: 4, type: "FIX_POINT", p_ids: [1], seg_ids: [], value: -60, value2: -20},
      {id: 5, type: "DISTANCE", p_ids: [1, 2], seg_ids: [3], value: 120}
    ]
  }, "presetSlide30");
}

export function loadPresetRectangle() {
  loadPreset({
    points: [
      {id: 1, x: -90, y: -55, label: "P1"},
      {id: 2, x: 80, y: -40, label: "P2"},
      {id: 3, x: 95, y: 60, label: "P3"},
      {id: 4, x: -85, y: 50, label: "P4"}
    ],
    segments: [
      {id: 5, p1_id: 1, p2_id: 2, label: "L1"},
      {id: 6, p1_id: 2, p2_id: 3, label: "L2"},
      {id: 7, p1_id: 4, p2_id: 3, label: "L3"},
      {id: 8, p1_id: 1, p2_id: 4, label: "L4"}
    ],
    restrictions: [
      {id: 9, type: "FIX_POINT", p_ids: [1], seg_ids: [], value: -90, value2: -55},
      {id: 10, type: "HORIZONTAL", p_ids: [1, 2], seg_ids: [5]},
      {id: 11, type: "VERTICAL", p_ids: [1, 4], seg_ids: [8]},
      {id: 12, type: "PARALLEL", p_ids: [1, 2, 4, 3], seg_ids: [5, 7]},
      {id: 13, type: "PERPENDICULAR", p_ids: [1, 2, 2, 3], seg_ids: [5, 6]},
      {id: 14, type: "DISTANCE", p_ids: [1, 2], seg_ids: [5], value: 180},
      {id: 15, type: "DISTANCE", p_ids: [2, 3], seg_ids: [6], value: 110}
    ]
  }, "presetRect");
}

export function loadPresetAll9Constraints(notify = true) {
  loadPreset({
    points: [
      {id: 1, x: -120, y: -60, label: "P1"},
      {id: 2, x: 40, y: -60, label: "P2"},
      {id: 3, x: 40, y: 40, label: "P3"},
      {id: 4, x: -120, y: 40, label: "P4"},
      {id: 5, x: 45, y: -55, label: "P5"},
      {id: 6, x: 110, y: 30, label: "P6"},
      {id: 7, x: 75, y: -10, label: "P7"}
    ],
    segments: [
      {id: 8, p1_id: 1, p2_id: 2, label: "L1"},
      {id: 9, p1_id: 2, p2_id: 3, label: "L2"},
      {id: 10, p1_id: 4, p2_id: 3, label: "L3"},
      {id: 11, p1_id: 1, p2_id: 4, label: "L4"},
      {id: 12, p1_id: 5, p2_id: 6, label: "L5"}
    ],
    restrictions: [
      {id: 13, type: "FIX_POINT", p_ids: [1], seg_ids: [], value: -120, value2: -60},
      {id: 14, type: "HORIZONTAL", p_ids: [1, 2], seg_ids: [8]},
      {id: 15, type: "VERTICAL", p_ids: [1, 4], seg_ids: [11]},
      {id: 16, type: "PARALLEL", p_ids: [1, 2, 4, 3], seg_ids: [8, 10]},
      {id: 17, type: "PERPENDICULAR", p_ids: [1, 2, 2, 3], seg_ids: [8, 9]},
      {id: 18, type: "DISTANCE", p_ids: [1, 2], seg_ids: [8], value: 150},
      {id: 19, type: "DISTANCE", p_ids: [2, 3], seg_ids: [9], value: 95},
      {id: 20, type: "DISTANCE", p_ids: [5, 6], seg_ids: [12], value: 110},
      {id: 21, type: "COINCIDENT", p_ids: [2, 5], seg_ids: []},
      {id: 22, type: "ANGLE", p_ids: [1, 2, 5, 6], seg_ids: [8, 12], value: 60},
      {id: 23, type: "POINT_ON_LINE", p_ids: [7, 5, 6], seg_ids: [12]}
    ]
  }, "presetAll9", notify);
}

export function loadPresetLinkage() {
  loadPreset({
    points: [
      {id: 1, x: -100, y: -50, label: "P1"},
      {id: 2, x: -70, y: 40, label: "P2"},
      {id: 3, x: 60, y: 65, label: "P3"},
      {id: 4, x: 90, y: -50, label: "P4"}
    ],
    segments: [
      {id: 5, p1_id: 1, p2_id: 2, label: "L1"},
      {id: 6, p1_id: 2, p2_id: 3, label: "L2"},
      {id: 7, p1_id: 4, p2_id: 3, label: "L3"},
      {id: 8, p1_id: 1, p2_id: 4, label: "L4"}
    ],
    restrictions: [
      {id: 9, type: "FIX_POINT", p_ids: [1], seg_ids: [], value: -100, value2: -50},
      {id: 10, type: "FIX_POINT", p_ids: [4], seg_ids: [], value: 90, value2: -50},
      {id: 11, type: "DISTANCE", p_ids: [1, 2], seg_ids: [5], value: 95},
      {id: 12, type: "DISTANCE", p_ids: [2, 3], seg_ids: [6], value: 135},
      {id: 13, type: "DISTANCE", p_ids: [4, 3], seg_ids: [7], value: 120}
    ]
  }, "presetLinkage");
}
