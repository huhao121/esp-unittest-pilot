#include <stddef.h>   // NULL
#include "lamp_store.h"

bool lamp_store_save(const lamp_store_backend_t *backend, lamp_state_t state)
{
    if (backend == NULL || backend->save == NULL) {
        return false;
    }
    // 非法状态退回到关，绝不把垃圾写进存储
    if (state < LAMP_OFF || state >= LAMP_STATE_COUNT) {
        state = LAMP_OFF;
    }
    return backend->save(backend->ctx, (uint8_t)state);
}

lamp_state_t lamp_store_load(const lamp_store_backend_t *backend)
{
    if (backend == NULL || backend->load == NULL) {
        return LAMP_OFF;
    }
    uint8_t raw = 0;
    // 从没存过 -> 当灭的
    if (!backend->load(backend->ctx, &raw)) {
        return LAMP_OFF;
    }
    // 存进去的是非法值（旧格式 / 损坏）-> 也当灭的
    // raw 是 uint8_t，下界天然 >= LAMP_OFF(0)，只需查上界
    if (raw >= LAMP_STATE_COUNT) {
        return LAMP_OFF;
    }
    return (lamp_state_t)raw;
}
