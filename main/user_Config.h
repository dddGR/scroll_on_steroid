#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include "sdkconfig.h"
#include "soc/gpio_num.h"  // IWYU pragma: export

#define DEVICE_NAME         CONFIG_DEVICE_NAME
#define DEVICE_MANUFACTURER "DDDGR"
#define DEVICE_SERIAL       "1234567890"

/**
 * @note If you are using configurations defined in the config.h file, 
 * make sure to go to menuconfig and disable the option 
 * {Use menuconfig to set parameters}. If not, this may result in conflicts. 
 */
#if !CONFIG_USE_MENU_CONFIG
    #define CONFIG_IDLE_TIME_BEFORE_SLEEP (10) /* in minutes */

    /* ============================================================== */
    /* ============================================================== */
    #if CONFIG_IDF_TARGET_ESP32C3
        /* MAGNETIC SENSOR */
        /* -------------------------------------------------------------- */
        #define CONFIG_PIN_SENSOR_POWER       (GPIO_NUM_0)
        #define CONFIG_PIN_SENSOR_SDA         (GPIO_NUM_8)
        #define CONFIG_PIN_SENSOR_SCL         (GPIO_NUM_7)
        #define CONFIG_SENSOR_NOISE_THRESHOLD (4)

        /* TOUCH/BUTTON CONFIG */
        /* -------------------------------------------------------------- */
        #define CONFIG_USE_TOUCH_BUTTON (1)
        #define CONFIG_PIN_TOUCH_SENSOR (GPIO_NUM_6)

        /* SLEEP CONFIG */
        /* -------------------------------------------------------------- */
        #define CONFIG_SHAKE_TO_WAKEUP (1)
        #define CONFIG_PIN_WAKEUP      (GPIO_NUM_5)

    #endif  // CONFIG_IDF_TARGET_ESP32C3
    /* ============================================================== */
    /* ============================================================== */
    #if CONFIG_IDF_TARGET_ESP32S3
        /* MAGNETIC SENSOR */
        /* -------------------------------------------------------------- */
        #define CONFIG_PIN_SENSOR_POWER       (GPIO_NUM_12)
        #define CONFIG_PIN_SENSOR_SDA         (GPIO_NUM_10)
        #define CONFIG_PIN_SENSOR_SCL         (GPIO_NUM_11)
        #define CONFIG_SENSOR_NOISE_THRESHOLD (4)

        /* TOUCH/BUTTON CONFIG */
        /* -------------------------------------------------------------- */
        #define CONFIG_USE_TOUCH_BUTTON  (1)
        #define CONFIG_USE_TOUCH_BUILTIN (1)
        #define CONFIG_TOUCH_TO_WAKEUP   (1)

        #if CONFIG_USE_TOUCH_BUILTIN
            #include <soc/touch_sensor_channel.h>
            #define CONFIG_TOUCH_PAD_ID (TOUCH_PAD_GPIO7_CHANNEL)
        #endif

    #endif  // CONFIG_IDF_TARGET_ESP32S3

#endif      // CONFIG_USE_MENU_CONFIG

#endif      // _USER_CONFIG_H_
