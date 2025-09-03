#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H


#include "esp_err.h"


extern lv_display_t *lvgl_disp;

esp_err_t app_lcd_init(void);
esp_err_t app_lvgl_init(void);
esp_err_t app_lcd_deinit(void);
esp_err_t app_lvgl_deinit(void);

#endif /*LV_PORT_DISP_H*/