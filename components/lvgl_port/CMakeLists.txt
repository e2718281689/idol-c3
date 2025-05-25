idf_component_register(SRCS "lv_port_disp.c" "lv_port_tick.c" 
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "esp_lcd_st7789" "unity"
                        PRIV_REQUIRES espressif__esp_lvgl_port
                        )
