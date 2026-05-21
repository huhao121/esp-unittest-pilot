#include "lamp.h"

lamp_state_t lamp_next(lamp_state_t current)
{
    // 非法输入安全退回到关
    if (current < LAMP_OFF || current >= LAMP_STATE_COUNT) {
        return LAMP_OFF;
    }
    // 枚举顺序就是循环顺序，取模实现 BLUE -> OFF 的回绕
    return (lamp_state_t)((current + 1) % LAMP_STATE_COUNT);
}

lamp_rgb_t lamp_color(lamp_state_t state)
{
    // 满量程颜色，亮度调节交给硬件层；非法状态当作关 -> 黑
    switch (state) {
        case LAMP_RED:   return (lamp_rgb_t){.r = 255, .g = 0,   .b = 0};
        case LAMP_GREEN: return (lamp_rgb_t){.r = 0,   .g = 255, .b = 0};
        case LAMP_BLUE:  return (lamp_rgb_t){.r = 0,   .g = 0,   .b = 255};
        case LAMP_OFF:
        default:         return (lamp_rgb_t){.r = 0,   .g = 0,   .b = 0};
    }
}
