set(reqs unity lamp_store)

# 真重启测试要用真 NVS（nvs_flash）和 esp_restart（esp_system），这些只在板子/QEMU 上编。
# host(linux) 不拉这俩，保持和 lamp 一样精简，host CI 不受影响。
if(NOT CONFIG_IDF_TARGET_LINUX)
    list(APPEND reqs nvs_flash esp_system)
endif()

idf_component_register(SRCS "test_lamp_store.c"
                       INCLUDE_DIRS "."
                       REQUIRES ${reqs}
                       WHOLE_ARCHIVE)
