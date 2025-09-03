#include "wifi_bt_net_model.h"
#include "wifi_prov_mgr.h"
#include <stdio.h>
#include <string.h>


bool wifi_bt_net_model_init(void)
{
    return wifi_bt_net_init();
}

void wifi_bt_net_model_run(char* url)
{
    wifi_bt_net_run(url);
}

void wifi_bt_net_model_net(void)
{
    wifi_bt_net_net();
}

void wifi_bt_net_model_wait(void)
{
    wifi_bt_net_wait();
}
