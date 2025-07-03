idf_component_register(SRCS "font/font.c"
                            "icons/icons.c"
                            "hb_sd/hb_sd.c"
                            "ir_transmiter/ir_transmiter.c"
                    INCLUDE_DIRS "font/include"
                                 "hb_sd/include"
                                 "icons/include"
                                 "ir_transmiter/include"
                    REQUIRES sdmmc
                    driver  
                    vfs
                    fatfs
                    esp_timer
                    Drivers 
                    Applications
                    Core)
