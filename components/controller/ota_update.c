#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include <esp_wifi_types_generic.h>
#include <esp_wifi.h>


static char *TAG = "ota_update_controller";

#define DEVICE_MODEL "esp32-c3"
// #define CURRENT_FIRMWARE_VERSION "0.9.0"

void ota_update_prov_task(void *pv)
{
    char ota_file_path[150]; // 准备一个足够大的缓冲区


    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi is not connected");

        wifi_view_update_status("Wi-Fi is not connected");
        vTaskDelay(pdMS_TO_TICKS(1000));
        goto err;
    }


    const esp_app_desc_t *app_desc = esp_app_get_description();

    ota_status_t ota_status = web_ota_check_and_download(
        DEVICE_MODEL, 
        app_desc->version, // 使用从描述符中获取的版本
        ota_file_path,
        sizeof(ota_file_path)
    );

    switch(ota_status) {
        case OTA_UPDATE_SUCCESSFUL:
            ESP_LOGW(TAG, "Update available! Preparing to switch to OTA updater app...");

            // 1. 查找专门用于OTA的 "factory" 分区
            const esp_partition_t *ota_updater_partition = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP,         // 分区类型: app
                ESP_PARTITION_SUBTYPE_APP_FACTORY, // 分区子类型: factory
                NULL                            // 分区名: 任意
            );

            if (ota_updater_partition == NULL) {
                ESP_LOGE(TAG, "FATAL: Could not find 'factory' partition. Check your partition table.");
                break; // 无法继续，中断操作
            }
            // ESP_LOGI(TAG, "Found OTA updater partition ('factory') at offset 0x%x", ota_updater_partition->address);

            // 2. 设置下一次启动时从 "factory" 分区启动
            esp_err_t err = esp_ota_set_boot_partition(ota_updater_partition);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set boot partition to 'factory'. Error: %s", esp_err_to_name(err));
            } else {
                ESP_LOGW(TAG, "Boot partition successfully set to 'factory'. Rebooting into OTA mode in 2 seconds...");
                // 短暂延时，确保日志可以被完整打印
                vTaskDelay(pdMS_TO_TICKS(2000));
                // 3. 重启设备
                esp_restart();
                // esp_restart() 函数不会返回，下面的代码不会被执行
            }
            break;
        case OTA_NO_UPDATE_AVAILABLE:
            ESP_LOGI(TAG, "OTA Info: No new updates available.");
            wifi_view_update_status("No new updates available");
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;
        case OTA_CHECK_FAILED:
            ESP_LOGE(TAG, "OTA Error: Failed to check or download update.");
            wifi_view_update_status("Failed to check or download update");
            vTaskDelay(pdMS_TO_TICKS(1000));
            break;
    }

err:
    wifi_view_load_main();
    vTaskDelete(NULL);
}

void ota_update_start_prov(void)
{
    xTaskCreatePinnedToCore(ota_update_prov_task, "ota_update_task", 8192, NULL, 10, NULL, 0);
}