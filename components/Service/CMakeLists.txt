idf_component_register(SRCS "font/font.c"
                            "icons/icons.c"
                            "hb_sd/hb_sd.c"
                            "wifi/wifi_service.c"
                            "http_server/http_server_service.c"
                            "virtual_display_client/virtual_display_client.c"
                            "usb_stream/usb_stream.c"
                            "dns_server/dns_server.c"
                            "bluetooth/bluetooth_service.c"
                    INCLUDE_DIRS "font/include"
                                 "hb_sd/include"
                                 "icons/include"
                                 "wifi/include"
                                 "http_server/include"
                                 "dns_server/include"
                                 "virtual_display_client/include"
                                 "usb_stream/include"
                                 "bluetooth/include"
                    REQUIRES sdmmc
                    driver  
                    vfs
                    fatfs
                    esp_timer
                    esp_wifi
                    esp_common
                    esp_netif
                    Drivers 
                    esp_http_server
                    nvs_flash
                    
                    PRIV_REQUIRES
                    bt
                    )
