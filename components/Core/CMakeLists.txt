idf_component_register(SRCS "hb_hal_button/hb_hal_button.c"
                            "hb_hal_i2c/hb_hal_i2c.c" 
                            "hb_hal_gpio/hb_hal_gpio.c" 
                       INCLUDE_DIRS "hb_hal_button/include"
                                    "hb_hal_i2c/include"
                                    "hb_hal_gpio/include"
                       REQUIRES driver)