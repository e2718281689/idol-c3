#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include <esp_wifi_types_generic.h>
#include <esp_wifi.h>

static char *TAG = "wifi bt connect";


void wifi_prov_task(void *pv)
{
    char url[150]={0};
    
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi is already connected");

        wifi_view_update_status("Wi-Fi is already connected");
        vTaskDelay(pdMS_TO_TICKS(1000));
        goto err;
    }

    if(!wifi_bt_net_init())
    {
       wifi_view_update_status("wifi bt net");
       wifi_bt_net_run(url);
       wifi_view_show_qrcode(url);
    }
    else
    {
        wifi_bt_net_net();
    }

    // wifi_bt_net_init();
    // wifi_bt_net_run(url);
    // wifi_bt_net_net();

    wifi_bt_net_wait();
    wifi_view_update_status("over connect Wi-Fi...");

    vTaskDelay(pdMS_TO_TICKS(1000));

err:
    wifi_view_load_main();
    vTaskDelete(NULL);
}

void wifi_controller_start_prov(void)
{
    xTaskCreatePinnedToCore(wifi_prov_task, "wifi_prov_task", 8192, NULL, 10, NULL, 0);
}