#ifndef WIFI_BT_NET_MODEL_H
#define WIFI_BT_NET_MODEL_H


#include "stdbool.h"
#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include "web_download.h"
#include "esp_log.h"


bool wifi_bt_net_model_init(void);
void wifi_bt_net_model_run(char* url);
void wifi_bt_net_model_net(void);
void wifi_bt_net_model_wait(void);

#endif /*WIFI_BT_NET_MODEL_H*/

