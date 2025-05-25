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
#include  "include/lv_port_tick.h"
#include  "include/lv_port_disp.h"
#include "unity.h"

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



void lvgl_task(void *pvParameters)
{

    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());

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