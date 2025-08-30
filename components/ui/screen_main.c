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

/**
 * @brief 在当前屏幕的中央显示一张指定路径的图片。
 *
 * @param image_path 图片文件的完整路径字符串 (例如 "A:/littlefs/my_image.bin")
 * @return lv_obj_t* 返回创建的图片对象的指针，如果需要后续操作（如移动、删除）可以使用它。
 */
lv_obj_t * wifi_view_show_image(const char * image_path)
{
    // 1. 获取当前活动的屏幕作为父对象
    lv_obj_t * parent = lv_scr_act();
    if (!parent) {
        // 如果没有活动的屏幕，则无法创建图片
        return NULL;
    }

    // 2. 创建一个新的图片对象，父对象是当前屏幕
    lv_obj_t * img = lv_img_create(parent);

    // 3. 设置图片的源文件路径
    // LVGL 会自动通过你注册的文件系统驱动来读取这个文件
    lv_img_set_src(img, image_path);

    // 4. 将图片放置在屏幕中央 (这是一个很好的默认行为)
    lv_obj_center(img);
    
    // (可选) 如果图片需要交互，可以开启点击事件
    // lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);

    // 5. 返回创建的图片对象，方便调用者后续控制
    return img;
}

void wifi_view_update_status(const char * msg)
{
    if (!label_task) {
        label_task = lv_label_create(lv_scr_act());
        lv_obj_align(label_task, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
    lv_label_set_text(label_task, msg);
}

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

// 按钮事件：进入任务界面
static void image_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) 
    {


        task_scr = lv_obj_create(NULL);

        label_task = lv_label_create(task_scr);
        lv_obj_center(label_task);
        lv_label_set_text(label_task, "ready ...");

        lv_scr_load(task_scr);

        download_file_task_prov();
    }
}



void create_main_screen(void)
{
    main_scr = lv_obj_create(NULL);

    // --- 新增代码：创建并设置背景图片 ---

    // 1. 创建一个图像对象，它的父对象是我们的主屏幕
    lv_obj_t * background_img = lv_img_create(main_scr);

    // 2. 设置图像的来源。
    //    这里的路径 "A:/images/background.bin" 依赖于您配置好的 lv_port_fs.c
    //    'A:' 会被映射到 TF 卡。请确保 TF 卡的根目录下有一个 "images" 文件夹，
    //    并且里面存放了名为 "background.bin" 的图片文件。
    lv_img_set_src(background_img, "A:/sdcard/Chie_240.bin");

    // 3. 将图片在屏幕上居中显示
    lv_obj_align(background_img, LV_ALIGN_CENTER, 0, 0);
    
    // (可选) 如果图片很大，覆盖了整个屏幕，建议添加此标志。
    // 这会让图片对象“不可点击”，使得触摸事件可以“穿透”到它后面的屏幕或按钮上。
    lv_obj_add_flag(background_img, LV_OBJ_FLAG_ADV_HITTEST);


    // --- 原有的代码保持不变 ---

    // 创建第一个按钮 (它将被绘制在背景图片之上)
    lv_obj_t * btn = lv_btn_create(main_scr);
    // 按钮在屏幕上方居中，向上偏移 1/4 屏幕高度
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, - (lv_disp_get_ver_res(NULL) / 4));
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "wifi bt net ");
    lv_obj_center(label);

    // 创建第二个按钮 (它也将被绘制在背景图片之上)
    lv_obj_t * btn2 = lv_btn_create(main_scr);
    // 按钮在屏幕下方居中，向下偏移 1/4 屏幕高度
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, (lv_disp_get_ver_res(NULL) / 4));
    lv_obj_add_event_cb(btn2, image_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "show image");
    lv_obj_center(label2);

    lv_scr_load(main_scr);
}
