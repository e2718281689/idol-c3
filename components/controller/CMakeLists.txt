idf_component_register(SRCS "wifi_controller.c" "web_download_controller.c"  "ota_update.c"
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "web_download" "ui" "model" "wifi_prov_mgr"
                        PRIV_REQUIRES espressif__esp_lvgl_port
                        )
