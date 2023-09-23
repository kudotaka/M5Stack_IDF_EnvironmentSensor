// https://github.com/m5stack/M5Unit-ENV/blob/master/src/QMP6988.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "i2c_device.h"

esp_err_t Qmp6988_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud);
esp_err_t Qmp6988_GetChipID();
float Qmp6988_CalcPressure();
float Qmp6988_calcTemperature();

#ifdef __cplusplus
}
#endif
