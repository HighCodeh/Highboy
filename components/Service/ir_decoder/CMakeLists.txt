idf_component_register(
    SRCS "ir_decoder.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
            driver
            esp_system
            esp_timer
            driver 
            fatfs 
            sdmmc
)