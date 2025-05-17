/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdlib.h>
 #include <sys/cdefs.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_lcd_panel_interface.h"
 #include "esp_lcd_panel_io.h"
 #include "esp_lcd_panel_vendor.h"
 #include "esp_lcd_panel_ops.h"
 #include "esp_lcd_panel_commands.h"
 #include "driver/gpio.h"
 #include "esp_log.h"
 #include "esp_check.h"
 #include "esp_lcd_st7789v3.h"
 
 static const char *TAG = "st7789v3";
 
 static esp_err_t panel_st7789v3_del(esp_lcd_panel_t *panel);
 static esp_err_t panel_st7789v3_reset(esp_lcd_panel_t *panel);
 static esp_err_t panel_st7789v3_init(esp_lcd_panel_t *panel);
 static esp_err_t panel_st7789v3_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
 static esp_err_t panel_st7789v3_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
 static esp_err_t panel_st7789v3_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
 static esp_err_t panel_st7789v3_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
 static esp_err_t panel_st7789v3_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
 static esp_err_t panel_st7789v3_disp_on_off(esp_lcd_panel_t *panel, bool off);
 
 typedef struct {
     esp_lcd_panel_t base;
     esp_lcd_panel_io_handle_t io;
     int reset_gpio_num;
     bool reset_level;
     int x_gap;
     int y_gap;
     uint8_t fb_bits_per_pixel;
     uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
     uint8_t colmod_cal; // save current value of LCD_CMD_COLMOD register
     uint16_t width;
     uint16_t height;
 } st7789v3_panel_t;
 
 esp_err_t esp_lcd_new_panel_st7789v3(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
 {
     esp_err_t ret = ESP_OK;
     st7789v3_panel_t *st7789v3 = NULL;
     ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
     st7789v3 = calloc(1, sizeof(st7789v3_panel_t));
     ESP_GOTO_ON_FALSE(st7789v3, ESP_ERR_NO_MEM, err, TAG, "no mem for st7789v3 panel");
 
     if (panel_dev_config->reset_gpio_num >= 0) {
         gpio_config_t io_conf = {
             .mode = GPIO_MODE_OUTPUT,
             .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
         };
         ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
     }
 
     switch (panel_dev_config->color_space) {
     case ESP_LCD_COLOR_SPACE_RGB:
         st7789v3->madctl_val = 0;
         break;
     case ESP_LCD_COLOR_SPACE_BGR:
         st7789v3->madctl_val |= LCD_CMD_BGR_BIT;
         break;
     default:
         ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
         break;
     }
 
     uint8_t fb_bits_per_pixel = 0;
     switch (panel_dev_config->bits_per_pixel) {
     case 16: // RGB565
         st7789v3->colmod_cal = 0x55;
         fb_bits_per_pixel = 16;
         break;
     case 18: // RGB666
         st7789v3->colmod_cal = 0x66;
         fb_bits_per_pixel = 24;
         break;
     default:
         ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
         break;
     }
 
     st7789v3->io = io;
     st7789v3->fb_bits_per_pixel = fb_bits_per_pixel;
     st7789v3->reset_gpio_num = panel_dev_config->reset_gpio_num;
     st7789v3->reset_level = panel_dev_config->flags.reset_active_high;
    //  st7789v3->width = panel_dev_config->resolution.width;
    //  st7789v3->height = panel_dev_config->resolution.height;
     st7789v3->base.del = panel_st7789v3_del;
     st7789v3->base.reset = panel_st7789v3_reset;
     st7789v3->base.init = panel_st7789v3_init;
     st7789v3->base.draw_bitmap = panel_st7789v3_draw_bitmap;
     st7789v3->base.invert_color = panel_st7789v3_invert_color;
     st7789v3->base.set_gap = panel_st7789v3_set_gap;
     st7789v3->base.mirror = panel_st7789v3_mirror;
     st7789v3->base.swap_xy = panel_st7789v3_swap_xy;
 #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
     st7789v3->base.disp_off = panel_st7789v3_disp_on_off;
 #else
     st7789v3->base.disp_on_off = panel_st7789v3_disp_on_off;
 #endif
     *ret_panel = &(st7789v3->base);
     ESP_LOGD(TAG, "new st7789v3 panel @%p", st7789v3);
 
     return ESP_OK;
 
 err:
     if (st7789v3) {
         if (panel_dev_config->reset_gpio_num >= 0) {
             gpio_reset_pin(panel_dev_config->reset_gpio_num);
         }
         free(st7789v3);
     }
     return ret;
 }
 
 static esp_err_t panel_st7789v3_del(esp_lcd_panel_t *panel)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
 
     if (st7789v3->reset_gpio_num >= 0) {
         gpio_reset_pin(st7789v3->reset_gpio_num);
     }
     ESP_LOGD(TAG, "del st7789v3 panel @%p", st7789v3);
     free(st7789v3);
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_reset(esp_lcd_panel_t *panel)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     esp_lcd_panel_io_handle_t io = st7789v3->io;
 
     // perform hardware reset
     if (st7789v3->reset_gpio_num >= 0) {
         gpio_set_level(st7789v3->reset_gpio_num, st7789v3->reset_level);
         vTaskDelay(pdMS_TO_TICKS(10));
         gpio_set_level(st7789v3->reset_gpio_num, !st7789v3->reset_level);
         vTaskDelay(pdMS_TO_TICKS(10));
     } else { // perform software reset
         esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
         vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5ms before sending new command
     }
 
     return ESP_OK;
 }
 
 typedef struct {
     uint8_t cmd;
     uint8_t data[16];
     uint8_t data_bytes; // Length of data in above data array; 0xFF = end of cmds.
 } lcd_init_cmd_t;
 
 static const lcd_init_cmd_t vendor_specific_init[] = {
     /* Software reset */
     {LCD_CMD_SWRESET, {0}, 0},
     /* Delay 150ms */
     {0x80, {150}, 1},
     /* Sleep out */
     {LCD_CMD_SLPOUT, {0}, 0},
     /* Delay 500ms */
     {0x80, {250}, 1},
     {0x80, {250}, 1},
     /* Interface Pixel Format, 16bit/pixel for RGB/MCU interface */
     {LCD_CMD_COLMOD, {0x55}, 1},
     /* Memory Data Access Control */
     {LCD_CMD_MADCTL, {0x00}, 1},
     /* Display Inversion On */
     {LCD_CMD_INVON, {0}, 0},
     /* Normal display mode on */
     {LCD_CMD_NORON, {0}, 0},
     /* Frame rate control */
     {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5},
     /* Display Function Control */
     {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
     /* Power Control 1 */
     {0xC0, {0x25}, 1},
     /* Power Control 2 */
     {0xC1, {0x11}, 1},
     /* VCOM Control 1 */
     {0xC5, {0x35, 0x3E}, 2},
     /* VCOM Control 2 */
     {0xC7, {0xBE}, 1},
     /* Gamma Positive Correction */
     {0xE0, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17}, 14},
     /* Gamma Negative Correction */
     {0xE1, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E}, 14},
     /* Display on */
     {LCD_CMD_DISPON, {0}, 0},
     /* Delay 100ms */
     {0x80, {100}, 1},
     {0, {0}, 0xff},
 };
 
 static esp_err_t panel_st7789v3_init(esp_lcd_panel_t *panel)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     esp_lcd_panel_io_handle_t io = st7789v3->io;
 
     // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
     esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
     vTaskDelay(pdMS_TO_TICKS(120));
     
     // Set pixel format
     esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD, (uint8_t[]) {
         st7789v3->colmod_cal,
     }, 1);
     
     // Set memory data access control
     esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
         st7789v3->madctl_val,
     }, 1);
 
     // vendor specific initialization
     int cmd = 0;
     while (vendor_specific_init[cmd].data_bytes != 0xff) {
         esp_lcd_panel_io_tx_param(io, vendor_specific_init[cmd].cmd, vendor_specific_init[cmd].data, vendor_specific_init[cmd].data_bytes & 0x1F);
         cmd++;
     }
 
     // Set display window
     uint8_t col_data[4] = {
         0x00, 0x00,
         (uint8_t)((st7789v3->width - 1) >> 8),
         (uint8_t)(st7789v3->width - 1)
     };
     uint8_t row_data[4] = {
         0x00, 0x00,
         (uint8_t)((st7789v3->height - 1) >> 8),
         (uint8_t)(st7789v3->height - 1)
     };
     
     esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, col_data, 4);
     esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, row_data, 4);
 
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
     esp_lcd_panel_io_handle_t io = st7789v3->io;
 
     x_start += st7789v3->x_gap;
     x_end += st7789v3->x_gap;
     y_start += st7789v3->y_gap;
     y_end += st7789v3->y_gap;
 
     // define an area of frame memory where MCU can access
     uint8_t col_data[4] = {
         (uint8_t)(x_start >> 8),
         (uint8_t)(x_start & 0xff),
         (uint8_t)((x_end - 1) >> 8),
         (uint8_t)((x_end - 1) & 0xff)
     };
     uint8_t row_data[4] = {
         (uint8_t)(y_start >> 8),
         (uint8_t)(y_start & 0xff),
         (uint8_t)((y_end - 1) >> 8),
         (uint8_t)((y_end - 1) & 0xff)
     };
     
     esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, col_data, 4);
     esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, row_data, 4);
     
     // transfer frame buffer
     size_t len = (x_end - x_start) * (y_end - y_start) * st7789v3->fb_bits_per_pixel / 8;
     esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);
 
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     esp_lcd_panel_io_handle_t io = st7789v3->io;
     int command = 0;
     if (invert_color_data) {
         command = LCD_CMD_INVON;
     } else {
         command = LCD_CMD_INVOFF;
     }
     esp_lcd_panel_io_tx_param(io, command, NULL, 0);
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     esp_lcd_panel_io_handle_t io = st7789v3->io;
     if (mirror_x) {
         st7789v3->madctl_val |= LCD_CMD_MX_BIT;
     } else {
         st7789v3->madctl_val &= ~LCD_CMD_MX_BIT;
     }
     if (mirror_y) {
         st7789v3->madctl_val |= LCD_CMD_MY_BIT;
     } else {
         st7789v3->madctl_val &= ~LCD_CMD_MY_BIT;
     }
     esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
         st7789v3->madctl_val
     }, 1);
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     esp_lcd_panel_io_handle_t io = st7789v3->io;
     if (swap_axes) {
         st7789v3->madctl_val |= LCD_CMD_MV_BIT;
     } else {
         st7789v3->madctl_val &= ~LCD_CMD_MV_BIT;
     }
     esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
         st7789v3->madctl_val
     }, 1);
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     st7789v3->x_gap = x_gap;
     st7789v3->y_gap = y_gap;
     return ESP_OK;
 }
 
 static esp_err_t panel_st7789v3_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
 {
     st7789v3_panel_t *st7789v3 = __containerof(panel, st7789v3_panel_t, base);
     esp_lcd_panel_io_handle_t io = st7789v3->io;
     int command = 0;
 
 #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
     on_off = !on_off;
 #endif
 
     if (on_off) {
         command = LCD_CMD_DISPON;
     } else {
         command = LCD_CMD_DISPOFF;
     }
     esp_lcd_panel_io_tx_param(io, command, NULL, 0);
     return ESP_OK;
 }