# 乐鑫官方 pytest-embedded 驱动：把 lamp_store 的 unity 单测放进 QEMU 自动跑一遍。
#
# app_main 在板子/QEMU 上起的是 unity 菜单（unity_run_menu）。这里用
# run_all_single_board_cases() 把菜单里所有用例都跑掉，它同时认两类用例：
#   - 普通用例（save/load 那两条纯逻辑，用内存假后端）；
#   - 多阶段用例（"BLUE survives a real reboot"：阶段一存蓝并 esp_restart()，
#     阶段二重启后读回）—— 中间那次真重启由它自动检测、续跑阶段二。
#
# 跑法（先 source ~/esp/esp-idf/export.sh，并装好 qemu-xtensa 和
# pytest-embedded[idf,qemu]）：
#   idf.py set-target esp32s3 && idf.py build
#   pytest pytest_lamp_store.py --embedded-services idf,qemu --target esp32s3

import pytest


@pytest.mark.esp32s3  # QEMU 模拟的芯片，对齐板子
@pytest.mark.qemu     # 走 QEMU，不需要真硬件
def test_lamp_store_on_qemu(dut):
    # 跑菜单里全部用例（含多阶段真重启）；任一用例失败 -> 这条 pytest 就红。
    dut.run_all_single_board_cases()
