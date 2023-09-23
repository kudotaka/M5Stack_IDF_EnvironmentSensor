#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "M5stack.h"

static const char *TAG = "M5Stack";

void M5Stack_Init(void) {
ESP_LOGD(TAG, "M5Stack_Init() start.");

#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
    M5Stack_Button_Init();
#endif

#if CONFIG_SOFTWARE_INTERNAL_SK6812_SUPPORT
    M5Stack_Sk6812_Init();
#endif

}

/* --------------------------------------------- BUTTON ----------------------------------------------*/
#if CONFIG_SOFTWARE_INTERNAL_BUTTON_SUPPORT
Button_Internal_Button_t* button_a;

void M5Stack_Button_Init(void) {
    Button_Internal_Init();
    if (Button_Internal_Enable(BUTTON_INT1_GPIO_PIN, BUTTON_INTERNAL_ACTIVE_LOW) == ESP_OK) {
        button_a = Button_Internal_Attach(BUTTON_INT1_GPIO_PIN, BUTTON_INTERNAL_ACTIVE_LOW);
    }
}
#endif
/* ----------------------------------------------- End -----------------------------------------------*/

/* --------------------------------------------- SK6812 ----------------------------------------------*/
#if CONFIG_SOFTWARE_INTERNAL_SK6812_SUPPORT
rmt_channel_handle_t sk6812_channel = NULL;
rmt_encoder_handle_t sk6812_encoder = NULL;
pixel_settings_t px;

esp_err_t M5Stack_Sk6812_Init(void) {
    ESP_LOGD(TAG, "M5Stack_Sk6812_Init() start.");

    esp_err_t res = ESP_OK;
    res = Sk6812_Init_Ex(&px, 1, RGB_INT1_GPIO_PIN, &sk6812_channel, &sk6812_encoder);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "M5Stack_Sk6812_Init() Sk6812_Init_Ex");
		return res;
	}
    res = Sk6812_Enable_Ex(&px, sk6812_channel);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "M5Stack_Sk6812_Init() Sk6812_Enable_Ex");
		return res;
	}
    if (sk6812_channel == NULL || sk6812_encoder == NULL) {
        ESP_LOGE(TAG, "M5Stack_Sk6812_Init() NULL!");
        return ESP_ERR_NOT_FINISHED;
    }
    res = Sk6812_Clear_Ex(&px, sk6812_channel, sk6812_encoder);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "M5Stack_Sk6812_Init() Sk6812_Clear_Ex");
		return res;
	}
    return res;
}

esp_err_t M5Stack_Sk6812_Show(uint32_t color) {
    ESP_LOGD(TAG, "M5Stack_Sk6812_Show() start.");

    esp_err_t res = ESP_OK;
    Sk6812_SetAllColor_Ex(&px, color);
    res = Sk6812_Show_Ex(&px, sk6812_channel, sk6812_encoder);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "M5Stack_Sk6812_Show() Sk6812_Show_Ex");
		return res;
	}
    return res;
}

esp_err_t M5Stack_Sk6812_Clear(void) {
    ESP_LOGD(TAG, "M5Stack_Sk6812_Clear() start.");

    esp_err_t res = ESP_OK;
    Sk6812_SetAllColor_Ex(&px, SK6812_COLOR_OFF);
    res = Sk6812_Show_Ex(&px, sk6812_channel, sk6812_encoder);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "M5Stack_Sk6812_Clear() Sk6812_Show_Ex");
		return res;
	}
    return res;
}

void M5Stack_Sk6812_SetBrightness(uint8_t brightness) {
    ESP_LOGD(TAG, "M5Stack_Sk6812_SetBrightness() start.");

    Sk6812_SetBrightness_Ex(&px, brightness);
}

#endif
/* ----------------------------------------------- End -----------------------------------------------*/
