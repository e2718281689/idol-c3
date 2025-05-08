idf_component_register(SRCS "spi_lcd_touch_example_main.c" "lvgl_demo_ui.c" "esp_lcd_st7789v3.c"
                       INCLUDE_DIRS ".")
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security;-Wformat=0" APPEND)
# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(storage ../image FLASH_IN_PROJECT)