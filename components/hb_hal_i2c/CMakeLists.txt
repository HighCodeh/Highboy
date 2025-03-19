idf_component_register(SRCS "hb_hal_i2c.c"
    INCLUDE_DIRS "include"            # Diretório de include para o SPI HAL
    REQUIRES driver             # Adiciona a dependência do componente driver do ESP-IDF
)