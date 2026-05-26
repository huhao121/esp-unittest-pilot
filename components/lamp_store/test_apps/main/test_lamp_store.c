#include <stdlib.h>
#include <stdbool.h>
#include "unity.h"
#include "lamp_store.h"

#if !CONFIG_IDF_TARGET_LINUX
#include "esp_system.h"     // esp_restart
#include "nvs_flash.h"
#include "nvs.h"
#endif

void app_main(void)
{
#if CONFIG_IDF_TARGET_LINUX
    // host(linux)：非交互跑完所有用例（真重启用例只在板子/QEMU 编，这里没有）。
    // 套 UNITY_BEGIN/END 打汇总行；跑完按失败数给退出码，host CI 直接看 elf 返回值。
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
    exit(Unity.TestFailures == 0 ? 0 : 1);
#else
    // 板子 / QEMU：先把 NVS 初始化好。重启后 app_main 会再跑一遍，所以两阶段都覆盖。
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }
    // 起 unity 菜单，交给 pytest 的 run_all_single_board_cases() 驱动；
    // 多阶段（真重启）用例靠菜单在两阶段之间续跑。
    unity_run_menu();
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

#if !CONFIG_IDF_TARGET_LINUX
// ---- 真 NVS 后端：掉电不丢，给「真重启」测试用 ----
// 这是碰硬件的薄胶水（按工程约定本该放 demo/）。放在测试工程里，是因为只有它
// 需要在 QEMU/板子上验证“重启后还在”；以后接真板子可照搬到 demo/。
#define LS_NVS_NAMESPACE "lampstore"
#define LS_NVS_KEY       "state"

static bool nvs_be_load(void *ctx, uint8_t *out)
{
    (void)ctx;
    nvs_handle_t h;
    if (nvs_open(LS_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        return false;        // namespace 还不存在 = 从没存过
    }
    esp_err_t err = nvs_get_u8(h, LS_NVS_KEY, out);
    nvs_close(h);
    return err == ESP_OK;
}

static bool nvs_be_save(void *ctx, uint8_t value)
{
    (void)ctx;
    nvs_handle_t h;
    if (nvs_open(LS_NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        return false;
    }
    esp_err_t err = nvs_set_u8(h, LS_NVS_KEY, value);
    if (err == ESP_OK) {
        err = nvs_commit(h);   // commit 才真正落到 flash
    }
    nvs_close(h);
    return err == ESP_OK;
}

static lamp_store_backend_t nvs_backend(void)
{
    return (lamp_store_backend_t){ .load = nvs_be_load, .save = nvs_be_save, .ctx = NULL };
}

// 阶段一（重启前）：存蓝，然后真的重启芯片。
static void stage_save_blue_then_reboot(void)
{
    lamp_store_backend_t b = nvs_backend();
    TEST_ASSERT_TRUE(lamp_store_save(&b, LAMP_BLUE));
    esp_restart();   // 真重启；重启后由 pytest 续跑阶段二（这行之后不再返回）
}

// 阶段二（重启后）：读回来还得是蓝 —— 证明值扛过了一次真重启。
static void stage_after_reboot_still_blue(void)
{
    lamp_store_backend_t b = nvs_backend();
    TEST_ASSERT_EQUAL(LAMP_BLUE, lamp_store_load(&b));
}

// 官方多阶段写法：第一段跑完 esp_restart()，第二段在重启后跑。
TEST_CASE_MULTIPLE_STAGES("lamp_store: BLUE survives a real reboot", "[lamp_store]",
                          stage_save_blue_then_reboot, stage_after_reboot_still_blue);
#endif // !CONFIG_IDF_TARGET_LINUX
