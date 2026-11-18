import { api } from "./editor-api.js";
import { showToast, uiState } from "./editor-state.js";
import { renderAll } from "./panels.js";
import { RestrictionMeta } from "./restriction-meta.js";

export function setActiveTool(toolName) {
  uiState.activeTool = toolName;
  uiState.activeConstraintTool = null;
  uiState.segmentStartPointId = null;

  document.querySelectorAll(".tool-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.tool === toolName);
  });
  document.querySelectorAll(".constraint-btn").forEach(btn => btn.classList.remove("active"));

  const badge = document.getElementById("modePromptBadge");
  const hint = document.getElementById("modeHintText");
  if (toolName === "select") {
    badge.textContent = "Режим: Курсор (Pointer Usecase)";
    hint.textContent = "Перетаскивайте точки или отрезки мышью — решатель пересчитывает эскиз в реальном времени.";
  } else if (toolName === "point") {
    badge.textContent = "Режим: Создание точки";
    hint.textContent = "Кликните в любом месте рабочего поля для добавления новой точки P(x, y).";
  } else if (toolName === "segment") {
    badge.textContent = "Режим: Создание отрезка";
    hint.textContent = "Кликните по начальной и конечной точкам (или в пустое место для автоматического создания точек).";
  } else if (toolName === "delete") {
    badge.textContent = "Режим: Удаление объектов";
    hint.textContent = "Кликните по любой точке или отрезку для удаления примитива и связанных с ним ограничений.";
  }
  renderAll();
}

function openParamModal({ title, desc, label1, default1, label2 = null, default2 = 0 }, onConfirm) {
  const modal = document.getElementById("paramModal");
  document.getElementById("paramModalTitle").textContent = title;
  document.getElementById("paramModalDesc").textContent = desc;
  document.getElementById("paramInput1Label").textContent = label1;
  const in1 = document.getElementById("paramInput1");
  in1.value = Number(default1).toFixed(1);

  const wrap2 = document.getElementById("paramInput2Wrapper");
  const in2 = document.getElementById("paramInput2");
  if (label2) {
    wrap2.classList.remove("hidden");
    document.getElementById("paramInput2Label").textContent = label2;
    in2.value = Number(default2).toFixed(1);
  } else {
    wrap2.classList.add("hidden");
  }

  uiState.pendingModalCallback = () => {
    const v1 = parseFloat(in1.value);
    const v2 = parseFloat(in2.value);
    if (Number.isNaN(v1)) return;
    modal.classList.add("hidden");
    onConfirm(v1, Number.isNaN(v2) ? 0 : v2);
  };

  modal.classList.remove("hidden");
  in1.focus();
  in1.select();
}

