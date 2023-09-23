#pragma once

#if CONFIG_SOFTWARE_EXTERNAL_BUTTON_SUPPORT
#include "button_external.h"
#endif

#if CONFIG_SOFTWARE_EXTERNAL_LED_SUPPORT
#include "led.h"
#endif

#if CONFIG_SOFTWARE_EXTERNAL_RTC_SUPPORT
#include "pcf8563.h"
#endif

#if CONFIG_SOFTWARE_EXTERNAL_SK6812_SUPPORT
#include "m5stack.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_SHT3X
#include "sht3x.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_BMP280
#include "bmp280.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_QMP6988
#include "qmp6988.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_BME680
#include "esp32_bme680.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_ADT7410
#include "adt7410.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_SCD30
#include "scd30.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_SCD40
#include "scd40.h"
#endif

#if CONFIG_SOFTWARE_SENSOR_MHZ19C
#include "mhz19c.h"
#endif
