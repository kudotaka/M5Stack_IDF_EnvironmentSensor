#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "i2c_device.h"
#include "adt7410.h"

esp_err_t Adt7410_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud);
float Adt7410_getTemperature_13bit(void);

#ifdef __cplusplus
}
#endif
