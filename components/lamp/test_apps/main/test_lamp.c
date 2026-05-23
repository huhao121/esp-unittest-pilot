#include <stdlib.h>
#include "unity.h"
#include "lamp.h"

void app_main(void)
{
    // 非交互地跑完所有 TEST_CASE。同一份测试在 host(linux) 和板子(esp32s3) 上都能跑。
    unity_run_all_tests();
#if CONFIG_IDF_TARGET_LINUX
    // FreeRTOS-on-linux 在 app_main 返回后不会退出进程，
    // 按失败数主动退出，给自动化 / CI 一个明确的退出码。
    // 在真板子上没有“进程”可退，跑完让它 idle 即可，所以这段只在 host 编。
    exit(Unity.TestFailures == 0 ? 0 : 1);
#endif
}

// ---- lamp_next: 每个状态按一下变成谁 ----

TEST_CASE("lamp_next: OFF advances to RED", "[lamp]")
{
    TEST_ASSERT_EQUAL(LAMP_RED, lamp_next(LAMP_OFF));
}

TEST_CASE("lamp_next: RED advances to GREEN", "[lamp]")
{
    TEST_ASSERT_EQUAL(LAMP_GREEN, lamp_next(LAMP_RED));
}

TEST_CASE("lamp_next: GREEN advances to BLUE", "[lamp]")
{
    TEST_ASSERT_EQUAL(LAMP_BLUE, lamp_next(LAMP_GREEN));
}

// 最重要的一条：到蓝按一下要绕回到关
TEST_CASE("lamp_next: BLUE wraps back to OFF", "[lamp]")
{
    TEST_ASSERT_EQUAL(LAMP_OFF, lamp_next(LAMP_BLUE));
}

// 连按四下回到关
TEST_CASE("lamp_next: four presses return to OFF", "[lamp]")
{
    lamp_state_t s = LAMP_OFF;
    for (int i = 0; i < 4; i++) {
        s = lamp_next(s);
    }
    TEST_ASSERT_EQUAL(LAMP_OFF, s);
}

// 传一个非法状态要安全返回关
TEST_CASE("lamp_next: invalid state returns OFF safely", "[lamp]")
{
    TEST_ASSERT_EQUAL(LAMP_OFF, lamp_next(LAMP_STATE_COUNT));
    TEST_ASSERT_EQUAL(LAMP_OFF, lamp_next((lamp_state_t)42));
    TEST_ASSERT_EQUAL(LAMP_OFF, lamp_next((lamp_state_t)-1));
}

// ---- lamp_color: 颜色映射每个都测 ----

static void assert_rgb(lamp_rgb_t c, uint8_t r, uint8_t g, uint8_t b)
{
    TEST_ASSERT_EQUAL_UINT8(r, c.r);
    TEST_ASSERT_EQUAL_UINT8(g, c.g);
    TEST_ASSERT_EQUAL_UINT8(b, c.b);
}

TEST_CASE("lamp_color: OFF is black", "[lamp]")
{
    assert_rgb(lamp_color(LAMP_OFF), 0, 0, 0);
}

TEST_CASE("lamp_color: RED is full red", "[lamp]")
{
    assert_rgb(lamp_color(LAMP_RED), 254, 0, 0);  // 故意改错：期望值应是 255，用来看 CI 变红
}

TEST_CASE("lamp_color: GREEN is full green", "[lamp]")
{
    assert_rgb(lamp_color(LAMP_GREEN), 0, 255, 0);
}

TEST_CASE("lamp_color: BLUE is full blue", "[lamp]")
{
    assert_rgb(lamp_color(LAMP_BLUE), 0, 0, 255);
}

// 非法状态的颜色也要安全返回黑（关）
TEST_CASE("lamp_color: invalid state is black", "[lamp]")
{
    assert_rgb(lamp_color((lamp_state_t)99), 0, 0, 0);
}
