# Вариационный геометрический решатель (Кафедра РК6, ЛР №1)

## Архитектура (по схеме бригады)
- **Backend (C++17 / WebAssembly)**:
  - `include/Figure.hpp`: Базовый полиморфный класс `Figure` (`params()`) и примитивы `PointFigure`, `SegmentFigure`.
  - `include/Restrictions.hpp`: 9 геометрических ограничений с аналитическими выражениями невязок $f_j(p+\Delta p)$, первых производных $\frac{\partial f_j}{\partial \Delta p_i}$ и вторых производных $\frac{\partial^2 f_j}{\partial \Delta p_i \partial \Delta p_k}$, зашитыми в код.
  - `include/CoreAlgo.hpp`:
    - `CoreAlgo::solve(A, b)` — собственная реализация решения СЛАУ методом Гаусса с выбором главного элемента без сторонних библиотек.
    - `CoreAlgo::optimize(f, df, params)` — решение оптимизационной задачи Лагранжа на основе принципа наименьших изменений методом Ньютона–Рафсона.
  - `include/GeomRepository.hpp`: Хранилище примитивов `Geom` и ограничений `Restrictions`, а также `JSON Serializer`.
  - `include/HelperFunctions.hpp`: Модуль `query` (поиск объектов по координатам, вычисление степеней свободы `DOF`).
  - `include/GeomSolver.hpp`: Модуль `solve(geom, restrictions, new_restrictions) -> new_geom`.
  - `include/UseCases.hpp` и `src/EmscriptenBindings.cpp`: `Render Usecase`, `Tool_1 Usecase`, `Pointer Usecase` и `API Handler` (`getAllPrimitives`, экспорт в WASM).
- **Visualization (JS / HTML5 Canvas)**:
  - Рабочее поле (CAD Canvas с сеткой, привязкой, масштабированием, размерными выносками и иконками ограничений).
  - Панель инструментов (создание/удаление точек и отрезков, наложение 9 типов ограничений, готовые демо-эскизы).
  - Браузер ограничений и инспектор матрицы Якоби / СЛАУ.

## Сборка и запуск тестов
1. Проверка математического ядра на C++ (g++):
   ```bash
   g++ -std=c++17 -O2 tests/test_solver.cpp -o test_solver && ./test_solver
   ```
2. Компиляция в WebAssembly (Emscripten):
   ```bash
   ./build_wasm.sh
   ```
