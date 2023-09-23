// https://github.com/Seeed-Studio/Seeed_Arduino_BME68x
#include "freertos/FreeRTOS.h"
#include "esp32_bme680.h"

#define BME680_ADDR (0x77)

typedef struct Result {
    float temperature;
    float pressure;
    float humidity;
    float gas;
} sensor_result_t;

static const char *TAG = "BME680";

static I2CDevice_t bme680_device = NULL;
static sensor_result_t sensor_result_value;
struct bme680_dev sensor_param; // Official LIB structure.


static void delay_msec(uint32_t ms) {
    vTaskDelay( pdMS_TO_TICKS(ms) );
}

int8_t iic_read(uint8_t dev_id, uint8_t reg_addr, uint8_t* reg_data, uint16_t len) {
    esp_err_t ret = ESP_OK;
    if (bme680_device == NULL) {
        return 1;
    }

    ret = i2c_read_bytes(bme680_device, (uint32_t)reg_addr, reg_data, (uint16_t)len);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "iic_read error!");
        return 1;
    }

    return 0;
}
int8_t iic_write(uint8_t dev_id, uint8_t reg_addr, uint8_t* reg_data, uint16_t len) {
    esp_err_t ret = ESP_OK;
    if (bme680_device == NULL) {
        return 1;
    }

    ret = i2c_write_bytes(bme680_device, (uint32_t)reg_addr, reg_data, (uint16_t)len);
    vTaskDelay( pdMS_TO_TICKS(100) );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "iic_write error!");
        return 1;
    }

    return 0;
}

float Esp32_Bme680_get_temperature(void) {
    return sensor_result_value.temperature;
}
float Esp32_Bme680_get_pressure(void) {
    return sensor_result_value.pressure;
}
float Esp32_Bme680_get_humidity(void) {
    return sensor_result_value.humidity;
}
float Esp32_Bme680_get_gas(void) {
    return sensor_result_value.gas;
}
int8_t Esp32_Bme680_read_sensor_data(void) {

    struct bme680_field_data data;

    int8_t ret;

    sensor_param.power_mode = BME680_FORCED_MODE;


    uint16_t settings_sel;

    sensor_param.tph_sett.os_hum = BME680_OS_1X;
    sensor_param.tph_sett.os_pres = BME680_OS_16X;
    sensor_param.tph_sett.os_temp = BME680_OS_2X;

    sensor_param.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
    sensor_param.gas_sett.heatr_dur = 100;
    sensor_param.gas_sett.heatr_temp = 300;

    settings_sel = BME680_OST_SEL | BME680_OSH_SEL | BME680_OSP_SEL | BME680_FILTER_SEL | BME680_GAS_SENSOR_SEL;

    /*Set sensor's registers*/
    if ((ret = bme680_set_sensor_settings(settings_sel, &sensor_param))) {
        ESP_LOGE(TAG, "bme680_set_sensor_settings() ==>ret value = %d", ret);
        return -1;
    }
    
    /*Set sensor's mode ,activate sensor*/
    if ((ret = bme680_set_sensor_mode(&sensor_param))) {
        ESP_LOGE(TAG, "bme680_set_sensor_mode() ==>ret value = %d", ret);
        return -2;
    }

    uint16_t meas_period;
    bme680_get_profile_dur(&meas_period, &sensor_param);

    vTaskDelay( pdMS_TO_TICKS(meas_period) ); /**<It is necessary to delay for a duration time */

    /*Get sensor's result value from registers*/
    if ((ret = bme680_get_sensor_data(&data, &sensor_param))) {
        ESP_LOGE(TAG, "bme680_get_sensor_data() ==>ret value = %d", ret);
        return -3;
    }

    sensor_result_value.temperature = data.temperature / 100.0;
    sensor_result_value.humidity = data.humidity / 1000.0;
    sensor_result_value.pressure = data.pressure;
    if (data.status & BME680_HEAT_STAB_MSK) {
        sensor_result_value.gas = data.gas_resistance;
    } else {
        sensor_result_value.gas = 0;
    }
    return BME680_OK;
}

esp_err_t Esp32_Bme680_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud) {
    ESP_LOGI(TAG, "BME680_Init Init()");
    esp_err_t ret = ESP_OK;
    bme680_device = i2c_malloc_device(i2c_num, sda, scl, baud, BME680_ADDR);
    if (bme680_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    int8_t bme680lib_ret = BME680_OK;
    sensor_param.intf = BME680_I2C_INTF;
    sensor_param.read = iic_read;
    sensor_param.write = iic_write;
    sensor_param.delay_ms = delay_msec;
    if ((bme680lib_ret = bme680_init(&sensor_param))) {
        ESP_LOGE(TAG, "bme680_init() ==>ret value = %d", bme680lib_ret);
        return ESP_ERR_NOT_FOUND;
    }

    return ret;
}
