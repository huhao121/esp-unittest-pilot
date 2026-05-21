#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 灯的状态。按钮每按一次按 OFF -> RED -> GREEN -> BLUE -> OFF 循环。
 * 枚举值的顺序就是循环顺序，lamp_next() 依赖这个顺序。
 */
typedef enum {
    LAMP_OFF = 0,
    LAMP_RED,
    LAMP_GREEN,
    LAMP_BLUE,
    LAMP_STATE_COUNT,   // 状态个数，方便取模循环；不是有效状态
} lamp_state_t;

/**
 * 一个 RGB 颜色，每个分量 0-255。
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} lamp_rgb_t;

/**
 * 算出"再按一次按钮"后的状态。
 * 纯函数：只依赖入参，无副作用。
 *
 * @param current 当前状态
 * @return 下一个状态，BLUE 之后回到 OFF
 */
lamp_state_t lamp_next(lamp_state_t current);

/**
 * 把状态翻译成 RGB 颜色。
 * 纯函数：OFF -> 黑(0,0,0)，RED/GREEN/BLUE -> 对应满量程颜色。
 *
 * @param state 灯的状态
 * @return 对应的 RGB 颜色
 */
lamp_rgb_t lamp_color(lamp_state_t state);

#ifdef __cplusplus
}
#endif
