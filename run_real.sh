#!/usr/bin/env bash
# ============================================================
# RP-26Rune 真实硬件集合启动程序
# 仿照 sp_vision_25/autostart.sh + watchdog.sh：
#   1. cmake 编译
#   2. 加载海康/迈德威视相机 SDK 动态库路径
#   3. screen 后台运行 + 看门狗崩溃自动重启
#
# 用法:
#   ./run_real.sh                    # 默认海康相机(runtime_config_real.json)
#   ./run_real.sh --mindvision       # 改用迈德威视相机(runtime_config_mv.json)
#   ./run_real.sh --build-only
#   ./run_real.sh --screen           # 放入 screen 后台运行(仿 autostart)
# ============================================================
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "${project_root}"

BUILD_JOBS="${BUILD_JOBS:-4}"
CONFIG_NAME="${CONFIG_NAME:-runtime_config_real.json}"

# 海康 + 迈德威视相机 SDK 运行时库
export LD_LIBRARY_PATH=/opt/MVS/lib/64:"${project_root}/third_party/hikrobot/lib/amd64:${project_root}/third_party/mindvision/lib/amd64:${project_root}/output:${LD_LIBRARY_PATH:-}"

mode="run"
for arg in "$@"; do
    case "$arg" in
        --build-only) mode="build-only" ;;
        --screen)     mode="screen" ;;
        --mindvision) CONFIG_NAME="runtime_config_mv.json" ;;
        --headless)   export QT_QPA_PLATFORM=offscreen ;;
        *) echo "unknown arg: $arg" >&2; exit 2 ;;
    esac
done

build() {
    echo "[run_real] cmake configure..."
    cmake -S "${project_root}" -B "${project_root}/build" -DCMAKE_BUILD_TYPE=Release
    echo "[run_real] building with ${BUILD_JOBS} jobs..."
    cmake --build "${project_root}/build" --parallel "${BUILD_JOBS}"
}

watchdog() {
    mkdir -p "${project_root}/logs"
    local logfile="${project_root}/logs/$(date '+%Y-%m-%d_%H-%M-%S').log"
    echo "[run_real] log -> ${logfile}"
    while true; do
        echo "[run_real] starting app (config=${CONFIG_NAME}) at $(date)"
        "${project_root}/output/app" "${CONFIG_NAME}" 2>&1 | tee -a "${logfile}"
        echo "[run_real] app exited with ${PIPESTATUS[0]}, restart in 1s..."
        sleep 1
    done
}

case "${mode}" in
    build-only)
        build
        echo "[run_real] build-only done."
        ;;
    screen)
        build
        exec screen -L -Logfile "${project_root}/logs/screen_$(date '+%Y-%m-%d_%H-%M-%S').log" \
            -d -m bash -c "cd ${project_root}; exec bash ${BASH_SOURCE[0]} --headless"
        ;;
    run)
        build
        watchdog
        ;;
esac
