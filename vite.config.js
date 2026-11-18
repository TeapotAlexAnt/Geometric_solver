import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { resolve, sep } from "node:path";
import tailwindcss from "tailwindcss";
import { defineConfig } from "vite";

const projectRoot = fileURLToPath(new URL(".", import.meta.url));
const webRoot = resolve(projectRoot, "web");
const buildScript = resolve(projectRoot, "build_wasm.sh");
const cppDirs = ["src", "include"].map(dir => resolve(projectRoot, dir));
const buildFiles = [buildScript, resolve(projectRoot, "CMakeLists.txt")];

function compileWasm(mode) {
  execFileSync("bash", [buildScript, mode], { cwd: projectRoot, stdio: "inherit" });
}

function watchWasm() {
  let rebuildTimer;

  return {
    name: "geometric-solver-wasm",
    apply: "serve",
    configureServer(server) {
      server.watcher.add([...cppDirs, ...buildFiles]);
      server.watcher.once("close", () => clearTimeout(rebuildTimer));
    },
    hotUpdate({ file, server }) {
      const isCpp = cppDirs.some(dir => file.startsWith(dir + sep));
      if (this.environment.name !== "client" || (!isCpp && !buildFiles.includes(file))) {
        return;
      }

      clearTimeout(rebuildTimer);
      rebuildTimer = setTimeout(() => {
        try {
          compileWasm("debug");
          server.moduleGraph.invalidateAll();
          server.ws.send({ type: "full-reload" });
        } catch (error) {
          server.config.logger.error(error.message);
          if (server.ws.clients.size) {
            server.ws.send({
              type: "error",
              err: { message: "Сборка WASM завершилась ошибкой. Подробности в терминале.", stack: "" }
            });
          }
        }
      }, 100);
      return [];
    }
  };
}

export default defineConfig(({ command, isPreview }) => {
  const wasmMode = command === "build" ? "release" : "debug";
  if (!isPreview) compileWasm(wasmMode);

  return {
    root: webRoot,
    base: "./",
    resolve: {
      alias: {
        "@geom-solver": resolve(projectRoot, ".build/wasm", wasmMode, "geom_solver.mjs")
      }
    },
    css: {
      postcss: {
        plugins: [
          tailwindcss({
            content: [resolve(webRoot, "index.html"), resolve(webRoot, "src/**/*.js")]
          })
        ]
      }
    },
    plugins: [watchWasm()],
    server: {
      host: "127.0.0.1",
      fs: { allow: [projectRoot] },
      watch: { ignored: ["**/.build/**"] }
    }
  };
});
