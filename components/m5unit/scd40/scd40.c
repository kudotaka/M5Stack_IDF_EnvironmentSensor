// https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library/blob/main/src/SparkFun_SCD4x_Arduino_Library.cpp
// https://github.com/Seeed-Studio/Seeed_SCD30/blob/master/SCD30.cpp
#include "freertos/FreeRTOS.h"
#include "scd40.h"
#include "string.h"

#define SCD40_I2C_ADDRESS (0x62)

//Available commands
//Basic Commands
#define SCD4x_COMMAND_START_PERIODIC_MEASUREMENT              (0x21b1)
#define SCD4x_COMMAND_READ_MEASUREMENT                        (0xec05) // execution time: 1ms
#define SCD4x_COMMAND_STOP_PERIODIC_MEASUREMENT               (0x3f86) // execution time: 500ms

//On-chip output signal compensation
#define SCD4x_COMMAND_SET_TEMPERATURE_OFFSET                  (0x241d) // execution time: 1ms
#define SCD4x_COMMAND_GET_TEMPERATURE_OFFSET                  (0x2318) // execution time: 1ms
#define SCD4x_COMMAND_SET_SENSOR_ALTITUDE                     (0x2427) // execution time: 1ms
#define SCD4x_COMMAND_GET_SENSOR_ALTITUDE                     (0x2322) // execution time: 1ms
#define SCD4x_COMMAND_SET_AMBIENT_PRESSURE                    (0xe000) // execution time: 1ms

//Field calibration
#define SCD4x_COMMAND_PERFORM_FORCED_CALIBRATION              (0x362f) // execution time: 400ms
#define SCD4x_COMMAND_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED  (0x2416) // execution time: 1ms
#define SCD4x_COMMAND_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED  (0x2313) // execution time: 1ms

//Low power
#define SCD4x_COMMAND_START_LOW_POWER_PERIODIC_MEASUREMENT    (0x21ac)
#define SCD4x_COMMAND_GET_DATA_READY_STATUS                   (0xe4b8) // execution time: 1ms

//Advanced features
#define SCD4x_COMMAND_PERSIST_SETTINGS                        (0x3615) // execution time: 800ms
#define SCD4x_COMMAND_GET_SERIAL_NUMBER                       (0x3682) // execution time: 1ms
#define SCD4x_COMMAND_PERFORM_SELF_TEST                       (0x3639) // execution time: 10000ms
#define SCD4x_COMMAND_PERFORM_FACTORY_RESET                   (0x3632) // execution time: 1200ms
#define SCD4x_COMMAND_REINIT                                  (0x3646) // execution time: 20ms

//Low power single shot - SCD41 only
#define SCD4x_COMMAND_MEASURE_SINGLE_SHOT                     (0x219d) // execution time: 5000ms
#define SCD4x_COMMAND_MEASURE_SINGLE_SHOT_RHT_ONLY            (0x2196) // execution time: 50ms

#define SCD40_POLYNOMIAL                        (0x31)

static I2CDevice_t scd40_device = NULL;

