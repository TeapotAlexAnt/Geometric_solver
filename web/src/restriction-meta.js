export const RestrictionMeta = {
  COINCIDENT: {
    code: 1, name: "Совпадение точек", badge: "◎", eqs: 2,
    formula: "x₂+Δx₂−x₁−Δx₁ = 0; y₂+Δy₂−y₁−Δy₁ = 0"
  },
  DISTANCE: {
    code: 2, name: "Расстояние между точками", badge: "↔", eqs: 1,
    formula: "(X₂−X₁)² + (Y₂−Y₁)² − d² = 0"
  },
  PARALLEL: {
    code: 3, name: "Параллельность отрезков", badge: "∥", eqs: 1,
    formula: "(X₂−X₁)(Y₄−Y₃) − (X₄−X₃)(Y₂−Y₁) = 0"
  },
  PERPENDICULAR: {
    code: 4, name: "Перпендикулярность отрезков", badge: "⊥", eqs: 1,
    formula: "(X₂−X₁)(X₄−X₃) + (Y₂−Y₁)(Y₄−Y₃) = 0"
  },
  VERTICAL: {
    code: 5, name: "Вертикальность отрезка", badge: "V", eqs: 1,
    formula: "x₂ + Δx₂ − x₁ − Δx₁ = 0"
  },
  HORIZONTAL: {
    code: 6, name: "Горизонтальность отрезка", badge: "H", eqs: 1,
    formula: "y₂ + Δy₂ − y₁ − Δy₁ = 0"
  },
  ANGLE: {
    code: 7, name: "Угол между отрезками", badge: "∠", eqs: 1,
    formula: "(v₁₂ · v₃₄) − |v₁₂||v₃₄| cos(α) = 0"
  },
  POINT_ON_LINE: {
    code: 8, name: "Принадлежность точки прямой", badge: "∈", eqs: 1,
    formula: "(X_P−X₁)(Y₂−Y_P) − (X₂−X_P)(Y_P−Y₁) = 0"
  },
  FIX_POINT: {
    code: 9, name: "Фиксация точки", badge: "🔒", eqs: 2,
    formula: "x + Δx − x_c = 0; y + Δy − y_c = 0"
  }
};
