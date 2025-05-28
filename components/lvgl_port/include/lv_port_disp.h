#pragma once


#include "esp_err.h"

esp_err_t app_lcd_init(void);
esp_err_t app_lvgl_init(void);
esp_err_t app_lcd_deinit(void);
esp_err_t app_lvgl_deinit(void);