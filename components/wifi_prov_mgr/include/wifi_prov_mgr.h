#ifndef WIFI_PROV_MGR_H
#define WIFI_PROV_MGR_H

#include <stdbool.h>

void wifi_prov_mgr(void);
bool wifi_bt_net_init(void);
void wifi_bt_net_run(char* url);
void wifi_bt_net_net();
void wifi_bt_net_wait(void);

#endif /*WIFI_PROV_MGR_H*/