#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include "web_download.h"
#include "esp_log.h"
#include <esp_wifi_types_generic.h>
#include <esp_wifi.h>

static char *TAG = "web_download_controller";

void download_file_task(void *pvParameters)
{

    esp_err_t err = 0;
    char *download_name = "qianzhi";
    char download_file[100];
    char lvgl_show_file_url[200];

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi is not connected");

        wifi_view_update_status("Wi-Fi is not connected");
        vTaskDelay(pdMS_TO_TICKS(1000));
        goto err;
    }

    // 开始下载文件
    err = web_download_file_by_alias(download_name, download_file, 512);
    if (err == ESP_OK) { // Only proceed if download was successful
        ESP_LOGI(TAG, "File downloaded successfully");
        // snprintf(lvgl_show_file_url, sizeof(lvgl_show_file_url), "A:%s", download_file);
        snprintf(lvgl_show_file_url, sizeof(lvgl_show_file_url), "A:/sdcard/%s", download_file + 3);
        ESP_LOGI(TAG, "download_file %s", lvgl_show_file_url);
        wifi_view_show_image(lvgl_show_file_url);
        vTaskDelay(pdMS_TO_TICKS(1000));
    } else {
        ESP_LOGE(TAG, "Failed to download file");
        wifi_view_update_status("Download failed");
        vTaskDelay(pdMS_TO_TICKS(1000));
        // No return here, allow the task to proceed to cleanup and exit.
    }

err:
    wifi_view_load_main();
    vTaskDelete(NULL);
}


void download_file_task_prov(void)
{
    xTaskCreatePinnedToCore(download_file_task, "download_file_task", 8192, NULL, 10, NULL, 0);
}
