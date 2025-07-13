idf_component_register(
    SRCS "ir_debug.c" "ir_decoder.c" "ir_encoder.c" "ir_file_generator.c" "ir_file.c" "ir_manager.c" "ir_path.c" "ir_processor.c" "ir_protocol.c" "ir_receiver.c" "ir_transmitter.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
            driver
            esp_system
            esp_timer
            driver 
            fatfs 
            sdmmc
)