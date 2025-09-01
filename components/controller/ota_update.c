#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"

static char *TAG = "ota_update_controller";

#define DEVICE_MODEL "esp32-c3"
#define CURRENT_FIRMWARE_VERSION "0.9.0"

void ota_update_prov_task(void *pv)
{
    char ota_file_path[150]; // 准备一个足够大的缓冲区

    ota_status_t ota_status = web_ota_check_and_download(
        DEVICE_MODEL, 
        CURRENT_FIRMWARE_VERSION,
        ota_file_path,
        sizeof(ota_file_path)
    );

    switch(ota_status) {
        case OTA_UPDATE_SUCCESSFUL:
            ESP_LOGI(TAG, "OTA Success: New firmware downloaded to '%s'", ota_file_path);
            // 在这里，您可以调用esp_ota_*函数来执行更新
            // e.g., perform_firmware_update(ota_file_path);
            break;
        case OTA_NO_UPDATE_AVAILABLE:
            ESP_LOGI(TAG, "OTA Info: No new updates available.");
            break;
        case OTA_CHECK_FAILED:
            ESP_LOGE(TAG, "OTA Error: Failed to check or download update.");
            break;
    }

    wifi_view_load_main();

    vTaskDelete(NULL);
}

void ota_update_start_prov(void)
{
    xTaskCreatePinnedToCore(ota_update_prov_task, "ota_update_task", 8192, NULL, 10, NULL, 0);
}