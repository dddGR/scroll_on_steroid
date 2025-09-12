#ifndef _TOUCH_BUTTON_H_
#define _TOUCH_BUTTON_H_

#include "sdkconfig.h"
#include "user_Types.h"
#include "user_Config_priv.h"

#ifdef __cplusplus
extern "C" {
#endif


#if CONFIG_USE_TOUCH_BUILTIN
/** ESP32-S3 has built-in touch sensor */
esp_err_t init_TouchBuitin(const char *_TAG, int touch_channel_id, EventGroupHandle_t xEventGroup);
esp_err_t regTouchWakeup(const char *_TAG);
#else
esp_err_t init_TouchExternal(const char *_TAG, gpio_num_t pin_touch_sensor);
#endif

ButtonPressState_t getButtonPressState(EventGroupHandle_t xEventGroup, TaskHandle_t xTask);


#ifdef __cplusplus
}
#endif

#endif // _TOUCH_BUTTON_H_