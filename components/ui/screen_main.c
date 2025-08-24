#include "lvgl.h"
#include "controller.h"
// #include "lv_qrcode.h"

static lv_obj_t * main_scr;   // 主界面对象
static lv_obj_t * task_scr;   // 任务界面对象
static lv_obj_t * label_task; // 任务界面文本


void wifi_view_load_main()
{
    lv_scr_load(main_scr);
}

void wifi_view_show_qrcode(const char * uri)
{
    lv_obj_t * qr = lv_qrcode_create(lv_scr_act(), 150, lv_color_black(), lv_color_white());
    lv_qrcode_update(qr, uri, strlen(uri));
    lv_obj_center(qr);
}

void wifi_view_update_status(const char * msg)
{
    if (!label_task) {
        label_task = lv_label_create(lv_scr_act());
        lv_obj_align(label_task, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
    lv_label_set_text(label_task, msg);
}


// // 后台任务
// static void long_task(void * pvParameter)
// {

//     lv_label_set_text(label_task, "wifi_prov_mgr!");

//     wifi_prov_mgr();

//     lv_label_set_text(label_task, "over!");

//     // 保持显示一会儿
//     vTaskDelay(pdMS_TO_TICKS(1000));

//     // 返回主界面
//     lv_scr_load(main_scr);

//     // 删除这个任务
//     vTaskDelete(NULL);
// }

// 按钮事件：进入任务界面
static void btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) 
    {


        task_scr = lv_obj_create(NULL);

        label_task = lv_label_create(task_scr);
        lv_obj_center(label_task);
        lv_label_set_text(label_task, "ready ...");

        lv_scr_load(task_scr);

        wifi_controller_start_prov();
    }
}

// 创建主界面
void create_main_screen(void)
{
    main_scr = lv_obj_create(NULL);

    lv_obj_t * btn = lv_btn_create(main_scr);
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "in view");

    lv_scr_load(main_scr);
}

// /**
//  * Extending the current theme
//  */
// void lv_example_style_14(void)
// {
//     lv_obj_t * btn;
//     lv_obj_t * label;

//     btn = lv_btn_create(lv_scr_act());
//     lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 20);

//     label = lv_label_create(btn);
//     lv_label_set_text(label, "Original theme");

//     new_theme_init_and_set();

//     btn = lv_btn_create(lv_scr_act());
//     lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

//     label = lv_label_create(btn);
//     lv_label_set_text(label, "New theme");
//     // lv_obj_t * img;
//     // img = lv_gif_create(lv_scr_act());
//     // /* Assuming a File system is attached to letter 'A'
//     // * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
//     // lv_gif_set_src(img, "A:/sdcard/miho_150_bad.gif");
//     // lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

// }
