#!/usr/bin/env bash
set -euo pipefail
idf_commit=fcae32885b0296b32044cb99ecbdc50d98dddb83
idf_dir="${TDONGLE_IDF_DIR:-$HOME/.cache/tdongle/esp-idf-v5.5.1}"
if [[ ! -d "$idf_dir/.git" ]]; then
  mkdir -p "$(dirname "$idf_dir")"
  git clone --branch v5.5.1 --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git "$idf_dir"
fi
[[ "$(git -C "$idf_dir" rev-parse HEAD)" == "$idf_commit" ]] || { echo 'Wrong IDF commit' >&2; exit 1; }
"$idf_dir/install.sh" esp32s3
printf 'Next: source %q/export.sh\n' "$idf_dir"
