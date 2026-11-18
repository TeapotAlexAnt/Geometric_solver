import "./styles.css";
import "@fontsource/inter/latin-400.css";
import "@fontsource/inter/latin-500.css";
import "@fontsource/inter/latin-600.css";
import "@fontsource/inter/latin-700.css";
import "@fontsource/inter/cyrillic-400.css";
import "@fontsource/inter/cyrillic-500.css";
import "@fontsource/inter/cyrillic-600.css";
import "@fontsource/inter/cyrillic-700.css";
import "@fontsource/jetbrains-mono/latin-400.css";
import "@fontsource/jetbrains-mono/latin-500.css";
import "@fontsource/jetbrains-mono/latin-600.css";
import "@fontsource/jetbrains-mono/cyrillic-400.css";
import "@fontsource/jetbrains-mono/cyrillic-500.css";
import "@fontsource/jetbrains-mono/cyrillic-600.css";

import { observeSketchSize } from "./sketch-view.js";
import { bindControls } from "./controls.js";
import { initEditorApi } from "./editor-api.js";
import { bindPointerEvents } from "./pointer.js";
import { loadPresetAll9Constraints } from "./presets.js";
import { bindSourceViewer } from "./source-viewer.js";

async function start() {
  const badge = document.getElementById("wasmStatusBadge");
  try {
    await initEditorApi();
    observeSketchSize();
    loadPresetAll9Constraints();
    bindControls();
    bindPointerEvents();
    bindSourceViewer();
    badge.textContent = "WASM активен";
    badge.classList.add("bg-emerald-500/15", "text-emerald-300", "border-emerald-500/30");
  } catch (error) {
    badge.textContent = "Ошибка запуска WASM";
    badge.classList.add("text-rose-300", "border-rose-500/50");
    console.error(error);
  }
}

start();
