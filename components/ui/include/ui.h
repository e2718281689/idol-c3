#pragma once
#include "lvgl.h"

// 创建主界面
void create_main_screen(void);

// void wifi_view_show_qrcode(char* url);

void wifi_view_load_main();
void wifi_view_show_qrcode(const char * uri);
void wifi_view_update_status(const char * msg);
lv_obj_t * wifi_view_show_image(const char * image_path);