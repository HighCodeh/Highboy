idf_component_register(
    SRCS "ir_processor.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
            driver
            esp_system
            esp_timer
            driver 
            fatfs 
            sdmmc
            "ir_decoder" "ir_path" "ir_protocol" "ir_debug" "ir_file_generator"
)