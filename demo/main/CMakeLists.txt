idf_component_register(SRCS "main.c" "led_strip_encoder.c"
                       INCLUDE_DIRS "."
                       REQUIRES lamp lamp_store nvs_flash driver esp_driver_rmt)
