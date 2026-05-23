#!/usr/bin/env bash
# host 单测一键脚本：本地和 CI 跑的是同一个文件，避免“在我机器上是好的”。
#
# 用法（本地）：先 source ~/esp/esp-idf/export.sh，再在仓库任意目录跑
#     ./ci/run_host_tests.sh
# CI 会跑同一个脚本。

# 出错就立刻停，并把非 0 退出码往外传 —— 这样测试一挂，CI 就变红。
set -euo pipefail

# 不管你在哪个目录调用，都先切回仓库根目录（脚本在 ci/ 下，所以往上一层）。
cd "$(dirname "$0")/.."

# 如果当前还没有 idf.py（说明环境没 source 过），就 source 一下 ESP-IDF。
#   本地：你已经 source 过 export.sh，这步会自动跳过。
#   CI ：官方镜像里 IDF_PATH=/opt/esp/idf，这里帮你 source。
if ! command -v idf.py >/dev/null 2>&1; then
    . "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh"
fi

# 进到 lamp 组件的测试工程目录。
cd components/lamp/test_apps

# 设成 linux target（在电脑上跑单测，必须带 --preview）。
idf.py --preview set-target linux

# 编译。
idf.py build

# 直接跑生成的 elf。测试失败时它返回非 0，配合上面的 set -e，脚本会一起失败。
./build/lamp_test.elf
