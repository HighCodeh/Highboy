idf_component_register(
    SRCS "ir_debug.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
            driver
            esp_system
            esp_timer
            driver 
            fatfs 
            sdmmc
            "ir_decoder" "ir_path"
)