        idf_component_register(SRCS "font/font.c"
                                "icons/icons.c"
                                "wifi/wifi_service.c"
                                "http_server/http_server_service.c"
                                "virtual_display_client/virtual_display_client.c"
                                "usb_stream/usb_stream.c"
                                "dns_server/dns_server.c"
                                "bluetooth/bluetooth_service.c"

                                "sd_card/sd_card_dir.c"
                                "sd_card/sd_card_file.c"
                                "sd_card/sd_card_info.c"
                                "sd_card/sd_card_init.c"
                                "sd_card/sd_card_read.c"
                                "sd_card/sd_card_write.c"

                                "ir/ir_encoder.c"
                                "ir/ir_common.c"
                                "ir/ir_tx.c"
                                "ir/ir_rx.c"
                                "ir/ir_storage.c"
                                "ir/protocol_nec.c"
                                "ir/protocol_rc6.c"
                                "ir/protocol_rc5.c"
                                "ir/protocol_samsung32.c"
                                "ir/protocol_sony.c"
                                "ir/ir_burst.c"

                    INCLUDE_DIRS "font/include"
                                 "icons/include"
                                 "wifi/include"
                                 "http_server/include"
                                 "dns_server/include"
                                 "virtual_display_client/include"
                                 "usb_stream/include"
                                 "bluetooth/include"
                                 "ir/include"
                                 "sd_card/include"
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
                    bt 
                    )
