#pragma once

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

esp_err_t web_download_file(char *url, char* FILE_url);

