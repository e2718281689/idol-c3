/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7789v3.h"
#include "esp_littlefs.h"
#include "esp_task_wdt.h"
#include "lv_port_tick.h"
#include "sht40.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"

static char *TAG = "main";

SemaphoreHandle_t spi_mutex;

void app_main(void)
{

    const esp_app_desc_t *app_desc = esp_app_get_description();
    if (app_desc) {
    ESP_LOGI(TAG, "Project Name: %s", app_desc->project_name);
    ESP_LOGI(TAG, "Version: %s", app_desc->version);
    ESP_LOGI(TAG, "Compile Time: %s %s", app_desc->date, app_desc->time);
    ESP_LOGI(TAG, "IDF Version: %s", app_desc->idf_ver);
    } else {
    ESP_LOGE(TAG, "Failed to get application description");
    }


    spi_mutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    printf("TEST ESP LVGL port\n\r");


    xTaskCreatePinnedToCore(lvgl_task, "taskLVGL", 8192, NULL, 10, NULL, 0);

}
