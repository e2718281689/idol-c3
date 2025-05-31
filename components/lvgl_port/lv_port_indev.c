#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "LVGL_ADC_BTN";

#define ADC_BTN_CHANNEL ADC_CHANNEL_2
#define ADC_UNIT        ADC_UNIT_1
#define ADC_ATTEN       ADC_ATTEN_DB_11

#define ADC_BTN_PREV_THRESH   3048     //RIGHT  // 电压阈值范围示意
#define ADC_BTN_NEXT_THRESH   10 
#define ADC_BTN_ENTER_THRESH  2280
#define ADC_BTN_MARGIN        100

typedef struct {
    adc_oneshot_unit_handle_t adc_handle;
    lv_indev_t *indev;
    uint32_t last_key;
    bool btn_pressed;
} lvgl_adc_btn_ctx_t;

static void lvgl_adc_btn_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data);

static lv_indev_t *lvgl_port_add_adc_buttons(void);

esp_err_t lvgl_indev_init()
{

    esp_err_t ret = ESP_OK;

    lv_indev_t* buttons_handle = lvgl_port_add_adc_buttons();

    if(buttons_handle == NULL)
    {
        return ESP_FAIL;
    }

    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(buttons_handle,g);

    return ret;
}


lv_indev_t *lvgl_port_add_adc_buttons(void)
{
    ESP_LOGI(TAG, "lvgl_port_add_adc_buttons");

    esp_err_t ret;
    lvgl_adc_btn_ctx_t *ctx = calloc(1, sizeof(lvgl_adc_btn_ctx_t));
    ESP_GOTO_ON_FALSE(ctx, false, err, TAG, "No memory for ADC button context");

    // 初始化 ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ret = adc_oneshot_new_unit(&init_config, &ctx->adc_handle);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "ADC unit init failed");

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(ctx->adc_handle, ADC_BTN_CHANNEL, &chan_cfg);
    ESP_GOTO_ON_ERROR(ret, err, TAG, "ADC channel config failed");

    // 注册 LVGL 输入设备
    lvgl_port_lock(0);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    // lv_indev_set_mode(indev, LV_INDEV_MODE_BUTTON);
    lv_indev_set_read_cb(indev, lvgl_adc_btn_read_cb);
    lv_indev_set_driver_data(indev, ctx);
    ctx->indev = indev;
    lvgl_port_unlock();

    return indev;

err:
    if (ctx) free(ctx);
    return NULL;
}

static void lvgl_adc_btn_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    lvgl_adc_btn_ctx_t *ctx = lv_indev_get_driver_data(indev_drv);
    int val = 0;
    adc_oneshot_read(ctx->adc_handle, ADC_BTN_CHANNEL, &val);
    
    data->state = LV_INDEV_STATE_RELEASED;

    if (abs(val - ADC_BTN_PREV_THRESH) < ADC_BTN_MARGIN) {
        data->key = LV_KEY_LEFT;
        ctx->last_key = LV_KEY_LEFT;
        data->state = LV_INDEV_STATE_PRESSED;
        // ESP_LOGI(TAG, "LV_KEY_LEFT adc value: %d", val);
    } else if (abs(val - ADC_BTN_NEXT_THRESH) < ADC_BTN_MARGIN) {
        data->key = LV_KEY_RIGHT;
        ctx->last_key = LV_KEY_RIGHT;
        data->state = LV_INDEV_STATE_PRESSED;
        // ESP_LOGI(TAG, "LV_KEY_RIGHT adc value: %d", val);
    } else if (abs(val - ADC_BTN_ENTER_THRESH) < ADC_BTN_MARGIN) {
        data->key = LV_KEY_ENTER;
        ctx->last_key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
        // ESP_LOGI(TAG, "LV_KEY_ENTER adc value: %d", val);
    } else {
        data->key = ctx->last_key;
    }
}
