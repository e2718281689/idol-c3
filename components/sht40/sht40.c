#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA 20
#define I2C_SCL 21
#define SHT4X_ADDR 0x44
#define CMD_MEASURE_HIGH_PRECISION 0xFD

void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

esp_err_t sht4x_read(float *temperature, float *humidity) {
    uint8_t cmd = CMD_MEASURE_HIGH_PRECISION;
    uint8_t rx_data[6];

    // 发送测量命令
    esp_err_t ret = i2c_master_write_to_device(I2C_PORT, SHT4X_ADDR, &cmd, 1, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    // 等待 10ms，确保传感器完成测量
    vTaskDelay(pdMS_TO_TICKS(10));

    // 读取6字节数据
    ret = i2c_master_read_from_device(I2C_PORT, SHT4X_ADDR, rx_data, 6, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;

    uint16_t t_ticks = (rx_data[0] << 8) | rx_data[1];
    // uint8_t checksum_t = rx_data[2]; // 可用于CRC校验

    uint16_t rh_ticks = (rx_data[3] << 8) | rx_data[4];
    // uint8_t checksum_rh = rx_data[5]; // 可用于CRC校验

    float t_degC = -45 + 175 * ((float)t_ticks / 65535.0f);
    float rh_pRH = -6 + 125 * ((float)rh_ticks / 65535.0f);

    // 限制在 0~100% 范围
    if (rh_pRH > 100) rh_pRH = 100;
    if (rh_pRH < 0) rh_pRH = 0;

    // printf("Temperature: %.2f °C, Humidity: %.2f %%\n", t_degC, rh_pRH);
    *temperature = t_degC;
    *humidity = rh_pRH;

    return ESP_OK;
}
