#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"

#if CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT
#include "wifi.h"

#if CONFIG_SOFTWARE_ESP_MQTT_SUPPORT
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#endif //CONFIG_SOFTWARE_ESP_MQTT_SUPPORT

#endif //CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT

#if CONFIG_SOFTWARE_UI_SUPPORT
#include "ui.h"
#endif

#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
#include "driver/gpio.h"
#include "esp_sntp.h"
#endif

#include "m5stack.h"

#if (  CONFIG_SOFTWARE_SENSOR_SHT3X \
    || CONFIG_SOFTWARE_SENSOR_BMP280 \
    || CONFIG_SOFTWARE_SENSOR_QMP6988 \
    || CONFIG_SOFTWARE_SENSOR_BME680 \
    || CONFIG_SOFTWARE_SENSOR_ADT7410 \
    || CONFIG_SOFTWARE_SENSOR_SCD30 \
    || CONFIG_SOFTWARE_SENSOR_SCD40 \
    || CONFIG_SOFTWARE_SENSOR_MHZ19C \
    || CONFIG_SOFTWARE_EXTERNAL_SK6812_SUPPORT \
    || CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT )
#include "m5unit.h"
#endif

static const char *TAG = "MY-MAIN";


#if ( CONFIG_SOFTWARE_ESP_MQTT_SUPPORT || CONFIG_SOFTWARE_SENSOR_USE_SENSOR )
int8_t g_sensor_mode = 0;
float g_temperature = 0.0;
float g_humidity = 0.0;
float g_pressure = 0.0;
int g_co2 = 0;
#endif //CONFIG_SOFTWARE_SENSOR_USE_SENSOR

#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
#if CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT
static char g_lastSyncDatetime[72] = {0};
#endif //CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT
#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
#if CONFIG_SOFTWARE_EXTERNAL_RTC_CLOCKOUT_1KHZ_SUPPORT
static bool g_clockout_status = false;
#endif //CONFIG_SOFTWARE_EXTERNAL_RTC_CLOCKOUT_1KHZ_SUPPORT
#endif //CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
#endif //CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT

#if CONFIG_SOFTWARE_CLOCK_SOUND_SUPPORT
uint8_t g_clockCount = 0;
uint8_t g_clockCurrent = 0;
uint8_t g_clockSoundEnabled = 1;
#endif