uint8_t Scd40_CalculateCrc(uint8_t data[], uint8_t len) {
    uint8_t bit, byteCtr, crc = 0xff;

    // calculates 8-Bit checksum with given polynomial
    for (byteCtr = 0; byteCtr < len; byteCtr ++) {
        crc ^= (data[byteCtr]);

        for (bit = 8; bit > 0; -- bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ SCD40_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

esp_err_t Scd40_ReadBuffer(uint8_t* data, uint8_t len) {
    esp_err_t ret = ESP_OK;
    ret = i2c_read_bytes(scd40_device, (uint32_t)I2C_NO_REG, data, (uint16_t)len);
    vTaskDelay( pdMS_TO_TICKS(200) );
    return ret;
}

esp_err_t Scd40_WriteBuffer(uint8_t* data, uint8_t len) {
    esp_err_t ret = ESP_OK;
    ret = i2c_write_bytes(scd40_device, (uint32_t)I2C_NO_REG, data, (uint16_t)len);
    vTaskDelay( pdMS_TO_TICKS(200) );
    return ret;
}

esp_err_t Scd40_WriteCommand(uint16_t command) {
    if (scd40_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    uint8_t buf[2] = { 0 };
    buf[0] = command >> 8;
    buf[1] = command & 0xff;
    return Scd40_WriteBuffer(buf, (uint8_t)2);
}

esp_err_t Scd40_WriteCommandWithArguments(uint16_t command, uint16_t arguments) {
    uint8_t checkSum, buf[5] = { 0 };
    uint8_t data[2] = { 0 };
    data[0] = arguments >> 8;
    data[1] = arguments & 0xff;
    checkSum = Scd40_CalculateCrc(data, (uint8_t)2);

    buf[0] = command >> 8;
    buf[1] = command & 0xff;
    buf[2] = arguments >> 8;
    buf[3] = arguments & 0xff;
    buf[4] = checkSum;

    return Scd40_WriteBuffer(buf, (uint8_t)5);
}

uint16_t Scd40_ReadRegister(uint16_t address) {
    esp_err_t ret = ESP_OK;
    uint8_t buf[2] = { 0 };

    ret = Scd40_WriteCommand(address);
    ret = Scd40_ReadBuffer(buf, 2);
    if (ret != ESP_OK) {
        return false;
    }

    return (uint16_t)((buf[0] << 8) | buf[1]);
}

esp_err_t Scd40_SetTemperatureOffset(float offset) {
    if (offset < 0.0 || offset >= 175.0) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t offsetWord = (uint16_t)(offset * 65536.0 / 175.0 + 0.5f);
    return Scd40_WriteCommandWithArguments(SCD4x_COMMAND_SET_TEMPERATURE_OFFSET, offsetWord);
}

bool Scd40_IsAvailable(void) {
    return Scd40_ReadRegister(SCD4x_COMMAND_GET_DATA_READY_STATUS);
}

esp_err_t Scd40_SetAutoSelfCalibration(bool enable) {
    if (enable) {
        return Scd40_WriteCommandWithArguments(SCD4x_COMMAND_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, (uint16_t)0x0001);    //Activate continuous ASC
    } else {
        return Scd40_WriteCommandWithArguments(SCD4x_COMMAND_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, (uint16_t)0x0000);    //Deactivate continuous ASC
    }
}

esp_err_t Scd40_StartPeriodicMeasurement(void) {
    return Scd40_WriteCommandWithArguments(SCD4x_COMMAND_START_PERIODIC_MEASUREMENT, (uint16_t)0x0000);
}

esp_err_t Scd40_StopPeriodicMeasurement(void) {
    return Scd40_WriteCommand(SCD4x_COMMAND_STOP_PERIODIC_MEASUREMENT);
}

esp_err_t Scd40_ReadMeasurement(float* result) {
    esp_err_t ret = ESP_OK;
    uint8_t buf[9] = { 0 };
    uint16_t co2U16 = 0;
    uint16_t tempU16 = 0;
    uint16_t humU16 = 0;
    float co2Concentration = 0;
    float temperature = 0;
    float humidity = 0;
    ret = Scd40_WriteCommand(SCD4x_COMMAND_READ_MEASUREMENT);
    if (ret != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }
    ret = Scd40_ReadBuffer(buf, 9);
    if (ret != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }

    co2U16 = (uint16_t)((((uint16_t)buf[0]) << 8) | ((uint16_t)buf[1]));
    tempU16 = (uint16_t)((((uint16_t)buf[3]) << 8) | ((uint16_t)buf[4]));
    humU16 = (uint16_t)((((uint16_t)buf[6]) << 8) | ((uint16_t)buf[7]));

    co2Concentration = (float)co2U16;
    temperature = (float)(tempU16 * 175.0 / 65535.0 - 45.0);
    humidity = (float)(humU16 * 100.0 / 65535.0);
    memcpy(&result[0], &co2Concentration, sizeof(co2Concentration));
    memcpy(&result[1], &temperature, sizeof(temperature));
    memcpy(&result[2], &humidity, sizeof(humidity));

    return ret;
}

esp_err_t Scd40_Init(i2c_port_t i2c_num, gpio_num_t sda, gpio_num_t scl, uint32_t baud) {
    esp_err_t ret = ESP_OK;
    scd40_device = i2c_malloc_device(i2c_num, sda, scl, baud, SCD40_I2C_ADDRESS);
    if (scd40_device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    return ret;
}
