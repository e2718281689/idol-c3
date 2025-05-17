/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


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
#include "esp_littlefs.h"

#include "unity.h"

/* LCD size */
#define EXAMPLE_LCD_H_RES   (240)
#define EXAMPLE_LCD_V_RES   (240)

/* LCD settings */
#define EXAMPLE_LCD_SPI_NUM         (SPI2_HOST)
#define EXAMPLE_LCD_PIXEL_CLK_HZ    (40 * 1000 * 1000)
#define EXAMPLE_LCD_CMD_BITS        (8)
#define EXAMPLE_LCD_PARAM_BITS      (8)
#define EXAMPLE_LCD_COLOR_SPACE     (ESP_LCD_COLOR_SPACE_RGB)
#define EXAMPLE_LCD_BITS_PER_PIXEL  (16)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (40)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_SCLK       (GPIO_NUM_7)
#define EXAMPLE_LCD_GPIO_MOSI       (GPIO_NUM_6)
#define EXAMPLE_LCD_GPIO_RST        (-1)
#define EXAMPLE_LCD_GPIO_DC         (GPIO_NUM_4)
#define EXAMPLE_LCD_GPIO_CS         (GPIO_NUM_3)


static char *TAG = "test";

/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_fs_drv_t *disp_driver = (lv_fs_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SCLK,
        .mosi_io_num = EXAMPLE_LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        // .on_color_trans_done = example_notify_lvgl_flush_ready,
        // .user_ctx = &lvgl_disp,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_GPIO_RST,
        .color_space = EXAMPLE_LCD_COLOR_SPACE,
        .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789v3(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);


    return ret;

err:
    if (lcd_panel) {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io) {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}

static esp_err_t app_lcd_deinit(void)
{
    ESP_RETURN_ON_ERROR(esp_lcd_panel_del(lcd_panel), TAG, "LCD panel deinit failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_del(lcd_io), TAG, "LCD IO deinit failed");
    ESP_RETURN_ON_ERROR(spi_bus_free(EXAMPLE_LCD_SPI_NUM), TAG, "SPI BUS free failed");
    return ESP_OK;
}

static esp_err_t app_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,         /* LVGL task priority */
        .task_stack = 4096,         /* LVGL task stack size */
        .task_affinity = -1,        /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5        /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
        }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    esp_lcd_panel_set_gap(lcd_panel, 0, 80); 

    return ESP_OK;
}

static esp_err_t app_lvgl_deinit(void)
{

    ESP_RETURN_ON_ERROR(lvgl_port_remove_disp(lvgl_disp), TAG, "LVGL disp removing failed");
    ESP_RETURN_ON_ERROR(lvgl_port_deinit(), TAG, "LVGL deinit failed");

    return ESP_OK;
}

void *read_file_to_ram(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Failed to open file: %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    void *buf = malloc(size);
    if (!buf) {
        printf("Failed to allocate memory (%zu bytes)\n", size);
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(buf, 1, size, f);
    fclose(f);

    if (read_size != size) {
        printf("Read size mismatch: expected %zu, got %zu\n", size, read_size);
        free(buf);
        return NULL;
    }

    if (out_size) *out_size = size;
    return buf;
}

static void app_main_display(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* Task lock */
    lvgl_port_lock(0);

    /* Your LVGL objects code here .... */

    // 设置背景颜色为黄色（RGB: 255, 255, 0）
    lv_obj_set_style_bg_color(scr, lv_color_make(255, 255, 0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Task unlock */
    lvgl_port_unlock();
}

// Some resources are lazy allocated in the LCD driver, the threadhold is left for that case
#define TEST_MEMORY_LEAK_THRESHOLD (50)

static void check_leak(size_t start_free, size_t end_free, const char *type)
{
    ssize_t delta = start_free - end_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, start_free, end_free, delta);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE (delta, TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void lvgl_test()
{
    size_t start_freemem_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t start_freemem_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);

    ESP_LOGI(TAG, "Initilize LCD.");

    /* LCD HW initialization */
    TEST_ASSERT_EQUAL(app_lcd_init(), ESP_OK);

    size_t start_lvgl_freemem_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t start_lvgl_freemem_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);

    ESP_LOGI(TAG, "Initilize LVGL.");

    /* LVGL initialization */
    TEST_ASSERT_EQUAL(app_lvgl_init(), ESP_OK);

    /* Show LVGL objects */
    app_main_display();

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    /* LVGL deinit */
    TEST_ASSERT_EQUAL(app_lvgl_deinit(), ESP_OK);

    /* When using LVGL8, it takes some time to release all memory */
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "LVGL deinitialized.");

    size_t end_lvgl_freemem_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t end_lvgl_freemem_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(start_lvgl_freemem_8bit, end_lvgl_freemem_8bit, "8BIT LVGL");
    check_leak(start_lvgl_freemem_32bit, end_lvgl_freemem_32bit, "32BIT LVGL");

    /* LCD deinit */
    TEST_ASSERT_EQUAL(app_lcd_deinit(), ESP_OK);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "LCD deinitilized.");

    size_t end_freemem_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t end_freemem_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(start_freemem_8bit, end_freemem_8bit, "8BIT");
    check_leak(start_freemem_32bit, end_freemem_32bit, "32BIT");


}

void lvgl_main()
{

    TEST_ASSERT_EQUAL(app_lcd_init(), ESP_OK);

    TEST_ASSERT_EQUAL(app_lvgl_init(), ESP_OK);

    app_main_display();
}

void app_main(void)
{

    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    printf("TEST ESP LVGL port\n\r");

    lvgl_main();

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }

}
