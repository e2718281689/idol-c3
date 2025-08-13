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
#include <dirent.h>
static char *TAG = "lv_port_tick";

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

SemaphoreHandle_t spi_mutex;

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

/*Will be called when the styles of the base theme are already added
  to add new styles*/
static void new_theme_apply_cb(lv_theme_t * th, lv_obj_t * obj)
{
    LV_UNUSED(th);

    if(lv_obj_check_type(obj, &lv_btn_class)) {
        lv_obj_add_style(obj, &style_btn, 0);
    }
}

static void new_theme_init_and_set(void)
{
    /*Initialize the styles*/
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_border_color(&style_btn, lv_palette_darken(LV_PALETTE_GREEN, 3));
    lv_style_set_border_width(&style_btn, 3);

    /*Initialize the new theme from the current theme*/
    lv_theme_t * th_act = lv_disp_get_theme(NULL);
    static lv_theme_t th_new;
    th_new = *th_act;

    /*Set the parent theme and the style apply callback for the new theme*/
    lv_theme_set_parent(&th_new, th_act);
    lv_theme_set_apply_cb(&th_new, new_theme_apply_cb);

    /*Assign the new theme to the current display*/
    lv_disp_set_theme(NULL, &th_new);
}

void list_files_in_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Error opening directory: %s\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // readdir 会返回 "." 和 ".."，通常我们希望忽略它们
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("Found file: %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

/**
 * Extending the current theme
 */
void lv_example_style_14(void)
{
    // lv_obj_t * btn;
    // lv_obj_t * label;

    // btn = lv_btn_create(lv_scr_act());
    // lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 20);

    // label = lv_label_create(btn);
    // lv_label_set_text(label, "Original theme");

    // new_theme_init_and_set();

    // btn = lv_btn_create(lv_scr_act());
    // lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    // label = lv_label_create(btn);
    // lv_label_set_text(label, "New theme");

    if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) 
    {
        lv_obj_t * img;
        img = lv_gif_create(lv_scr_act());
        /* Assuming a File system is attached to letter 'A'
        * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
        lv_gif_set_src(img, "A:/littlefs/miho_150_bad.gif");
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

        list_files_in_directory("/littlefs");

        xSemaphoreGive(spi_mutex);
    }

}




void lvgl_task(void *pvParameters)
{
    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
       // 错误处理：Mutex 创建失败
        ESP_LOGE("main", "Failed to create SPI mutex");
        return;
    }
    

    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());
    ESP_ERROR_CHECK(lvgl_indev_init());

    lv_example_style_14();

    while (1) {

        if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) 
        {
            uint32_t  time = lv_timer_handler();
            vTaskDelay(pdMS_TO_TICKS(time));  // 推荐 5~20ms
        }

        xSemaphoreGive(spi_mutex);
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