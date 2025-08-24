idf_component_register(SRCS "screen_prov.c" "screen_main.c" 
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "controller" "model" "lvgl"
                        PRIV_REQUIRES espressif__esp_lvgl_port lvgl__lvgl
                        )
