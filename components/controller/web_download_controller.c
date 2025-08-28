#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include "web_download.h"
#include "esp_log.h"

void download_file_task(void *pvParameters)
{

    esp_err_t err = 0;
    // 开始下载文件
    err = web_download_file("qianzhi");
    if (err != ESP_OK) {
        ESP_LOGE("download_file_task", "Failed to download file");
        wifi_view_update_status("Download failed");
        return;
    }
    else 
    {
        ESP_LOGI("download_file_task", "File downloaded successfully");
        wifi_view_show_image("/spiffs/qianzhi.bin");
    }

    vTaskDelete(NULL);

}


void download_file_task_prov(void)
{
    xTaskCreatePinnedToCore(download_file_task, "download_file_task", 8192, NULL, 10, NULL, 0);
}
