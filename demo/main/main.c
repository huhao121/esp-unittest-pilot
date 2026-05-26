// lamp demo：BOOT 键(GPIO0)切状态，WS2812(GPIO48)显示对应颜色，并把颜色记进 NVS。
// 开机先把上次的颜色读回来点亮；按键换色后存起来。掉电不丢。
// 纯逻辑全在 lamp / lamp_store 组件里，这里只是薄薄的硬件胶水，不进单测。
// WS2812 用 RMT 驱动 + vendor 进来的 led_strip_encoder，不依赖托管组件。
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "led_strip_encoder.h"
#include "lamp.h"
#include "lamp_store.h"

#define BUTTON_GPIO         GPIO_NUM_0    // 板载 BOOT 键，按下接地（低有效）
#define LED_GPIO            GPIO_NUM_48   // 板载 WS2812
#define RMT_RESOLUTION_HZ   10000000      // 10 MHz，每 tick 0.1us

// WS2812 满量程太刺眼，硬件层统一压暗（亮度调节本就该在这层做）。
#define BRIGHTNESS          16            // 0-255 的上限

static const char *TAG = "lamp_demo";

static rmt_channel_handle_t s_chan;
static rmt_encoder_handle_t s_encoder;
static lamp_state_t s_state = LAMP_OFF;

static inline uint8_t dim(uint8_t v)
{
    return (uint16_t)v * BRIGHTNESS / 255;
}

// ---- lamp_store 的真 NVS 后端：掉电不丢的颜色记忆 ----
// 逻辑（“存什么、读不到当灭的”）在 lamp_store 组件里；这里只管把一个字节
// 读写到 NVS，是碰硬件的薄胶水。和测试工程里那份后端一致。
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

static lamp_store_backend_t store_backend(void)
{
    return (lamp_store_backend_t){ .load = nvs_be_load, .save = nvs_be_save, .ctx = NULL };
}

static void led_init(void)
{
    rmt_tx_channel_config_t tx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_chan));

    led_strip_encoder_config_t enc_cfg = {.resolution = RMT_RESOLUTION_HZ};
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&enc_cfg, &s_encoder));

    ESP_ERROR_CHECK(rmt_enable(s_chan));
}

// 把 lamp 状态对应的颜色画到灯上（WS2812 字节序是 GRB）
static void render(lamp_state_t state)
{
    lamp_rgb_t c = lamp_color(state);
    uint8_t grb[3] = {dim(c.g), dim(c.r), dim(c.b)};

    rmt_transmit_config_t tx = {.loop_count = 0};
    ESP_ERROR_CHECK(rmt_transmit(s_chan, s_encoder, grb, sizeof(grb), &tx));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(s_chan, portMAX_DELAY));
}

static void button_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
}

// 轮询 + 去抖，检测一次完整的“按下”
static void button_task(void *arg)
{
    int last = 1;   // 上拉，空闲为高
    while (1) {
        int level = gpio_get_level(BUTTON_GPIO);
        if (last == 1 && level == 0) {          // 下降沿：可能按下
            vTaskDelay(pdMS_TO_TICKS(20));      // 去抖
            if (gpio_get_level(BUTTON_GPIO) == 0) {
                s_state = lamp_next(s_state);   // 纯逻辑算下一个状态
                ESP_LOGI(TAG, "press -> state %d", s_state);
                render(s_state);
                // 换完色记进 NVS，下次开机能恢复
                lamp_store_backend_t store = store_backend();
                if (!lamp_store_save(&store, s_state)) {
                    ESP_LOGW(TAG, "save state failed");
                }
                // 等松开，避免长按连发
                while (gpio_get_level(BUTTON_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
        last = level;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    // NVS 初始化（颜色记忆要用）。没空间/版本变了就擦了重来。
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    led_init();
    button_init();

    // 开机读回上次的颜色（从没存过 lamp_store 会给 LAMP_OFF），先点亮它。
    lamp_store_backend_t store = store_backend();
    s_state = lamp_store_load(&store);
    render(s_state);
    ESP_LOGI(TAG, "restored state %d; press BOOT to cycle OFF/RED/GREEN/BLUE", s_state);

    xTaskCreate(button_task, "button", 2048, NULL, 5, NULL);
}
