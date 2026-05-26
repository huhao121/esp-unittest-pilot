# 乐鑫官方 pytest-embedded 驱动：把 lamp_store 的 unity 单测放进 QEMU 自动跑一遍。
#
# 为什么能在 QEMU 上跑：这些用例是纯逻辑（test_lamp_store.c 里塞的是内存假后端，
# 不碰真 NVS / 外设），所以在 QEMU 里跑等价于在真 esp32 上跑，只是不用插板子。
#
# 怎么判定通过：app_main 调的是 unity_run_all_tests()，会自动跑完并打印
#   "N Tests M Failures K Ignored" 汇总。下面的 expect_unity_test_output()
#   解析这段输出、把每个 TEST_CASE 记成一条结果；只要有用例失败，这条 pytest 就红。
#
# 跑法（先 source ~/esp/esp-idf/export.sh）：
#   pip install pytest-embedded pytest-embedded-idf pytest-embedded-qemu
#   idf.py set-target esp32          # QEMU 最成熟的 target；纯逻辑测试与具体芯片无关
#   idf.py build
#   pytest --embedded-services idf,qemu --target esp32
#
# 想换成板子的 esp32s3：把下面的 @pytest.mark.esp32 改成 @pytest.mark.esp32s3，
# 再用 `idf.py set-target esp32s3 && idf.py build`、`pytest ... --target esp32s3`。

import pytest


@pytest.mark.esp32s3  # QEMU 模拟的芯片，对齐板子；纯逻辑用例换 esp32 也一样过
@pytest.mark.qemu     # 走 QEMU，不需要真硬件
def test_lamp_store_on_qemu(dut):
    # 解析 unity 汇总输出；任一 TEST_CASE 失败这里就会让测试失败。
    dut.expect_unity_test_output(timeout=120)