#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
TaskHandle_t xInternalButton;
static void vInternal_button_task(void* pvParameters) {
    ESP_LOGI(TAG, "start INTERNAL Button");

    while(1){

        if (Button_Internal_WasPressed(button_a)) {
            ESP_LOGI(TAG, "BUTTON A PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif //CONFIG_SOFTWARE_UI_SUPPORT
        }
        if (Button_Internal_WasReleased(button_a)) {
            ESP_LOGI(TAG, "BUTTON A RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif //CONFIG_SOFTWARE_UI_SUPPORT
        }
        if (Button_Internal_WasLongPress(button_a, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON A LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
            ui_status_set(g_lastSyncDatetime);
#endif //CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
#endif //CONFIG_SOFTWARE_UI_SUPPORT

#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
#if CONFIG_SOFTWARE_EXTERNAL_RTC_CLOCKOUT_1KHZ_SUPPORT
            if (g_clockout_status)
            {
                g_clockout_status = 0;
            }
            else
            {
                g_clockout_status = 1;
            }
            PCF8563_ClockOutForTrimmer(g_clockout_status);
            ESP_LOGI(TAG, "PCF8563_ClockOutForTrimmer: %d", g_clockout_status);
#endif //CONFIG_SOFTWARE_EXTERNAL_RTC_CLOCKOUT_1KHZ_SUPPORT
#endif //CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT

        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define PORT_I2C_NUM I2C_NUM_0
#define PORT_SDA_PIN GPIO_NUM_13
#define PORT_SCL_PIN GPIO_NUM_15
#define PORT_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define PORT_I2C_NUM I2C_NUM_0
#define PORT_SDA_PIN GPIO_NUM_2
#define PORT_SCL_PIN GPIO_NUM_1
#define PORT_I2C_STANDARD_BAUD 400000
#endif
TaskHandle_t xClock;
void RtcInit()
{
    ESP_LOGI(TAG, "start I2C PCF8563");
    esp_err_t ret = ESP_OK;

    ret = PCF8563_Init(PORT_I2C_NUM, PORT_SDA_PIN, PORT_SCL_PIN, PORT_I2C_STANDARD_BAUD);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCF8563_Init Error");
        return;
    }
    ESP_LOGI(TAG, "PCF8563_Init() is OK!");
}

#if CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT
TaskHandle_t xExternalRtc;
static bool g_timeInitialized = false;
//const char servername[] = "ntp.jst.mfeed.ad.jp";
//const char servername[] = CONFIG_NTP_SERVER_NAME;

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    g_timeInitialized = true;
}

void vExternal_rtc_task(void *pvParametes)
{
    //PCF8563
    ESP_LOGI(TAG, "start EXTERNAL Rtc");

    // Set timezone to Japan Standard Time
    setenv("TZ", "JST-9", 1);
    tzset();

    ESP_LOGI(TAG, "NTP ServerName:%s", CONFIG_NTP_SERVER_NAME);
    esp_sntp_setservername(0, CONFIG_NTP_SERVER_NAME);

    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    while (1) {
        if (wifi_isConnected() == ESP_OK) {
            esp_sntp_init();
        } else {
            vTaskDelay( pdMS_TO_TICKS(60000) );
            continue;
        }

        ESP_LOGI(TAG, "Waiting for time synchronization with SNTP server");
        while (!g_timeInitialized)
        {
            vTaskDelay( pdMS_TO_TICKS(5000) );
        }

        time_t now = 0;
        struct tm timeinfo = {0};
        time(&now);
        localtime_r(&now, &timeinfo);
        sprintf(g_lastSyncDatetime,"NTP Update : %04d/%02d/%02d %02d:%02d", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
        ESP_LOGI(TAG, "%s", g_lastSyncDatetime);
#if CONFIG_SOFTWARE_UI_SUPPORT
        ui_status_set(g_lastSyncDatetime);
#endif

        rtc_date_t rtcdate;
        rtcdate.year = timeinfo.tm_year+1900;
        rtcdate.month = timeinfo.tm_mon+1;
        rtcdate.day = timeinfo.tm_mday;
        rtcdate.hour = timeinfo.tm_hour;
        rtcdate.minute = timeinfo.tm_min;
        rtcdate.second = timeinfo.tm_sec;
        PCF8563_SetTime(&rtcdate);

        g_timeInitialized = false;
        esp_sntp_stop();
//        vTaskDelay( pdMS_TO_TICKS(6000000) );
        vTaskDelay( pdMS_TO_TICKS(CONFIG_NTP_UPDATE_INTERVAL_TIME_MS) );
    }
}
#endif

void vClock_task(void *pvParametes)
{
    //PCF8563
    ESP_LOGI(TAG, "start Clock");
 
    // Set timezone to Japan Standard Time
    setenv("TZ", "JST-9", 1);
    tzset();

    while (1) {
        rtc_date_t rtcdate;
        PCF8563_GetTime(&rtcdate);
#if CONFIG_SOFTWARE_CLOCK_SOUND_SUPPORT
        if (g_clockCurrent != rtcdate.hour) {
            if (rtcdate.hour == 0) { // 0:00
                g_clockCount = 12;
            } else if (rtcdate.hour > 12) { // 13:00-23:00
                g_clockCount = rtcdate.hour - 12;
            } else { // 1:00-12:00
                g_clockCount = rtcdate.hour;
            }
            g_clockCurrent = rtcdate.hour;
            vTaskResume(xSpeaker);
        }
#endif
        char str1[30] = {0};
        sprintf(str1,"%04d/%02d/%02d %02d:%02d:%02d", rtcdate.year, rtcdate.month, rtcdate.day, rtcdate.hour, rtcdate.minute, rtcdate.second);
#if CONFIG_SOFTWARE_UI_SUPPORT
        ui_datetime_set(str1);
#else
#if CONFIG_NTP_CLOCK_LOG_ENABLE
    ESP_LOGI(TAG, "%s", str1);
#endif
#endif

        vTaskDelay( pdMS_TO_TICKS(990) );
    }
}
#endif


#if CONFIG_SOFTWARE_SENSOR_USE_SENSOR
#if (CONFIG_SOFTWARE_ESP_MQTT_SUPPORT != 1)
TaskHandle_t xSensorViewer;
void vSensor_Viewer_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start Sensor Viewer");
    vTaskDelay( pdMS_TO_TICKS(10000) );

    #if CONFIG_SOFTWARE_SENSOR_TYPE_TEMPERATURE
    g_sensor_mode += 1;
    #endif
    #if CONFIG_SOFTWARE_SENSOR_TYPE_HUMIDITY
    g_sensor_mode += 2;
    #endif
    #if CONFIG_SOFTWARE_SENSOR_TYPE_PRESSURE
    g_sensor_mode += 4;
    #endif
    #if CONFIG_SOFTWARE_SENSOR_TYPE_CO2
    g_sensor_mode += 8;
    #endif

    while (1) {
        ESP_LOGD(TAG, "SENSOR mode %d", g_sensor_mode);
        switch (g_sensor_mode)
        {
        case 1:
            ESP_LOGI(TAG, "SENSOR temperature:%f", g_temperature);
            break;
        case 2:
            ESP_LOGI(TAG, "SENSOR humidity:%f", g_humidity);
            break;
        case 3:
            ESP_LOGI(TAG, "SENSOR temperature:%f humidity:%f", g_temperature, g_humidity);
            break;
        case 4:
            ESP_LOGI(TAG, "SENSOR pressure:%f", g_pressure);
            break;
        case 5:
            ESP_LOGI(TAG, "SENSOR temperature:%f pressure:%f", g_temperature, g_pressure);
            break;
        case 6:
            ESP_LOGI(TAG, "SENSOR humidity:%f pressure:%f", g_humidity, g_pressure);
            break;
        case 7:
            ESP_LOGI(TAG, "SENSOR temperature:%f humidity:%f pressure:%f", g_temperature, g_humidity, g_pressure);
            break;
        case 8:
            ESP_LOGI(TAG, "SENSOR CO2:%d", g_co2);
            break;
        case 9:
            ESP_LOGI(TAG, "SENSOR temperature:%f CO2:%d", g_temperature, g_co2);
            break;
        case 10:
            ESP_LOGI(TAG, "SENSOR humidity:%f CO2:%d", g_humidity, g_co2);
            break;
        case 11:
            ESP_LOGI(TAG, "SENSOR temperature:%f humidity:%f CO2:%d", g_temperature, g_humidity, g_co2);
            break;
        case 12:
            ESP_LOGI(TAG, "SENSOR pressure:%f CO2:%d", g_pressure, g_co2);
            break;
        case 13:
            ESP_LOGI(TAG, "SENSOR temperature:%f pressure:%f CO2:%d", g_temperature, g_pressure, g_co2);
            break;
        case 14:
            ESP_LOGI(TAG, "SENSOR humidity:%f pressure:%f CO2:%d", g_humidity, g_pressure, g_co2);
            break;
        case 15:
            ESP_LOGI(TAG, "SENSOR temperature:%f humidity:%f pressure:%f CO2:%d", g_temperature, g_humidity, g_pressure, g_co2);
            break;
        default:
            break;
        }

        vTaskDelay( pdMS_TO_TICKS(10000) );
    }
}
#endif //(CONFIG_SOFTWARE_ESP_MQTT_SUPPORT != 1)
#endif //CONFIG_SOFTWARE_SENSOR_USE_SENSOR

#if CONFIG_SOFTWARE_SENSOR_SHT3X
TaskHandle_t xExternalSht3x;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_SHT3X_I2C_NUM I2C_NUM_0
#define EXTERNAL_SHT3X_SDA_PIN GPIO_NUM_13
#define EXTERNAL_SHT3X_SCL_PIN GPIO_NUM_15
#define EXTERNAL_SHT3X_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_SHT3X_I2C_NUM I2C_NUM_1
#define EXTERNAL_SHT3X_SDA_PIN GPIO_NUM_38
#define EXTERNAL_SHT3X_SCL_PIN GPIO_NUM_39
#define EXTERNAL_SHT3X_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Sht3x_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Sht3x");
    esp_err_t ret = ESP_OK;
    ret = Sht3x_Init(EXTERNAL_SHT3X_I2C_NUM, EXTERNAL_SHT3X_SDA_PIN, EXTERNAL_SHT3X_SCL_PIN, EXTERNAL_SHT3X_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sht3x_Init Error");
        g_sensor_mode -= 3;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGD(TAG, "Sht3x_Init() is OK!");

    while (1) {
        ret = Sht3x_Read();
        if (ret == ESP_OK) {
            vTaskDelay( pdMS_TO_TICKS(100) );
            g_temperature = Sht3x_GetTemperature();
            g_humidity = Sht3x_GetHumidity();
        } else {
            ESP_LOGE(TAG, "Sht3x_Read() is error code:%d", ret);
            vTaskDelay( pdMS_TO_TICKS(10000) );
        }

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_SHT3X

#if CONFIG_SOFTWARE_SENSOR_BMP280
TaskHandle_t xExternalBmp280;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_BMP280_I2C_NUM I2C_NUM_0
#define EXTERNAL_BMP280_SDA_PIN GPIO_NUM_13
#define EXTERNAL_BMP280_SCL_PIN GPIO_NUM_15
#define EXTERNAL_BMP280_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_BMP280_I2C_NUM I2C_NUM_1
#define EXTERNAL_BMP280_SDA_PIN GPIO_NUM_38
#define EXTERNAL_BMP280_SCL_PIN GPIO_NUM_39
#define EXTERNAL_BMP280_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Bmp280_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Bmp280");
    esp_err_t ret = ESP_OK;
    ret = Bmp280_Init(EXTERNAL_BMP280_I2C_NUM, EXTERNAL_BMP280_SDA_PIN, EXTERNAL_BMP280_SCL_PIN, EXTERNAL_BMP280_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bmp280_Init Error");
        g_sensor_mode -= 4;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGD(TAG, "Bmp280_Init() is OK!");

    while (1) {
        vTaskDelay( pdMS_TO_TICKS(100) );
        g_pressure = Bmp280_getPressure() / 100.0;

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_BMP280

#if CONFIG_SOFTWARE_SENSOR_QMP6988
TaskHandle_t xExternalQmp6988;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_QMP6988_I2C_NUM I2C_NUM_0
#define EXTERNAL_QMP6988_SDA_PIN GPIO_NUM_13
#define EXTERNAL_QMP6988_SCL_PIN GPIO_NUM_15
#define EXTERNAL_QMP6988_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_QMP6988_I2C_NUM I2C_NUM_1
#define EXTERNAL_QMP6988_SDA_PIN GPIO_NUM_38
#define EXTERNAL_QMP6988_SCL_PIN GPIO_NUM_39
#define EXTERNAL_QMP6988_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Qmp6988_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Qmp6988");
    esp_err_t ret = ESP_OK;
    ret = Qmp6988_Init(EXTERNAL_QMP6988_I2C_NUM, EXTERNAL_QMP6988_SDA_PIN, EXTERNAL_QMP6988_SCL_PIN, EXTERNAL_QMP6988_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Qmp6988_Init Error");
        g_sensor_mode -= 4;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGD(TAG, "Qmp6988_Init() is OK!");

    while (1) {
        vTaskDelay( pdMS_TO_TICKS(100) );
        g_pressure = Qmp6988_CalcPressure() / 100;

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_QMP6988

#if CONFIG_SOFTWARE_SENSOR_BME680
TaskHandle_t xExternalBme680;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_BME680_I2C_NUM I2C_NUM_0
#define EXTERNAL_BME680_SDA_PIN GPIO_NUM_13
#define EXTERNAL_BME680_SCL_PIN GPIO_NUM_15
#define EXTERNAL_BME680_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_BME680_I2C_NUM I2C_NUM_1
#define EXTERNAL_BME680_SDA_PIN GPIO_NUM_38
#define EXTERNAL_BME680_SCL_PIN GPIO_NUM_39
#define EXTERNAL_BME680_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Bme680_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Bme680");
    esp_err_t ret = ESP_OK;
    ret = Esp32_Bme680_Init(EXTERNAL_BME680_I2C_NUM, EXTERNAL_BME680_SDA_PIN, EXTERNAL_BME680_SCL_PIN, EXTERNAL_BME680_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Esp32_Bme680_Init Error");
        g_sensor_mode -= 7;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGD(TAG, "Esp32_Bme680_Init() is OK!");

    while (1) {
        if (Esp32_Bme680_read_sensor_data() == 0) {
            g_temperature = Esp32_Bme680_get_temperature();
            g_humidity = Esp32_Bme680_get_humidity();
            g_pressure = Esp32_Bme680_get_pressure() / 100.0;
//            g_gas = Esp32_Bme680_get_gas() / 1000.0;
        }

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_BME680

#if CONFIG_SOFTWARE_SENSOR_ADT7410
TaskHandle_t xExternalAdt7410;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_ADT7410_I2C_NUM I2C_NUM_0
#define EXTERNAL_ADT7410_SDA_PIN GPIO_NUM_13
#define EXTERNAL_ADT7410_SCL_PIN GPIO_NUM_15
#define EXTERNAL_ADT7410_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_ADT7410_I2C_NUM I2C_NUM_1
#define EXTERNAL_ADT7410_SDA_PIN GPIO_NUM_38
#define EXTERNAL_ADT7410_SCL_PIN GPIO_NUM_39
#define EXTERNAL_ADT7410_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Adt7410_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Adt7410");
    esp_err_t ret = ESP_OK;
    ret = Adt7410_Init(EXTERNAL_ADT7410_I2C_NUM, EXTERNAL_ADT7410_SDA_PIN, EXTERNAL_ADT7410_SCL_PIN, EXTERNAL_ADT7410_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Adt7410_Init Error");
        g_sensor_mode -= 1;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGD(TAG, "Adt7410_Init() is OK!");

    while (1) {
        g_temperature = Adt7410_getTemperature_13bit();

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_ADT7410

#if CONFIG_SOFTWARE_SENSOR_SCD30
TaskHandle_t xExternalScd30;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_SCD30_I2C_NUM I2C_NUM_0
#define EXTERNAL_SCD30_SDA_PIN GPIO_NUM_13
#define EXTERNAL_SCD30_SCL_PIN GPIO_NUM_15
#define EXTERNAL_SCD30_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_SCD30_I2C_NUM I2C_NUM_1
#define EXTERNAL_SCD30_SDA_PIN GPIO_NUM_38
#define EXTERNAL_SCD30_SCL_PIN GPIO_NUM_39
#define EXTERNAL_SCD30_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Scd30_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Scd30");
    esp_err_t ret = ESP_OK;
    ret = Scd30_Init(EXTERNAL_SCD30_I2C_NUM, EXTERNAL_SCD30_SDA_PIN, EXTERNAL_SCD30_SCL_PIN, EXTERNAL_SCD30_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd30_Init Error");
        g_sensor_mode -= 11;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ret = Scd30_SetAutoSelfCalibration(false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd30_SetAutoSelfCalibration() is Error");
    }
    ret = Scd30_SetTemperatureOffset(220); // 2.2*100
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd30_SetTemperatureOffset() is Error");
    }
    vTaskDelay( pdMS_TO_TICKS(200) );

    ESP_LOGD(TAG, "Scd30_Init() is OK!");

    while (1) {
        float scd30_tmp[3] = { 0.0 };
        if (Scd30_IsAvailable() != false) {
            ret = Scd30_ReadMeasurement(scd30_tmp);
            if (ret == ESP_OK) {
                g_co2 = scd30_tmp[0];         //scd30_co2Concentration
                g_temperature = scd30_tmp[1]; //scd30_temperature
                g_humidity = scd30_tmp[2];    //scd30_humidity
            } else {
                ESP_LOGE(TAG, "Scd30_ReadMeasurement is error code:%d", ret);
                vTaskDelay( pdMS_TO_TICKS(10000) );
            }
        }

        vTaskDelay( pdMS_TO_TICKS(10000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_SCD30

#if CONFIG_SOFTWARE_SENSOR_SCD40
TaskHandle_t xExternalScd40;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_SCD40_I2C_NUM I2C_NUM_0
#define EXTERNAL_SCD40_SDA_PIN GPIO_NUM_13
#define EXTERNAL_SCD40_SCL_PIN GPIO_NUM_15
#define EXTERNAL_SCD40_I2C_STANDARD_BAUD 400000
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_SCD40_I2C_NUM I2C_NUM_1
#define EXTERNAL_SCD40_SDA_PIN GPIO_NUM_38
#define EXTERNAL_SCD40_SCL_PIN GPIO_NUM_39
#define EXTERNAL_SCD40_I2C_STANDARD_BAUD 400000
#endif
void vExternal_Scd40_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Scd40");
    esp_err_t ret = ESP_OK;
    ret = Scd40_Init(EXTERNAL_SCD40_I2C_NUM, EXTERNAL_SCD40_SDA_PIN, EXTERNAL_SCD40_SCL_PIN, EXTERNAL_SCD40_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_Init Error");
        g_sensor_mode -= 11;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }
    ret = Scd40_StopPeriodicMeasurement();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_StopPeriodicMeasurement() is Error code:%X", ret);
    }
    ret = Scd40_SetAutoSelfCalibration(false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_SetAutoSelfCalibration() is Error code:%X", ret);
    }
    ret = Scd40_StartPeriodicMeasurement();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_StartPeriodicMeasurement() is Error code:%X", ret);
    }
    vTaskDelay( pdMS_TO_TICKS(1000) );

    ESP_LOGD(TAG, "Scd40_Init() is OK!");

    while (1) {
        float scd40_tmp[3] = { 0.0 };
        if (Scd40_IsAvailable() != false) {
            ret = Scd40_ReadMeasurement(scd40_tmp);
            if (ret == ESP_OK) {
                g_co2 = scd40_tmp[0];//scd40_co2Concentration
                g_temperature = scd40_tmp[1];//scd40_temperature
                g_humidity = scd40_tmp[2];//scd40_humidity
            } else {
                ESP_LOGE(TAG, "Scd40_GetCarbonDioxideConcentration is error code:%X", ret);
                vTaskDelay( pdMS_TO_TICKS(10000) );
            }
        }

        vTaskDelay( pdMS_TO_TICKS(10000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_SCD40

#if CONFIG_SOFTWARE_SENSOR_MHZ19C
TaskHandle_t xExternalMhz19c;
bool g_mhz19cInitialized = false;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define EXTERNAL_MHZ19C_UART_NUM UART_NUM_0
#define EXTERNAL_MHZ19C_TXD_PIN GPIO_NUM_43
#define EXTERNAL_MHZ19C_RXD_PIN GPIO_NUM_44
#define EXTERNAL_MHZ19C_UART_BAUD_RATE 9600
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define EXTERNAL_MHZ19C_UART_NUM UART_NUM_1
#define EXTERNAL_MHZ19C_TXD_PIN GPIO_NUM_5
#define EXTERNAL_MHZ19C_RXD_PIN GPIO_NUM_6
#define EXTERNAL_MHZ19C_UART_BAUD_RATE 9600
#endif
void vExternal_Mhz19c_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL Mhz19c");
    esp_err_t ret = ESP_OK;
    ret = Mhz19c_Init(EXTERNAL_MHZ19C_UART_NUM, EXTERNAL_MHZ19C_TXD_PIN, EXTERNAL_MHZ19C_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, EXTERNAL_MHZ19C_UART_BAUD_RATE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mhz19c_Init Error");
        g_sensor_mode -= 8;
        vTaskDelay( pdMS_TO_TICKS(500) );
        vTaskDelete(NULL);
        return;
    }

    while (!g_mhz19cInitialized)
    {
        Mhz19c_SetAutoCalibration(EXTERNAL_MHZ19C_UART_NUM, false);
        vTaskDelay( pdMS_TO_TICKS(5000) );

        g_co2 = Mhz19c_GetCO2Concentration(EXTERNAL_MHZ19C_UART_NUM);
        if (g_co2 != 0)
        {
            g_mhz19cInitialized = true;
        }
        ESP_LOGE(TAG, "Mhz19c_SetAutoCalibration retry");
        Mhz19c_SetAutoCalibration(EXTERNAL_MHZ19C_UART_NUM, true);
        vTaskDelay( pdMS_TO_TICKS(5000) );
    }

    ESP_LOGD(TAG, "Mhz19c_Init() is OK!");
    int tmp_co2 = 0;
    while (1) {
        tmp_co2 = 0;
        tmp_co2 = Mhz19c_GetCO2Concentration(EXTERNAL_MHZ19C_UART_NUM);
        if (tmp_co2 != 0)
        {
            g_co2 = tmp_co2;
        } else {
            ESP_LOGE(TAG, "Mhz19c_GetCO2Concentration is return co2 = 0");
            vTaskDelay( pdMS_TO_TICKS(1000) );
        }

        vTaskDelay( pdMS_TO_TICKS(10000) );
    }
}
#endif //CONFIG_SOFTWARE_SENSOR_MHZ19C



#if CONFIG_SOFTWARE_EXTERNAL_LED_SUPPORT
// SELECT GPIO_NUM_XX
TaskHandle_t xExternalLED;
Led_t* led_ext1;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define LED_EXT1_GPIO_PIN GPIO_NUM_9
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define LED_EXT1_GPIO_PIN GPIO_NUM_8
#endif
// SELECT GPIO_NUM_X (Active_HIGH)
static void vExternal_led_task(void* pvParameters) {
    ESP_LOGI(TAG, "start EXTERNAL LED");
    Led_Init();

    if (Led_Enable(LED_EXT1_GPIO_PIN) == ESP_OK) {
        led_ext1 = Led_Attach(LED_EXT1_GPIO_PIN);
    }
    while(1){
        Led_OnOff(led_ext1, true);
        vTaskDelay(pdMS_TO_TICKS(700));

        Led_OnOff(led_ext1, false);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_EXTERNAL_BUTTON_SUPPORT
// SELECT GPIO_NUM_XX
TaskHandle_t xExternalButton;
Button_External_Button_t* button_ext1;
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define BUTTON_EXT1_GPIO_PIN GPIO_NUM_1
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define BUTTON_EXT1_GPIO_PIN GPIO_NUM_7
#endif
static void vExternal_button_task(void* pvParameters) {
    ESP_LOGI(TAG, "start EXTERNAL Button");

#if 0
    // ONLY M5Stick C Plus
    M5Stick_Port_PinMode(GPIO_NUM_25, INPUT);
#endif

    Button_External_Button_Init();
    if (Button_External_Button_Enable(BUTTON_EXT1_GPIO_PIN, BUTTON_EXTERNAL_ACTIVE_LOW) == ESP_OK) {
        button_ext1 = Button_External_Button_Attach(BUTTON_EXT1_GPIO_PIN, BUTTON_EXTERNAL_ACTIVE_LOW);
    }
    while(1){
        if (Button_External_Button_WasPressed(button_ext1)) {
            ESP_LOGI(TAG, "BUTTON EXT1 PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_External_Button_WasReleased(button_ext1)) {
            ESP_LOGI(TAG, "BUTTON EXT1 RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_External_Button_WasLongPress(button_ext1, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON EXT1 LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif


#if CONFIG_SOFTWARE_ESP_MQTT_SUPPORT
TaskHandle_t xEspMqttClient;
esp_mqtt_client_handle_t mqttClient;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA, TOPIC=%s, DATA=%s", event->topic , event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .buffer.size = CONFIG_BROKER_BUFFER_SIZE,
        .session.protocol_ver = CONFIG_MQTT_PROTOCOL_311,
        .session.last_will.qos = CONFIG_BROKER_LWT_QOS,
        .credentials.client_id = CONFIG_BROKER_MY_DEVICE_ID,
    };

    mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (mqttClient == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init() is error.");
        return;
    }
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqttClient, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqttClient));
}

static void vEspMqttClient_task(void* pvParameters) {
    ESP_LOGI(TAG, "start EspMqttClient_task");

    // connected wifi
    while (1) {
        if (wifi_isConnected() == ESP_OK) {
            mqtt_app_start();
            break;
        }
        vTaskDelay( pdMS_TO_TICKS(30000) );
    }

    int msg_id;
    char pubMessage[128] = {0};
    vTaskDelay( pdMS_TO_TICKS(10000) );

    #if CONFIG_SOFTWARE_SENSOR_TYPE_TEMPERATURE
    g_sensor_mode += 1;
    #endif
    #if CONFIG_SOFTWARE_SENSOR_TYPE_HUMIDITY
    g_sensor_mode += 2;
    #endif
    #if CONFIG_SOFTWARE_SENSOR_TYPE_PRESSURE
    g_sensor_mode += 4;
    #endif
    #if CONFIG_SOFTWARE_SENSOR_TYPE_CO2
    g_sensor_mode += 8;
    #endif

    while (1) {
        // connected wifi
        if (wifi_isConnected() == ESP_OK) {

//            ESP_LOGI(TAG, "SENSOR mode %d", g_sensor_mode);
            switch (g_sensor_mode)
            {
            case 1:
                sprintf(pubMessage, "{\"temperature\":%4.1f}", g_temperature);
                break;
            case 2:
                sprintf(pubMessage, "{\"humidity\":%4.1f}", g_humidity);
                break;
            case 3:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f}", g_temperature, g_humidity);
                break;
            case 4:
                sprintf(pubMessage, "{\"pressure\":%4.1f}", g_pressure);
                break;
            case 5:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"pressure\":%4.1f}", g_temperature, g_pressure);
                break;
            case 6:
                sprintf(pubMessage, "{\"humidity\":%4.1f,\"pressure\":%4.1f}", g_humidity, g_pressure);
                break;
            case 7:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"pressure\":%4.1f}", g_temperature, g_humidity, g_pressure);
                break;
            case 8:
                sprintf(pubMessage, "{\"co2\":%4d}", g_co2);
                break;
            case 9:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"co2\":%4d}", g_temperature, g_co2);
                break;
            case 10:
                sprintf(pubMessage, "{\"humidity\":%4.1f,\"co2\":%4d}", g_humidity, g_co2);
                break;
            case 11:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"co2\":%4d}", g_temperature, g_humidity, g_co2);
                break;
            case 12:
                sprintf(pubMessage, "{\"pressure\":%4.1f,\"co2\":%4d}", g_pressure, g_co2);
                break;
            case 13:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"pressure\":%4.1f,\"co2\":%4d}", g_temperature, g_pressure, g_co2);
                break;
            case 14:
                sprintf(pubMessage, "{\"humidity\":%4.1f,\"pressure\":%4.1f,\"co2\":%4d}", g_humidity, g_pressure, g_co2);
                break;
            case 15:
                sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"pressure\":%4.1f,\"co2\":%4d}", g_temperature, g_humidity, g_pressure, g_co2);
                break;
            default:
                break;
            }
            msg_id = esp_mqtt_client_publish(mqttClient, CONFIG_BROKER_MY_PUB_TOPIC, pubMessage, 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d pubMessage=%s", msg_id, pubMessage);
        } else {
            ESP_LOGI(TAG, "wifi not Connect. wait for ...");
        }

//        vTaskDelay( pdMS_TO_TICKS(60000) );
        vTaskDelay( pdMS_TO_TICKS(CONFIG_BROKER_PUBLISH_INTERVAL_TIME_MS) );
    }

    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_INTERNAL_SK6812_SUPPORT
TaskHandle_t xInternalRGBLedBlink;
void vInternal_RGBLedBlink_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start INTERNAL RGBLedBlink");

    M5Stack_Sk6812_SetBrightness(50);
    while (1) {
        M5Stack_Sk6812_Show(SK6812_COLOR_LIME);
        vTaskDelay(pdMS_TO_TICKS(1000));

        M5Stack_Sk6812_Show(SK6812_COLOR_OFF);
        vTaskDelay(pdMS_TO_TICKS(30000));
    }

    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_EXTERNAL_SK6812_SUPPORT
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define RGBLED_EXT1_GPIO_PIN GPIO_NUM_5
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define RGBLED_EXT1_GPIO_PIN GPIO_NUM_39
#endif
TaskHandle_t xExternalRGBLedBlink;
rmt_channel_handle_t rgb_ext1_channel = NULL;
rmt_encoder_handle_t rgb_ext1_encoder = NULL;
pixel_settings_t px_ext1;

void vExternal_RGBLedBlink_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start EXTERNAL RGBLedBlink");

    esp_err_t res = ESP_OK;
    uint8_t blink_count = 5;
    uint32_t colors[] = {SK6812_COLOR_BLUE, SK6812_COLOR_LIME, SK6812_COLOR_AQUA
                    , SK6812_COLOR_RED, SK6812_COLOR_MAGENTA, SK6812_COLOR_YELLOW
                    , SK6812_COLOR_WHITE};

    res = Sk6812_Init_Ex(&px_ext1, 8, RGBLED_EXT1_GPIO_PIN, &rgb_ext1_channel, &rgb_ext1_encoder);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "vExternal_RGBLedBlink_task() Sk6812_Init_Ex");
	}
    res = Sk6812_Enable_Ex(&px_ext1, rgb_ext1_channel);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "vExternal_RGBLedBlink_task() Sk6812_Enable_Ex");
	}
    if (rgb_ext1_channel == NULL || rgb_ext1_encoder == NULL) {
        ESP_LOGE(TAG, "vExternal_RGBLedBlink_task() NULL!");
    }
    res = Sk6812_Clear_Ex(&px_ext1, rgb_ext1_channel, rgb_ext1_encoder);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "vExternal_RGBLedBlink_task() Sk6812_Clear_Ex");
	}

    while (1) {
        for (uint8_t c = 0; c < sizeof(colors)/sizeof(uint32_t); c++) {
            for (uint8_t i = 0; i < blink_count; i++) {
                Sk6812_SetAllColor_Ex(&px_ext1, colors[c]);
                Sk6812_Show_Ex(&px_ext1, rgb_ext1_channel, rgb_ext1_encoder);
                vTaskDelay(pdMS_TO_TICKS(1000));

                Sk6812_SetAllColor_Ex(&px_ext1, SK6812_COLOR_OFF);
                Sk6812_Show_Ex(&px_ext1, rgb_ext1_channel, rgb_ext1_encoder);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "app_main() start.");
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("MY-MAIN", ESP_LOG_INFO);
//    esp_log_level_set("MY-UI", ESP_LOG_INFO);
//    esp_log_level_set("MY-WIFI", ESP_LOG_INFO);


    M5Stack_Init();

#if CONFIG_SOFTWARE_UI_SUPPORT
    ui_init();
#endif

#if CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT
    wifi_initialise();
#endif

#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
    // BUTTON
    xTaskCreatePinnedToCore(&vInternal_button_task, "internal_button_task", 4096 * 1, NULL, 2, &xInternalButton, 1);
#endif

#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
    RtcInit();
#if CONFIG_SOFTWARE_INTERNAL_WIFI_SUPPORT
    // rtc
    xTaskCreatePinnedToCore(&vExternal_rtc_task, "vExternal_rtc_task", 4096 * 1, NULL, 2, &xExternalRtc, 1);
#endif
    // clock
    xTaskCreatePinnedToCore(&vClock_task, "clock_task", 4096 * 1, NULL, 2, &xClock, 1);
#endif

#if CONFIG_SOFTWARE_SENSOR_USE_SENSOR
#if (CONFIG_SOFTWARE_ESP_MQTT_SUPPORT != 1)
    // SENSOR VIEWER
    xTaskCreatePinnedToCore(&vSensor_Viewer_task, "vSensor_Viewer_task", 4096 * 1, NULL, 2, &xSensorViewer, 1);
#endif
#endif
#if CONFIG_SOFTWARE_SENSOR_SHT3X
    // SHT3X
    xTaskCreatePinnedToCore(&vExternal_Sht3x_task, "vExternal_Sht3x_task", 4096 * 1, NULL, 2, &xExternalSht3x, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_BMP280
    // BMP280
    xTaskCreatePinnedToCore(&vExternal_Bmp280_task, "vExternal_Bmp280_task", 4096 * 1, NULL, 2, &xExternalBmp280, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_QMP6988
    // QMP6988
    xTaskCreatePinnedToCore(&vExternal_Qmp6988_task, "vExternal_Qmp6988_task", 4096 * 1, NULL, 2, &xExternalQmp6988, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_BME680
    // BME680
    xTaskCreatePinnedToCore(&vExternal_Bme680_task, "vExternal_Bme680_task", 4096 * 1, NULL, 2, &xExternalBme680, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_ADT7410
    // ADT7410
    xTaskCreatePinnedToCore(&vExternal_Adt7410_task, "vExternal_Adt7410_task", 4096 * 1, NULL, 2, &xExternalAdt7410, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_SCD30
    // SCD30
    xTaskCreatePinnedToCore(&vExternal_Scd30_task, "vExternal_Scd30_task", 4096 * 1, NULL, 2, &xExternalScd30, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_SCD40
    // SCD40
    xTaskCreatePinnedToCore(&vExternal_Scd40_task, "vExternal_Scd40_task", 4096 * 1, NULL, 2, &xExternalScd40, 1);
#endif
#if CONFIG_SOFTWARE_SENSOR_MHZ19C
    // MHZ19C
    xTaskCreatePinnedToCore(&vExternal_Mhz19c_task, "vExternal_Mhz19c_task", 4096 * 1, NULL, 2, &xExternalMhz19c, 1);
#endif

#if CONFIG_SOFTWARE_EXTERNAL_LED_SUPPORT
    // EXTERNAL LED
    xTaskCreatePinnedToCore(&vExternal_led_task, "external_led_task", 4096 * 1, NULL, 2, &xExternalLED, 1);
#endif

#if CONFIG_SOFTWARE_EXTERNAL_BUTTON_SUPPORT
    // EXTERNAL BUTTON
    xTaskCreatePinnedToCore(&vExternal_button_task, "external_button_task", 4096 * 1, NULL, 2, &xExternalButton, 1);
#endif

#if CONFIG_SOFTWARE_ESP_MQTT_SUPPORT
    // ESP_MQTT
    xTaskCreatePinnedToCore(&vEspMqttClient_task, "vEspMqttClient_task", 4096 * 1, NULL, 2, &xEspMqttClient, 1);
#endif

#if CONFIG_SOFTWARE_INTERNAL_SK6812_SUPPORT
    // INTERNAL RGB LED Blink
    xTaskCreatePinnedToCore(&vInternal_RGBLedBlink_task, "Internal_rgb_led_blink_task", 4096 * 1, NULL, 2, &xInternalRGBLedBlink, 1);
#endif

#if CONFIG_SOFTWARE_EXTERNAL_SK6812_SUPPORT
    // EXTERNAL RGB LED BLINK
    xTaskCreatePinnedToCore(&vExternal_RGBLedBlink_task, "External_RGBLedBlink_task", 4096 * 1, NULL, 2, &xExternalRGBLedBlink, 1);
#endif

}
