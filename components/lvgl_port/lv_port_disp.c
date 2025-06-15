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
#include "lv_port_disp.h"

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
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (5)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_SCLK       (GPIO_NUM_7)
#define EXAMPLE_LCD_GPIO_MOSI       (GPIO_NUM_6)
#define EXAMPLE_LCD_GPIO_RST        (-1)
#define EXAMPLE_LCD_GPIO_DC         (GPIO_NUM_4)
#define EXAMPLE_LCD_GPIO_CS         (GPIO_NUM_3)


static char *TAG = "lv_port_disp";

/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;

/* LVGL display and touch */
lv_display_t *lvgl_disp = NULL;


esp_err_t app_lcd_init(void)

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

esp_err_t app_lcd_deinit(void)
{
    ESP_RETURN_ON_ERROR(esp_lcd_panel_del(lcd_panel), TAG, "LCD panel deinit failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_del(lcd_io), TAG, "LCD IO deinit failed");
    ESP_RETURN_ON_ERROR(spi_bus_free(EXAMPLE_LCD_SPI_NUM), TAG, "SPI BUS free failed");
    return ESP_OK;
}

esp_err_t app_lvgl_init(void)
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

esp_err_t app_lvgl_deinit(void)
{

    ESP_RETURN_ON_ERROR(lvgl_port_remove_disp(lvgl_disp), TAG, "LVGL disp removing failed");
    ESP_RETURN_ON_ERROR(lvgl_port_deinit(), TAG, "LVGL deinit failed");

    return ESP_OK;
}


// // 动画回调：设置角度
// static void rotate_cb(void *img_obj, int32_t angle)
// {
//     lv_image_set_rotation(img_obj, angle);
// }
// static void app_main_display(void)
// {
//     lv_obj_t *scr = lv_scr_act();

//     /* Task lock */
//     lvgl_port_lock(0);

//     /* Your LVGL objects code here .... */

//     // 设置背景颜色为黄色（RGB: 255, 255, 0）
//     lv_obj_set_style_bg_color(scr, lv_color_make(255, 255, 0), LV_PART_MAIN);
//     lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);


//     lv_obj_t* img = lv_gif_create(lv_screen_active());
//     lv_gif_set_src(img, "A:/littlefs/miho.gif");

//     /*Now create the actual image*/
//     // lv_obj_t *img = lv_image_create(lv_screen_active());
//     // lv_image_set_src(img, "A:/littlefs/moiw_2014_240.bin");
//     // lv_image_set_src(img, &moiw_2014);
   
//     lv_obj_center(img);

//     // lv_obj_set_style_transform_pivot_x(img, LV_PCT(50), 0);
//     // lv_obj_set_style_transform_pivot_y(img, LV_PCT(50), 0);

//     // // 动画配置
//     // lv_anim_t a;
//     // lv_anim_init(&a);
//     // lv_anim_set_var(&a, img);
//     // lv_anim_set_exec_cb(&a, rotate_cb);  // 用 C 函数作为回调
//     // lv_anim_set_values(&a, 0, 3600);
//     // lv_anim_set_time(&a, 3000);
//     // lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
//     // lv_anim_start(&a);

//     // /* Task unlock */
//     lvgl_port_unlock();
// }

