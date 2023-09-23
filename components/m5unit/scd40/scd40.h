// https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library/blob/main/src/SparkFun_SCD4x_Arduino_Library.h
// https://github.com/Seeed-Studio/Seeed_SCD30/blob/master/SCD30.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "i2c_device.h"

esp_err_t Scd40_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud);

bool Scd40_IsAvailable(void);

esp_err_t Scd40_SetAutoSelfCalibration(bool enable);

esp_err_t Scd40_StartPeriodicMeasurement(void);
esp_err_t Scd40_StopPeriodicMeasurement(void);
esp_err_t Scd40_SetTemperatureOffset(float offset);

esp_err_t Scd40_ReadMeasurement(float* result);

#ifdef __cplusplus
}
#endif