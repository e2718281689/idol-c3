

#pragma once

#include "driver/i2c.h"
   

esp_err_t sht4x_read(float *temperature, float *humidity);
void i2c_master_init();