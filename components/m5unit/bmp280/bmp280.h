// https://github.com/Seeed-Studio/Grove_BMP280/blob/master/Seeed_BMP280.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "i2c_device.h"

esp_err_t Bmp280_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud);
float Bmp280_getTemperature(void);
float Bmp280_getPressure(void);
float Bmp280_calcAltitude1(float p0);
float Bmp280_calcAltitude2(float p0, float p1, float t);

#ifdef __cplusplus
}
#endif
