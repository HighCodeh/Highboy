idf_component_register(SRCS "home/home.c"
                            "menu/menu.c"
                            "menu_battery/menu_battery.c"
                            "sub_menu/sub_menu.c"
                            "ui_sd/ui_sd.c"
                            "music_menu/music_menu.c"
                            "menu_generic/menu_generic.c"
                            "play/play.c"
                            "wifi/wifi.c"
                       INCLUDE_DIRS "home/include"
                                    "menu/include"
                                    "menu_battery/include"
                                    "sub_menu/include"
                                    "ui_sd/include"
                                    "music_menu/include"
                                    "menu_generic/include"
                                    "play/include"
                                    "wifi/include" 
                       REQUIRES 
                    driver
                    Drivers
                    Service
                    esp_common
                    esp_wifi
                 )
                 target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)

