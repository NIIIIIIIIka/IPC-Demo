#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AERON_DIR="${AERON_DIR:-$ROOT/../aeron}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"

if [[ ! -f "$AERON_DIR/CMakeLists.txt" ]]; then
  echo "[setup] 下载 Aeron 源码到 $AERON_DIR"
  git clone --depth 1 https://github.com/aeron-io/aeron.git "$AERON_DIR"
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DAERON_DIR="$AERON_DIR"
cmake --build "$BUILD_DIR" --target publisher subscriber aeronmd -j"$(nproc)"

LOG_DIR="$BUILD_DIR/logs"
mkdir -p "$LOG_DIR"
driver="$BUILD_DIR/aeron/binaries/aeronmd"
[[ -x "$driver" ]] || driver="$(find "$BUILD_DIR" -type f -name aeronmd -perm -111 | head -n 1)"
[[ -n "$driver" ]] || { echo "找不到 aeronmd" >&2; exit 1; }

pids=()
cleanup() {
  for pid in "${pids[@]:-}"; do kill "$pid" 2>/dev/null || true; done
  wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "[run] 启动 Media Driver"
"$driver" >"$LOG_DIR/media-driver.log" 2>&1 & pids+=("$!")
sleep 1
echo "[run] 启动慢订阅者"
"$BUILD_DIR/subscriber" >"$LOG_DIR/subscriber.log" 2>&1 & pids+=("$!")
sleep 1
echo "[run] 启动发布者；屏幕上将出现 BACK_PRESSURED"
"$BUILD_DIR/publisher" "${MESSAGE_COUNT:-4000}" "${PAYLOAD_SIZE:-65536}" | tee "$LOG_DIR/publisher.log"
echo "[done] 完整日志：$LOG_DIR"