export function tryApplyConstraintFromSelection(type) {
  const pts = uiState.selectedPointIds.filter(id => api.findPoint(id));
  const segs = uiState.selectedSegmentIds.filter(id => api.findSegment(id));

  if (type === "COINCIDENT" && pts.length >= 2) {
    api.applyRestriction("COINCIDENT", [pts[0], pts[1]], []);
    uiState.selectedPointIds = [];
    uiState.activeConstraintTool = null;
    document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
    showToast("Наложено ограничение: Совпадение двух точек");
    renderAll();
    return true;
  }

  if (type === "DISTANCE") {
    let p1 = null, p2 = null, sId = [];
    if (pts.length >= 2) {
      p1 = api.findPoint(pts[0]);
      p2 = api.findPoint(pts[1]);
    } else if (segs.length >= 1) {
      const seg = api.findSegment(segs[0]);
      if (seg) {
        p1 = seg.p1_ref;
        p2 = seg.p2_ref;
        sId = [seg.id];
      }
    }
    if (p1 && p2) {
      const curDist = Math.max(10, Math.hypot(p2.x - p1.x, p2.y - p1.y));
      openParamModal({
        title: "Ограничение: Расстояние (d)",
        desc: `Задайте расстояние между ${p1.label} и ${p2.label}:`,
        label1: "Расстояние d (мм):",
        default1: curDist
      }, (val) => {
        api.applyRestriction("DISTANCE", [p1.id, p2.id], sId, Math.max(1, val));
        uiState.selectedPointIds = [];
        uiState.selectedSegmentIds = [];
        uiState.activeConstraintTool = null;
        document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
        showToast(`Наложено расстояние d = ${val.toFixed(1)} мм`);
        renderAll();
      });
      return true;
    }
  }

  if ((type === "PARALLEL" || type === "PERPENDICULAR") && segs.length >= 2) {
    const s1 = api.findSegment(segs[0]);
    const s2 = api.findSegment(segs[1]);
    if (s1 && s2) {
      api.applyRestriction(type, [s1.p1_id, s1.p2_id, s2.p1_id, s2.p2_id], [s1.id, s2.id]);
      uiState.selectedSegmentIds = [];
      uiState.activeConstraintTool = null;
      document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
      showToast(`Наложено ограничение: ${RestrictionMeta[type].name}`);
      renderAll();
      return true;
    }
  }

  if (type === "VERTICAL" || type === "HORIZONTAL") {
    let pIds = null, sIds = [];
    if (segs.length >= 1) {
      const s = api.findSegment(segs[0]);
      if (s) { pIds = [s.p1_id, s.p2_id]; sIds = [s.id]; }
    } else if (pts.length >= 2) {
      pIds = [pts[0], pts[1]];
    }
    if (pIds) {
      api.applyRestriction(type, pIds, sIds);
      uiState.selectedPointIds = [];
      uiState.selectedSegmentIds = [];
      uiState.activeConstraintTool = null;
      document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
      showToast(`Наложено ограничение: ${RestrictionMeta[type].name}`);
      renderAll();
      return true;
    }
  }

  if (type === "ANGLE" && segs.length >= 2) {
    const s1 = api.findSegment(segs[0]);
    const s2 = api.findSegment(segs[1]);
    if (s1 && s2 && s1.p1_ref && s1.p2_ref && s2.p1_ref && s2.p2_ref) {
      const ux = s1.p2_ref.x - s1.p1_ref.x, uy = s1.p2_ref.y - s1.p1_ref.y;
      const vx = s2.p2_ref.x - s2.p1_ref.x, vy = s2.p2_ref.y - s2.p1_ref.y;
      const dot = ux * vx + uy * vy;
      const l1 = Math.max(1e-9, Math.hypot(ux, uy)), l2 = Math.max(1e-9, Math.hypot(vx, vy));
      const curAngle = Math.acos(Math.max(-1, Math.min(1, dot / (l1 * l2)))) * 180 / Math.PI;
      openParamModal({
        title: "Ограничение: Угол между отрезками (α°)",
        desc: `Задайте угол между отрезками ${s1.label} и ${s2.label}:`,
        label1: "Угол α (в градусах):",
        default1: curAngle || 60
      }, (val) => {
        api.applyRestriction("ANGLE", [s1.p1_id, s1.p2_id, s2.p1_id, s2.p2_id], [s1.id, s2.id], val);
        uiState.selectedSegmentIds = [];
        uiState.activeConstraintTool = null;
        document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
        showToast(`Наложен угол α = ${val.toFixed(1)}°`);
        renderAll();
      });
      return true;
    }
  }

  if (type === "POINT_ON_LINE" && pts.length >= 1 && segs.length >= 1) {
    const pt = api.findPoint(pts[0]);
    const seg = api.findSegment(segs[0]);
    if (pt && seg) {
      api.applyRestriction("POINT_ON_LINE", [pt.id, seg.p1_id, seg.p2_id], [seg.id]);
      uiState.selectedPointIds = [];
      uiState.selectedSegmentIds = [];
      uiState.activeConstraintTool = null;
      document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
      showToast(`Наложено ограничение: ${pt.label} принадлежит прямой ${seg.label}`);
      renderAll();
      return true;
    }
  }

  if (type === "FIX_POINT" && pts.length >= 1) {
    const pt = api.findPoint(pts[0]);
    if (pt) {
      openParamModal({
        title: `Фиксация точки ${pt.label}`,
        desc: "Задайте координаты фиксации (x_c, y_c):",
        label1: "Координата X (x_c, мм):",
        default1: pt.x,
        label2: "Координата Y (y_c, мм):",
        default2: pt.y
      }, (xc, yc) => {
        api.applyRestriction("FIX_POINT", [pt.id], [], xc, yc);
        uiState.selectedPointIds = [];
        uiState.activeConstraintTool = null;
        document.querySelectorAll(".constraint-btn").forEach(b => b.classList.remove("active"));
        showToast(`Точка ${pt.label} зафиксирована в (${xc.toFixed(1)}, ${yc.toFixed(1)})`);
        renderAll();
      });
      return true;
    }
  }

  return false;
}

export function activateConstraintButton(type) {
  // Если нужные объекты уже выделены — сразу применяем ограничение
  if (tryApplyConstraintFromSelection(type)) return;

  // Иначе включаем интерактивный режим пошагового выбора примитивов на холсте
  uiState.activeConstraintTool = type;
  uiState.selectedPointIds = [];
  uiState.selectedSegmentIds = [];

  document.querySelectorAll(".constraint-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.constraint === type);
  });

  const meta = RestrictionMeta[type];
  document.getElementById("modePromptBadge").textContent = `Ограничение: ${meta.name}`;
  const hints = {
    COINCIDENT: "Кликните последовательно по 2 точкам на рабочем поле для их совмещения.",
    DISTANCE: "Кликните по 1 отрезку или по 2 точкам для задания расстояния d.",
    PARALLEL: "Кликните последовательно по 2 отрезкам для наложения параллельности.",
    PERPENDICULAR: "Кликните последовательно по 2 отрезкам для наложения перпендикулярности.",
    VERTICAL: "Кликните по 1 отрезку (или 2 точкам), чтобы сделать его вертикальным.",
    HORIZONTAL: "Кликните по 1 отрезку (или 2 точкам), чтобы сделать его горизонтальным.",
    ANGLE: "Кликните последовательно по 2 отрезкам для задания угла α° между ними.",
    POINT_ON_LINE: "Кликните по 1 точке и по 1 отрезку (в любом порядке).",
    FIX_POINT: "Кликните по точке, которую необходимо зафиксировать в пространстве."
  };
  document.getElementById("modeHintText").textContent = hints[type] || "";
  renderAll();
}
