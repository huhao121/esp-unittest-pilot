#!/usr/bin/env bash
# QEMU 单测一键脚本：本地和 CI 跑的是同一个文件（和 run_host_tests.sh 一个套路）。
#
# 跑的是 lamp_store 那套 unity 单测，但不插板子 —— 用 QEMU 模拟 esp32s3。
#
# 用法（本地）：先 source ~/esp/esp-idf/export.sh，并确保已装好
#   qemu-xtensa 和 pytest-embedded[idf,qemu]（CI 里这步由 workflow 负责），
#   然后在仓库任意目录跑：
#     ./ci/run_qemu_tests.sh
# CI 会跑同一个脚本。

# 出错就立刻停，并把非 0 退出码往外传 —— 测试一挂，CI 就变红。
set -euo pipefail

# 不管在哪个目录调用，都先切回仓库根目录（脚本在 ci/ 下，往上一层）。
cd "$(dirname "$0")/.."

# 没有 idf.py（环境没 source 过）就 source 一下 ESP-IDF。
#   本地：你已 source 过 export.sh，这步自动跳过。
#   CI ：官方镜像里 IDF_PATH=/opt/esp/idf，这里帮你 source（也会把 qemu 带进 PATH）。
if ! command -v idf.py >/dev/null 2>&1; then
    . "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh"
fi

# 进到 lamp_store 组件的测试工程目录。
cd components/lamp_store/test_apps

# QEMU 跑的是真 Xtensa 固件，得编 esp32s3（不是 host 的 linux target）。
idf.py set-target esp32s3
idf.py build

# 用乐鑫官方 pytest-embedded 驱动 QEMU 跑那条 pytest。
# 任一 unity 用例失败 -> pytest 返回非 0 -> 配合上面的 set -e，脚本一起失败。
pytest pytest_lamp_store.py --embedded-services idf,qemu --target esp32s3 -v
