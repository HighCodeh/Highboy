idf_component_register(SRCS "home/home.c"
                            "menu/menu.c"
                            "UART/UART.c"
                            "GPIO/GPIO.c"
                            "sub_menu/sub_menu.c"
                            "ui_sd/ui_sd.c"
                            "menu_generic/menu_generic.c"
                            "play/play.c"
                            "wifi/wifi.c"
                            "wifi/wifi_deauther.c"
                            "wifi/evil_twin.c"
                            "brightness_ui/brightness_ui.c"
                            "bad_usb/bad_usb.c"                           
                            "bad_usb/bad_usb_menu.c"
                            "battery_ui/battery_ui.c"
                            "keyboard_ui/keyboard_ui.c"
                       INCLUDE_DIRS "home/include"
                                    "menu/include"
                                    "UART/include"
                                    "GPIO/include"
                                  "keyboard_ui/include"
                                    "sub_menu/include"
                                    "ui_sd/include"
                                    "menu_generic/include"
                                    "play/include"
                                    "wifi/include"   
                                    "brightness_ui/include"    
                                    "bad_usb/include"
                                    "battery_ui/include"
                       REQUIRES 
                    driver
                    Drivers
                    Service
                    esp_common
                    esp_wifi
                    esp_tinyusb
                    bt
                 )
                 target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)

