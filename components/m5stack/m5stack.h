#pragma once
#include "freertos/FreeRTOS.h"

#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
#include "button_internal.h"
#endif

#if ( CONFIG_SOFTWARE_INTERNAL_SK6812_SUPPORT || CONFIG_SOFTWARE_EXTERNAL_SK6812_SUPPORT )
#include "sk6812.h"
#endif

void M5Stack_Init(void);

#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define BUTTON_INT1_GPIO_PIN GPIO_NUM_0
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define BUTTON_INT1_GPIO_PIN GPIO_NUM_41
#endif
extern Button_Internal_Button_t* button_a;
void M5Stack_Button_Init(void);
#endif

#if CONFIG_SOFTWARE_INTERNAL_SK6812_SUPPORT
#if CONFIG_SOFTWARE_MODEL_M5STAMP_S3
#define RGB_INT1_GPIO_PIN GPIO_NUM_21
#elif CONFIG_SOFTWARE_MODEL_M5ATOMS3LITE
#define RGB_INT1_GPIO_PIN GPIO_NUM_35
#endif
esp_err_t M5Stack_Sk6812_Init(void);
esp_err_t M5Stack_Sk6812_Show(uint32_t color);
esp_err_t M5Stack_Sk6812_Clear(void);
void M5Stack_Sk6812_SetBrightness(uint8_t brightness);
#endif
