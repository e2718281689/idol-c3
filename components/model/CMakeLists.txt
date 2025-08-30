idf_component_register(SRCS "wifi_bt_net_model.c"
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "ui" "wifi_prov_mgr" "lvgl" "controller" "esp_http_client" "web_download"
                        # PRIV_REQUIRES espressif__esp_lvgl_port
                        )
