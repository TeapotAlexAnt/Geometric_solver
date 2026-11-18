#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

if [[ ! -x node_modules/.bin/vite || package-lock.json -nt node_modules/.package-lock.json ]]; then
  npm ci
fi

exec npm run dev -- "$@"
