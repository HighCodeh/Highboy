idf_component_register(SRCS "home/home.c"
                            "menu/menu.c"
                            "menu_battery/menu_battery.c"
                            "sub_menu/sub_menu.c"
                            "ui_sd/ui_sd.c"
                            "music_menu/music_menu.c"
                            "ir_menu/ir_menu.c"
                       INCLUDE_DIRS "home/include"
                                    "menu/include"
                                    "menu_battery/include"
                                    "sub_menu/include"
                                    "ui_sd/include"
                                    "music_menu/include"
                                    "ir_menu/include"
                       REQUIRES 
                    driver
                    Drivers
                    Service
                 )