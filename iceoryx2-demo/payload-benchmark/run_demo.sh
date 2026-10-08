#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ICEORYX2_DIR="${ICEORYX2_DIR:-$ROOT/../iceoryx2}"
ICEORYX2_BUILD_DIR="${ICEORYX2_BUILD_DIR:-$ICEORYX2_DIR/target/ff/cc/build}"
ICEORYX2_INSTALL_DIR="${ICEORYX2_INSTALL_DIR:-$ICEORYX2_DIR/target/ff/cc/install}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"

if [[ ! -f "$ICEORYX2_DIR/CMakeLists.txt" ]]; then
  echo "[setup] 下载 iceoryx2 源码到 $ICEORYX2_DIR"
  git clone --depth 1 https://github.com/eclipse-iceoryx/iceoryx2.git "$ICEORYX2_DIR"
fi
if [[ ! -f "$ICEORYX2_INSTALL_DIR/lib/cmake/iceoryx2-cxx/iceoryx2-cxxConfig.cmake" ]]; then
  echo "[setup] 首次构建并安装 iceoryx2 C/C++ bindings"
  cmake -S "$ICEORYX2_DIR" -B "$ICEORYX2_BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
  cmake --build "$ICEORYX2_BUILD_DIR" -j"$(nproc)"
  cmake --install "$ICEORYX2_BUILD_DIR" --prefix "$ICEORYX2_INSTALL_DIR"
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$ICEORYX2_INSTALL_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

run_case() {
  local size="$1" count="$2" id="size-$1-$$"
  echo
  echo "========== payload ${size}B =========="
  "$BUILD_DIR/subscriber" "$size" "$count" "$id" &
  local subscriber_pid=$!
  sleep 1
  "$BUILD_DIR/publisher" "$size" "$count" "$id"
  wait "$subscriber_pid"
}

run_case 64 "${SMALL_COUNT:-200}"
run_case 1048576 "${LARGE_COUNT:-200}"
echo "[done] 对比两轮的平均 send 时间；填充 1MiB 本身仍需写内存，零拷贝指收发阶段不再搬运 payload。"
