#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include "web_download.h"
#include "esp_log.h"

static char *TAG = "web_download_controller";

void download_file_task(void *pvParameters)
{

    esp_err_t err = 0;
    char *download_name = "qianzhi";
    char download_file[100];
    char lvgl_show_file_url[200];

    // 开始下载文件
    err = web_download_file(download_name, download_file);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to download file");
        wifi_view_update_status("Download failed");
        return;
    }
    else 
    {
        ESP_LOGI(TAG, "File downloaded successfully");
        snprintf(lvgl_show_file_url, sizeof(lvgl_show_file_url), "A:%s", download_file);
        ESP_LOGI(TAG, "download_file %s", lvgl_show_file_url);
        wifi_view_show_image(lvgl_show_file_url);
    }

    vTaskDelete(NULL);

}


void download_file_task_prov(void)
{
    xTaskCreatePinnedToCore(download_file_task, "download_file_task", 8192, NULL, 10, NULL, 0);
}
