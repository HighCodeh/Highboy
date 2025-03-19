idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
    hb_hal_core
    hb_hal_i2c
    hb_hal_gpio
    hb_hal_button

)
