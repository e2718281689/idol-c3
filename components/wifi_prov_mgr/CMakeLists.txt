idf_component_register(SRCS "wifi_prov_mgr.c" 
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "esp_wifi" "nvs_flash" "wifi_provisioning" "esp_http_client" "ui" "model"
                        )
