idf_component_register(SRCS   "web_download.c"
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "esp_wifi" "nvs_flash" "wifi_provisioning" "esp_http_client"
                        )
