// https://github.com/Seeed-Studio/Grove_BMP280/blob/master/Seeed_BMP280.cpp
#include "freertos/FreeRTOS.h"
#include "math.h"
#include "bmp280.h"

#define BMP280_ADDR (0x76)

#define BMP280_REG_DIG_T1    (0x88)
#define BMP280_REG_DIG_T2    (0x8A)
#define BMP280_REG_DIG_T3    (0x8C)

#define BMP280_REG_DIG_P1    (0x8E)
#define BMP280_REG_DIG_P2    (0x90)
#define BMP280_REG_DIG_P3    (0x92)
#define BMP280_REG_DIG_P4    (0x94)
#define BMP280_REG_DIG_P5    (0x96)
#define BMP280_REG_DIG_P6    (0x98)
#define BMP280_REG_DIG_P7    (0x9A)
#define BMP280_REG_DIG_P8    (0x9C)
#define BMP280_REG_DIG_P9    (0x9E)

#define BMP280_REG_CHIPID          (0xD0)
#define BMP280_REG_VERSION         (0xD1)
#define BMP280_REG_SOFTRESET       (0xE0)

#define BMP280_REG_CONTROL         (0xF4)
#define BMP280_REG_CONFIG          (0xF5)
#define BMP280_REG_PRESSUREDATA    (0xF7)
#define BMP280_REG_TEMPDATA        (0xFA)

#define BMP280_CHIPID (0x58)

typedef struct _bmp280_cali_data {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    int32_t t_fine;
} bmp280_cali_data_t;

typedef struct _bmp280_data {
    uint8_t chipid;
    float temperature;
    float pressure;
    float altitude;
    bmp280_cali_data_t bmp280_cali;
} bmp280_data_t;

static const char *TAG = "BMP280";

static I2CDevice_t bmp280_device = NULL;
static bmp280_data_t bmp280;


uint8_t Bmp280_bmp280Read8(uint8_t reg) {
    esp_err_t ret = ESP_OK;
    uint8_t data = 0;
    if (bmp280_device == NULL) {
        return 0;
    }

    ret = i2c_read_bytes(bmp280_device, (uint32_t)reg, &(data), (uint16_t)1);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BMP280_bmp280Read8 read error!");
        return 0;
    }
    return data;
}

uint16_t Bmp280_bmp280Read16(uint8_t reg) {
    esp_err_t ret = ESP_OK;
    uint8_t data[2] = {0};
    if (bmp280_device == NULL) {
        return 0;
    }

    ret = i2c_read_bytes(bmp280_device, (uint32_t)reg, data, (uint16_t)2);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BMP280_bmp280Read16 read error!");
        return 0;
    }
    return (uint16_t) data[0] << 8 | data[1];
}

uint16_t Bmp280_bmp280Read16LE(uint8_t reg) {
    esp_err_t ret = ESP_OK;
    uint8_t data[2] = {0};
    if (bmp280_device == NULL) {
        return 0;
    }

    ret = i2c_read_bytes(bmp280_device, (uint32_t)reg, data, (uint16_t)2);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BMP280_bmp280Read16 read error!");
        return 0;
    }
    return (uint16_t) data[1] << 8 | data[0];
}

uint32_t Bmp280_bmp280Read24(uint8_t reg) {
    esp_err_t ret = ESP_OK;
    uint8_t data[3] = {0};
    if (bmp280_device == NULL) {
        return 0;
    }

    ret = i2c_read_bytes(bmp280_device, (uint32_t)reg, data, (uint16_t)3);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BMP280_bmp280Read24 read error!");
        return 0;
    }
    return (uint32_t) data[0] << 16 | data[1] << 8 | data[2];
}

