#include "freertos/FreeRTOS.h"
#include "adt7410.h"

#define ADT7410_ADDR (0x48)

#define ADT7410_REG_TEMPDATA        (0x00)
#define ADT7410_REG_CONFIG          (0x03)
#define ADT7410_CONF_READ_13BIT        (0x00)
#define ADT7410_CONF_READ_16BIT        (0x00)

static const char *TAG = "ADT7410";

static I2CDevice_t adt7410_device = NULL;

float Adt7410_getTemperature_13bit(void) {
    esp_err_t ret = ESP_OK;
    if (adt7410_device == NULL) {
        return 0;
    }

    uint8_t data = ADT7410_CONF_READ_13BIT;
    ret = i2c_write_bytes(adt7410_device, (uint32_t)ADT7410_REG_CONFIG, &(data), (uint16_t)1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Adt7410_getTemperature_13bit write error!");
        return 0;
    }
    vTaskDelay( pdMS_TO_TICKS(100) );

    uint8_t tempdata[2] = {0};
    uint16_t tempvalue = 0;
    int32_t inttempvalue = 0;
    ret = i2c_read_bytes(adt7410_device, (uint32_t)ADT7410_REG_TEMPDATA, tempdata, (uint16_t)2);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Adt7410_getTemperature_13bit read error!");
        return 0;
    }

    tempvalue = (uint16_t)( tempdata[0] << 8 ) | ( tempdata[1] );
    tempvalue = tempvalue >> 3;

    if (tempvalue & 0x1000) {
        inttempvalue = tempvalue - 0x2000;
    } else {
        inttempvalue = tempvalue;
    }

    return inttempvalue / 16.0;
}

esp_err_t Adt7410_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud) {
    ESP_LOGI(TAG, "Adt7410_Init Init()");
    esp_err_t ret = ESP_OK;
    adt7410_device = i2c_malloc_device(i2c_num, sda, scl, baud, ADT7410_ADDR);
    if (adt7410_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    return ret;
}
