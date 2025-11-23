idf_component_register(SRCS "bq25896/bq25896.c"
                            "buzzer/buzzer.c"
                            "cc1101/cc1101.c"
                            "pn7150/pn7150.c" 
                            "st7789/st7789.c"
                            "led/led_control.c"
                            "backlight/backlight.c"
                            "spi/spi.c"
                            "i2c_init/i2c_init.c"
                            "tusb_desc/tusb_desc.c"
               INCLUDE_DIRS "bq25896/include"
                            "buzzer/include"
                            "cc1101/include"
                            "pn7150/include"
                            "st7789/include"
                            "led/include"
                            "pins/include"
                            "spi/include"
                            "i2c_init/include"
                            "backlight/include"
                            "tusb_desc/include"
            REQUIRES 
            driver
            Service
            PRIV_REQUIRES 
            esp_tinyusb
            "espressif__led_strip")