esp_err_t Bmp280_writeRegister(uint8_t reg, uint8_t val) {
    esp_err_t ret = ESP_OK;
    if (bmp280_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t data = val;
    i2c_write_bytes(bmp280_device, (uint32_t)reg, &(data), (uint16_t)1);
    ESP_LOGI(TAG, "BMP280_writeRegister 0x%x", data);
    vTaskDelay( pdMS_TO_TICKS(100) );
    return ret;
}

float Bmp280_getTemperature(void) {
  int32_t var1, var2;
  int32_t adc_T = Bmp280_bmp280Read24(BMP280_REG_TEMPDATA);

  adc_T >>= 4;
  var1 = (((adc_T >> 3) - ((int32_t)(bmp280.bmp280_cali.dig_T1 << 1))) *
          ((int32_t)bmp280.bmp280_cali.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)bmp280.bmp280_cali.dig_T1)) *
            ((adc_T >> 4) - ((int32_t)bmp280.bmp280_cali.dig_T1))) >> 12) *
          ((int32_t)bmp280.bmp280_cali.dig_T3)) >> 14;
  bmp280.bmp280_cali.t_fine = var1 + var2;
  float T = (bmp280.bmp280_cali.t_fine * 5 + 128) >> 8;
  return T / 100;
}

float Bmp280_getPressure(void) {
  int64_t var1, var2, p;
  // Call getTemperature to get t_fine
  Bmp280_getTemperature();

  int32_t adc_P = Bmp280_bmp280Read24(BMP280_REG_PRESSUREDATA);
  adc_P >>= 4;
  var1 = ((int64_t)bmp280.bmp280_cali.t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)bmp280.bmp280_cali.dig_P6;
  var2 = var2 + ((var1 * (int64_t)bmp280.bmp280_cali.dig_P5) << 17);
  var2 = var2 + (((int64_t)bmp280.bmp280_cali.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)bmp280.bmp280_cali.dig_P3) >> 8) + ((var1 * (int64_t)bmp280.bmp280_cali.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280.bmp280_cali.dig_P1) >> 33;
  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)bmp280.bmp280_cali.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)bmp280.bmp280_cali.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280.bmp280_cali.dig_P7) << 4);
  return (float)p / 256;
}

float Bmp280_calcAltitude2(float p0, float p1, float t) {
  float C;
  C = (p0 / p1);
  C = pow(C, (1 / 5.25588)) - 1.0;
  C = (C * (t + 273.15)) / 0.0065;
  return C;
}

float Bmp280_calcAltitude1(float p0) {
  float t = Bmp280_getTemperature();
  float p1 = Bmp280_getPressure();
  return Bmp280_calcAltitude2(p0, p1, t);
}

void Bmp280_GetCalibrationData() {
    bmp280.bmp280_cali.dig_T1 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_T1);
    bmp280.bmp280_cali.dig_T2 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_T2);
    bmp280.bmp280_cali.dig_T3 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_T3);
    bmp280.bmp280_cali.dig_P1 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P1);
    bmp280.bmp280_cali.dig_P2 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P2);
    bmp280.bmp280_cali.dig_P3 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P3);
    bmp280.bmp280_cali.dig_P4 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P4);
    bmp280.bmp280_cali.dig_P5 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P5);
    bmp280.bmp280_cali.dig_P6 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P6);
    bmp280.bmp280_cali.dig_P7 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P7);
    bmp280.bmp280_cali.dig_P8 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P8);
    bmp280.bmp280_cali.dig_P9 = Bmp280_bmp280Read16LE(BMP280_REG_DIG_P9);
    Bmp280_writeRegister(BMP280_REG_CONTROL, 0x3F);

}

esp_err_t Bmp280_GetChipID() {
    esp_err_t ret = ESP_OK;
    if (bmp280_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    ret = i2c_read_bytes(bmp280_device, (uint32_t)BMP280_REG_CHIPID, &(bmp280.chipid), (uint16_t)1);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret == ESP_OK) {
        if (bmp280.chipid != BMP280_CHIPID) {
            ESP_LOGE(TAG, "BMP280_GetChipID not error.");
            return ESP_ERR_INVALID_VERSION;
        }
    }
    ESP_LOGI(TAG, "BMP280_GetChipID %02X", bmp280.chipid);
    return ret;
}

esp_err_t Bmp280_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud) {
    ESP_LOGI(TAG, "BMP280_Init Init()");
    esp_err_t ret = ESP_OK;
    bmp280_device = i2c_malloc_device(i2c_num, sda, scl, baud, BMP280_ADDR);
    if (bmp280_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    ret = Bmp280_GetChipID();
    Bmp280_GetCalibrationData();

    return ret;
}
