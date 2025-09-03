

#ifndef SHT_40_H
#define SHT_40_H


#include "driver/i2c.h"
   

esp_err_t sht4x_read(float *temperature, float *humidity);
void i2c_master_init();

#endif /*SHT_40_H*/