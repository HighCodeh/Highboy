idf_component_register(
    SRCS "ir_receiver.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
            driver
            esp_system
            esp_timer
            driver 
            fatfs 
            sdmmc
)