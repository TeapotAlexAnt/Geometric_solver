import { showToast } from "./editor-state.js";

const files = import.meta.glob([
  "../../include/**/*.hpp",
  "../../src/**/*.cpp",
  "../../src/**/*.hpp",
  "../../tests/*.cpp",
  "../../CMakeLists.txt",
  "../../build_wasm.sh"
], { query: "?raw", import: "default", eager: true });
const sources = Object.fromEntries(Object.entries(files).map(([path, code]) => [path.slice(6), code]));

export function bindSourceViewer() {
  const cppSelect = document.getElementById("cppFileSelect");
  const cppViewer = document.getElementById("cppCodeViewer");
  Object.keys(sources).forEach(fname => {
    const opt = document.createElement("option");
    opt.value = fname;
    opt.textContent = fname;
    cppSelect.appendChild(opt);
  });
  cppSelect.addEventListener("change", () => {
    cppViewer.textContent = sources[cppSelect.value] || "";
  });
  cppViewer.textContent = sources[cppSelect.value] || "";

  document.getElementById("btnCopyCppCode").addEventListener("click", () => {
    const tmp = document.createElement("textarea");
    tmp.value = cppViewer.textContent;
    document.body.appendChild(tmp);
    tmp.select();
    document.execCommand("copy");
    document.body.removeChild(tmp);
    showToast(`Файл ${cppSelect.value} скопирован в буфер обмена`);
  });
}
