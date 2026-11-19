import katex from "katex";

const cache = new Map();

export function mathMarkup(tex, displayMode = false) {
  const key = (displayMode ? "display:" : "inline:") + tex;
  if (cache.has(key)) return cache.get(key);

  const markup = katex.renderToString(tex, { displayMode, throwOnError: false });
  // Размеры и невязки меняются во время перетаскивания, поэтому кэш ограничен.
  if (cache.size === 512) cache.delete(cache.keys().next().value);
  cache.set(key, markup);
  return markup;
}

export function renderStaticMath() {
  for (const element of document.querySelectorAll("[data-tex]")) {
    element.innerHTML = mathMarkup(element.dataset.tex, element.classList.contains("math-display"));
  }
}

export function numberTex(value, digits) {
  const magnitude = Math.abs(value);
  if (value === 0 || (magnitude >= 1e-3 && magnitude < 1e4)) {
    return String(Number(value.toPrecision(digits + 1)));
  }
  const [mantissa, exponent] = value.toExponential(digits).split("e");
  return mantissa + "\\times10^{" + Number(exponent) + "}";
}
