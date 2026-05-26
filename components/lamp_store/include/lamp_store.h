#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lamp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 持久化后端：薄薄一层胶水，只负责存/取一个字节。
 *
 * 逻辑层（lamp_store）不直接碰 NVS —— 掉电不丢的存储由这个后端提供：
 *  - host 单测里塞一个内存假后端；
 *  - 真板子上由 demo 里的 NVS 胶水实现。
 */
typedef struct {
    /**
     * 读回上次存的字节。
     * @param ctx   后端自己的上下文
     * @param out   存过则把值写到这里
     * @return 存过返回 true（并写 *out）；从没存过返回 false（不动 *out）
     */
    bool (*load)(void *ctx, uint8_t *out);

    /**
     * 存一个字节（要求掉电不丢）。
     * @return 成功返回 true
     */
    bool (*save)(void *ctx, uint8_t value);

    void *ctx;   // 透传给上面两个回调
} lamp_store_backend_t;

/**
 * 把灯当前状态存进掉电不丢的存储。
 * 非法状态按 LAMP_OFF 存，保证读回来时一定是个有效值。
 *
 * @param backend 持久化后端，NULL 安全（直接返回 false）
 * @param state   要记住的状态
 * @return 后端写入成功返回 true
 */
bool lamp_store_save(const lamp_store_backend_t *backend, lamp_state_t state);

/**
 * 开机读回上次存的状态。
 * 从没存过、读失败、或存进去的是非法值 —— 一律当成灭的（LAMP_OFF）。
 *
 * @param backend 持久化后端，NULL 安全（直接返回 LAMP_OFF）
 * @return 上次的状态，没有则 LAMP_OFF
 */
lamp_state_t lamp_store_load(const lamp_store_backend_t *backend);

#ifdef __cplusplus
}
#endif
