#!/usr/bin/env bash
# 在电脑上跑 lamp 单测并出覆盖率报告（linux host + gcov/gcovr）。
# 用法：在 components/lamp/test_apps/ 下执行  ./coverage.sh
set -euo pipefail

# 需要先 source ~/esp/esp-idf/export.sh
command -v idf.py  >/dev/null || { echo "先 source ~/esp/esp-idf/export.sh"; exit 1; }
command -v gcovr   >/dev/null || { echo "需要 gcovr： pip install gcovr"; exit 1; }

# linux 是 preview target；幂等
idf.py --preview set-target linux >/dev/null

# 带 --coverage 开关全新构建，再跑一遍生成 .gcda
idf.py -DLAMP_COVERAGE=ON build
./build/lamp_test.elf < /dev/null

# 只看 lamp.c 的行 + 分支覆盖
echo "=== Coverage (lamp.c) ==="
gcovr --root ../.. --filter '.*/lamp\.c$' --txt --branches build

# 顺手出一份 HTML
mkdir -p build/coverage
gcovr --root ../.. --filter '.*/lamp\.c$' --html-details build/coverage/index.html
echo "HTML: $(pwd)/build/coverage/index.html"
