#include <stdlib.h>
#include <stdbool.h>
#include "unity.h"
#include "lamp_store.h"

void app_main(void)
{
    // 非交互地跑完所有 TEST_CASE。同一份测试在 host(linux)、板子(esp32s3)、QEMU 上都能跑。
    // 套 UNITY_BEGIN/END：unity_run_all_tests() 本身只逐条打 PASS/FAIL，不打总账；
    // UNITY_END() 才会打出 "N Tests M Failures K Ignored" 汇总行 ——
    // pytest-embedded 的 expect_unity_test_output() 靠这行判定整批是否通过。
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
#if CONFIG_IDF_TARGET_LINUX
    // FreeRTOS-on-linux 在 app_main 返回后不会退出进程，
    // 按失败数主动退出，给自动化 / CI 一个明确的退出码。
    // 在真板子 / QEMU 上没有“进程”可退，跑完让它 idle 即可，所以这段只在 host 编。
    exit(Unity.TestFailures == 0 ? 0 : 1);
#endif
}

// ---- 内存假后端：替掉电不丢的存储，host 上专测 lamp_store 的逻辑 ----
// 逻辑层不碰真 NVS，这里塞一个内存版后端，存取都在 RAM 里。
typedef struct {
    bool    has_value;   // 是否存过（没存过 = NVS 里没这个 key）
    uint8_t value;       // 存进去的字节
} fake_nvs_t;

static bool fake_load(void *ctx, uint8_t *out)
{
    fake_nvs_t *f = (fake_nvs_t *)ctx;
    if (!f->has_value) {
        return false;        // 从没存过
    }
    *out = f->value;
    return true;
}

static bool fake_save(void *ctx, uint8_t value)
{
    fake_nvs_t *f = (fake_nvs_t *)ctx;
    f->value = value;
    f->has_value = true;
    return true;
}

static lamp_store_backend_t make_backend(fake_nvs_t *f)
{
    return (lamp_store_backend_t){ .load = fake_load, .save = fake_save, .ctx = f };
}

// ---- 存一个蓝色再读回，要还是蓝 ----
TEST_CASE("lamp_store: save BLUE then load is still BLUE", "[lamp_store]")
{
    fake_nvs_t f = {0};
    lamp_store_backend_t b = make_backend(&f);

    TEST_ASSERT_TRUE(lamp_store_save(&b, LAMP_BLUE));
    TEST_ASSERT_EQUAL(LAMP_BLUE, lamp_store_load(&b));
}

// ---- 从没存过，读出来是关 ----
TEST_CASE("lamp_store: never stored reads as OFF", "[lamp_store]")
{
    fake_nvs_t f = {0};   // 没存过任何东西
    lamp_store_backend_t b = make_backend(&f);

    TEST_ASSERT_EQUAL(LAMP_OFF, lamp_store_load(&b));
}
