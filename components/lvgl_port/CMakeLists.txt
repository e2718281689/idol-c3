idf_component_register(SRCS "lv_port_disp.c" "lv_port_tick.c" "lv_port_indev.c"
                        INCLUDE_DIRS "include" 
                        REQUIRES "driver" "esp_lcd_st7789" "unity" "esp_adc" "fatfs"
                        PRIV_REQUIRES espressif__esp_lvgl_port
                        )
