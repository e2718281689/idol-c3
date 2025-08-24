#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"


void wifi_prov_task(void *pv)
{
    char url[150]={0};
    wifi_bt_net_init();
    if(1)
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
    wifi_view_load_main();

    vTaskDelete(NULL);
}

void wifi_controller_start_prov(void)
{
    xTaskCreatePinnedToCore(wifi_prov_task, "wifi_prov_task", 8192, NULL, 10, NULL, 0);
}