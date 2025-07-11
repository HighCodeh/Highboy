idf_component_register(
    SRCS "ir_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
            driver
            esp_system
            esp_timer
            driver 
            fatfs 
            sdmmc
            "ir_receiver" "ir_transmitter" "ir_protocol" "ir_file" "ir_encoder" "ir_path"
)