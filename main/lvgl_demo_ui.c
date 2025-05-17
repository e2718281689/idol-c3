/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/widgets/extra/meter.html#simple-meter

#include "lvgl.h"
// #include "moiw_2014.c"

// LV_IMG_DECLARE(moiw_2014)


void example_lvgl_demo_ui(lv_disp_t *disp)
{

    // // 获取当前屏幕
    // lv_obj_t *scr = lv_disp_get_scr_act(disp);

    // // 创建图像对象
    // lv_obj_t *img = lv_img_create(scr);
    // lv_img_set_src(img, "A:/littlefs/moiw_2014.bin"); // 使用LittleFS中的图像文件
    // // lv_img_set_src(img, &moiw_2014); // 使用LittleFS中的图像文件

    // // 设置图像中心对齐
    // lv_obj_center(img);

    // // 允许旋转和缩放（开启变换属性）
    // lv_img_set_pivot(img, 100 ,100); // 设定旋转中心
    // lv_img_set_angle(img, 0); // 初始角度：0°

    // // 创建动画旋转图像
    // lv_anim_t a;
    // lv_anim_init(&a);
    // lv_anim_set_var(&a, img);
    // lv_anim_set_values(&a, 0, 3600); // 单位是0.1°，3600 = 360°
    // lv_anim_set_time(&a, 3000); // 3秒一圈
    // lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    // lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_angle);
    // lv_anim_start(&a);


    //CF_TRUE_COLOR_ALPHA Binary RGB565 Swap //png

}
