#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include "soc/gpio_num.h"

/* SLEEP TIMMER */
#ifndef CONFIG_IDLE_TIME_BEFORE_SLEEP
#define CONFIG_IDLE_TIME_BEFORE_SLEEP 10 // in minutes
#endif

static const int idle_time_before_sleep = CONFIG_IDLE_TIME_BEFORE_SLEEP;

#if CONFIG_USE_MENU_CONFIG
/** BLE DEVICE */
#define DEVICE_NAME             CONFIG_DEVICE_NAME
#define DEVICE_MANUFACTURER     "DDDGR"
#define DEVICE_SERIAL           "1234567890"

/** AS5600 */
static const gpio_num_t ss_power_pin = (gpio_num_t)CONFIG_SENSOR_POWER_ENABLE_PIN;
static const gpio_num_t i2c_sda      = (gpio_num_t)CONFIG_SENSOR_I2C_SDA_PIN;
static const gpio_num_t i2c_scl      = (gpio_num_t)CONFIG_SENSOR_I2C_SCL_PIN;
static const uint8_t sensor_cuttoff_threshold = (uint8_t)CONFIG_SENSOR_NOISE_CUTOFF_THRESHOLD;

/** TOUCH CONFIG */
#if CONFIG_USE_TOUCH_BUILTIN
static const int touch_channel_id = CONFIG_TOUCH_PAD;
#else
static const gpio_num_t pin_touch_sensor = (gpio_num_t)CONFIG_TOUCH_SENSOR_GPIO_PIN;

#if CONFIG_SHAKE_TO_WAKEUP
static const gpio_num_t pin_to_wakeup = (gpio_num_t)CONFIG_GPIO_WAKEUP_PIN;
#endif

#endif // CONFIG_USE_TOUCH_BUILTIN

#else
#if CONFIG_IDF_TARGET_ESP32C3
/** BLE DEVICE */
#define DEVICE_NAME             "Scroll On Steroid lite"
#define DEVICE_MANUFACTURER     "DDDGR"
#define DEVICE_SERIAL           "1234567890"

/** AS5600 */
static const gpio_num_t ss_power_pin = GPIO_NUM_0;
static const gpio_num_t i2c_sda = GPIO_NUM_8;
static const gpio_num_t i2c_scl = GPIO_NUM_7;
static const uint8_t sensor_cuttoff_threshold = 4U;

/** BUTTON CONFIG */
#define CONFIG_USE_TOUCH_BUTTON 1

/** SLEEP CONFIG */
#define CONFIG_SHAKE_TO_WAKEUP 1

#if CONFIG_SHAKE_TO_WAKEUP
static const gpio_num_t pin_to_wakeup = GPIO_NUM_5;
#endif
#endif // CONFIG_IDF_TARGET_ESP32C3

#if CONFIG_IDF_TARGET_ESP32S3
/** BLE DEVICE */
#define DEVICE_NAME             "Scroll On Steroid"
#define DEVICE_MANUFACTURER     "DDDGR"
#define DEVICE_SERIAL           "1234567890"

/** AS5600 */
static const gpio_num_t ss_power_pin = GPIO_NUM_12;
static const gpio_num_t i2c_sda = GPIO_NUM_10;
static const gpio_num_t i2c_scl = GPIO_NUM_11;
static const uint8_t sensor_cuttoff_threshold = 4U;

/** TOUCH/BUTTON CONFIG */
#define CONFIG_USE_TOUCH_BUTTON 1
#define CONFIG_USE_TOUCH_BUILTIN 1
#define CONFIG_TOUCH_TO_WAKEUP 1

#if CONFIG_USE_TOUCH_BUILTIN
#include <soc/touch_sensor_channel.h>
static const int touch_channel_id = TOUCH_PAD_GPIO7_CHANNEL;
#endif
#endif // CONFIG_IDF_TARGET_ESP32S3

#endif // CONFIG_USE_MENU_CONFIG

#endif // _USER_CONFIG_H_