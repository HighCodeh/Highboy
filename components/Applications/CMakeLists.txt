idf_component_register(SRCS "home/home.c"
                            "menu/menu.c"
                            "sub_menu/sub_menu.c"
                            "ui_sd/ui_sd.c"
                         
                            "menu_generic/menu_generic.c"
                            "play/play.c"
                            "wifi/wifi.c"
                            "wifi/wifi_deauther.c"
                            "wifi/evil_twin.c"
                            "brightness_ui/brightness_ui.c"
                            "led_ui/led_ui.c"
                            "info_device/info_device.c"
                            "buzzer_ui/buzzer_ui.c"
                            "battery_ui/battery_ui.c"
                       INCLUDE_DIRS "home/include"
                                    "menu/include"
                                    "sub_menu/include"
                                    "ui_sd/include"
                                  
                                    "menu_generic/include"
                                    "play/include"
                                    "wifi/include" 
                                    "brightness_ui/include"
                                    "led_ui/include"
                                    "info_device/include"
                                    "buzzer_ui/include"
                                    "battery_ui/include"
                  REQUIRES 
                    driver
                    Drivers
                    Service
                    esp_common
                    esp_wifi
                 )
                 target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)

