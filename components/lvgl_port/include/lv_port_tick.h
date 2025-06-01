#pragma once

void lvgl_task(void *pvParameters);

void i2c_master_init(void);
esp_err_t sht4x_read(float *temperature, float *humidity);