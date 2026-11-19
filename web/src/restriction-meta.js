export const RestrictionMeta = {
  COINCIDENT: {
    code: 1, name: "Совпадение точек", equations: []
  },
  DISTANCE: {
    code: 2, name: "Расстояние между точками",
    equations: ["(X_2-X_1)^2 + (Y_2-Y_1)^2 - d^2 = 0"]
  },
  PARALLEL: {
    code: 3, name: "Параллельность отрезков",
    equations: ["(X_2-X_1)(Y_4-Y_3) - (X_4-X_3)(Y_2-Y_1) = 0"]
  },
  PERPENDICULAR: {
    code: 4, name: "Перпендикулярность отрезков",
    equations: ["(X_2-X_1)(X_4-X_3) + (Y_2-Y_1)(Y_4-Y_3) = 0"]
  },
  VERTICAL: {
    code: 5, name: "Вертикальность отрезка",
    equations: ["X_2-X_1=0"]
  },
  HORIZONTAL: {
    code: 6, name: "Горизонтальность отрезка",
    equations: ["Y_2-Y_1=0"]
  },
  ANGLE: {
    code: 7, name: "Угол между отрезками",
    equations: [String.raw`\mathbf{v}_{12}\cdot\mathbf{v}_{34}
      - \lVert\mathbf{v}_{12}\rVert\,\lVert\mathbf{v}_{34}\rVert\cos\alpha=0`]
  },
  POINT_ON_LINE: {
    code: 8, name: "Принадлежность точки прямой",
    equations: ["(X_P-X_1)(Y_2-Y_P) - (X_2-X_P)(Y_P-Y_1) = 0"]
  },
  FIX_POINT: {
    code: 9, name: "Фиксация точки",
    equations: ["X-x_c=0", "Y-y_c=0"]
  }
};
