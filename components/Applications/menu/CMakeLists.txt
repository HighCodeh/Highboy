idf_component_register(SRCS "menu.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                    driver
                   Drivers
                    Applications
                    hb_sd
                    icons
                    sub_menu
                    music_menu
                    ir_menu
                    menu_generic)
