#pragma once

#include "stdbool.h"

bool wifi_bt_net_model_init(void);
void wifi_bt_net_model_run(char* url);
void wifi_bt_net_model_net(void);
void wifi_bt_net_model_wait(void);

