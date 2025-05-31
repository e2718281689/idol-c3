#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7789v3.h"
#include "include/lv_port_tick.h"
#include "include/lv_port_disp.h"
#include "include/lv_port_indev.h"

static char *TAG = "lv_port_tick";


// Some resources are lazy allocated in the LCD driver, the threadhold is left for that case
#define TEST_MEMORY_LEAK_THRESHOLD (512)

static void check_leak(size_t start_free, size_t end_free, const char *type)
{
    size_t delta = start_free - end_free;
    printf("MALLOC_CAP_%s: Before %.2f KB free, After %.2f KB free (delta %.2f KB)\n",
           type,
           start_free / 1024.0,
           end_free / 1024.0,
           delta / 1024.0);
    
    // TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(delta, TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}



static lv_style_t style_btn;
static lv_style_t style_button_pressed;
static lv_style_t style_button_red;

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Toggled");
    }
}

void lv_example_button_1(void)
{
    lv_obj_t * label;

    // 获取当前活动屏幕对象
    lv_obj_t *scr = lv_scr_act();

    // 设置背景色为白色
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    
    lv_obj_t * btn1 = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
    lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t * btn2 = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Toggle");
    lv_obj_center(label);

}


void lvgl_task(void *pvParameters)
{


    
    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());
    ESP_ERROR_CHECK(lvgl_indev_init());

    lv_example_button_1();

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));  // 推荐 5~20ms
    }
}

void freemem_task(void *pvParameters)
{

    while (1) {

        size_t start_freemem_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        size_t start_freemem_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);

        vTaskDelay(pdMS_TO_TICKS(10 * 1000)); 

        size_t end_freemem_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        size_t end_freemem_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
        check_leak(start_freemem_8bit, end_freemem_8bit, "8BIT LVGL");
        check_leak(start_freemem_32bit, end_freemem_32bit, "32BIT LVGL");
    }
}