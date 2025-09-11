#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// #include <inttypes.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
// #include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_check.h>

#include "sdkconfig.h"

#include "Arduino.h"
#include "BleMouse.h"
#include "AS5600.h"

#include "custom_types.h"
#include "touch_button.h"


#if CONFIG_FREERTOS_UNICORE
const BaseType_t app_core = 0;
#else
const BaseType_t app_core = 1;
#endif

RTC_DATA_ATTR static volatile scrollSpeedDiv g_scroll_div = SPEED_LOW;
RTC_DATA_ATTR static volatile scrollDirection g_scroll_dir = SCROLL_VERTICAL;

static AS5600 magSensor;
static BleMouse bleMouse(CONFIG_DEVICE_NAME); // Name of the device can be change here or use menuconfig (recommended)

static TaskHandle_t scroll_task_hdl;
EventGroupHandle_t g_xEventGroup = xEventGroupCreate();

//===================================================================================//
//===================================================================================//

static esp_err_t initMouseScroll(const char *_TAG, const gpio_num_t _pow, const gpio_num_t _sda, const gpio_num_t _scl)
{
    ESP_LOGI(_TAG, "Initializing BLE.....");
    xEventGroupSetBits(g_xEventGroup, BLE_DISCONNECTED);
    bleMouse.begin();

    ESP_LOGI(_TAG, "Initializing Magnetic Sensor.....");
    gpio_config_t config = {
        .pin_bit_mask = BIT(_pow),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_RETURN_ON_ERROR(gpio_config(&config), _TAG, "Failed to configure power pin..");
    ESP_RETURN_ON_ERROR(gpio_set_level(_pow, HIGH), _TAG, "Failed turn on power..");

    vTaskDelay(pdMS_TO_TICKS(100));

    Wire.begin(_sda, _scl);
    return (esp_err_t)!magSensor.begin();
}

static void task_MouseScroll(void *pvParameter)
{
    const char *_TAG = "MOUSE";

    ESP_LOGI(_TAG, "Initializing Mouse Scroll...");
    ESP_ERROR_CHECK(initMouseScroll(_TAG,
                        (gpio_num_t)CONFIG_SENSOR_POWER_ENABLE_PIN,
                        (gpio_num_t)CONFIG_SENSOR_I2C_SDA_PIN,
                        (gpio_num_t)CONFIG_SENSOR_I2C_SCL_PIN));

    vTaskDelay(pdMS_TO_TICKS(200));

    uint16_t prev_angle = 0;
    EventBits_t _bits;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE);

    for (;;)
    {
        _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BUTTON_PRESSED, pdFALSE, pdFALSE, portMAX_DELAY);
        if (_bits & DEVICE_IDLE)
        {
            ESP_LOGI(_TAG, "Active state...");
            prev_angle = magSensor.readAngle();
            xEventGroupClearBits(g_xEventGroup, DEVICE_IDLE);
            xEventGroupSetBits(g_xEventGroup, DEVICE_ACTIVE);
        }

        uint16_t curr_angle = magSensor.readAngle();
        const int16_t delta_angle = (curr_angle - prev_angle/*  - curr_angle */);

        ESP_LOGD(_TAG, "Current angle: %d", curr_angle);
        ESP_LOGD(_TAG, "Angle delta: %d", delta_angle);

        /** 
         *  @note: when delta is too small, assuming not spin and begin check for
         *  idle state. Incase of not spining but still touching, skip check.
         *  Only here can set idle state to true, this is to avoid false idle
         *  when scroll still spinning.
        */
        if (abs(delta_angle) < CONFIG_SENSOR_NOISE_CUTOFF_THRESHOLD)
        {
            _bits = xEventGroupWaitBits(g_xEventGroup, BUTTON_PRESSED, pdFALSE, pdFALSE, pdMS_TO_TICKS(200));

            if (~_bits & BUTTON_PRESSED)
            {
                ESP_LOGI(_TAG, "Entering idle state...");
                xEventGroupClearBits(g_xEventGroup, DEVICE_ACTIVE);
                xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE);
            }
            
            goto end_loop;
        }

        if (abs(delta_angle) < 2048) // ignore when angle loop back from 4095 to 0 (12-bit)
        {
            int8_t scroll_val = MIN(MAX((delta_angle/(int8_t)g_scroll_div), -127), 127);
            switch (g_scroll_dir)
            {
            default: break;
            case SCROLL_VERTICAL:
                bleMouse.scroll(-scroll_val, 0);
                break;
            case SCROLL_HORIZONTAL:
                bleMouse.scroll(0, scroll_val);
                break;
            }
        }

        prev_angle = curr_angle;

    end_loop:
        // ~60hz
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(16));
    }
}

#if CONFIG_USE_BUTTON_FEATURE
static void task_Button(void *pvParameter)
{
#if CONFIG_USE_TOUCH_BUILTIN
    ESP_LOGI("TOUCH", "Initializing Touch...");
    ESP_ERROR_CHECK(initTouch(_TAG));
#endif
    const char *_TAG = "BUTTON";

    for (;;)
    {
        xEventGroupWaitBits(g_xEventGroup, DEVICE_IDLE, pdFALSE, pdFALSE, portMAX_DELAY);

        switch (getButtonPressState(g_xEventGroup, scroll_task_hdl))
        {
        default: break;
        case B_PRESS_2:
            g_scroll_dir = (g_scroll_dir == SCROLL_HORIZONTAL) ? SCROLL_VERTICAL : SCROLL_HORIZONTAL;
            ESP_LOGI(_TAG, "[double pressed] - Change scroll direction.");
            break;
        case B_PRESS_3:
            g_scroll_div = (g_scroll_div == SPEED_HIGH) ? SPEED_LOW : SPEED_HIGH;
            ESP_LOGI(_TAG, "[triple pressed] - Change scroll speed to %s", (g_scroll_div == 1) ? "HIGH" : "LOW");
            break;
        case B_PRESS_NONE:
            xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE, pdFALSE, pdFALSE, portMAX_DELAY);
        }
    }

    /* safe guard */
    vTaskDelete(NULL);
}
#endif // CONFIG_USE_BUTTON_FEATURE

static void esp_pre_sleep_config(const char *_TAG)
{
#if CONFIG_TOUCH_TO_WAKEUP
#ifdef CONFIG_USE_TOUCH_BUILTIN
    // ESP_ERROR_CHECK(touch_pad_sleep_channel_enable((touch_pad_t)CONFIG_TOUCH_PAD, true));
    // ESP_ERROR_CHECK(touch_pad_sleep_set_threshold((touch_pad_t)CONFIG_TOUCH_PAD, CONFIG_TOUCH_THRESHOLD));

    // ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());

    ESP_ERROR_CHECK(regTouchWakeup(_TAG));
#else
    /** Sensor disabled when sleep -> no wakeup,
     *  maybe config to use vibration switch later */
    /* const gpio_config_t config = {
        .pin_bit_mask = BIT(CONFIG_TOUCH_SENSOR_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(BIT(CONFIG_TOUCH_SENSOR_GPIO_PIN), ESP_GPIO_WAKEUP_GPIO_HIGH));

    ESP_LOGI(_TAG, "Enabling GPIO wakeup on pins GPIO%d\n", CONFIG_TOUCH_SENSOR_GPIO_PIN); */
#endif // USE_TOUCH_BUILTIN
#endif // CONFIG_TOUCH_TO_WAKEUP

    bleMouse.end();
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)CONFIG_SENSOR_POWER_ENABLE_PIN, LOW));

    vTaskDelay(pdMS_TO_TICKS(100));
}

//===================================================================================//
//===================================================================================//

extern "C" void app_main()
{
    const char *M_TAG = "MAIN";
    ESP_LOGI(M_TAG, "Begin setup.....");
    initArduino();

#if !CONFIG_USE_TOUCH_BUILTIN
    TaskHandle_t touch_task_hdl;
    xTaskCreatePinnedToCore(&task_TouchScan, "task_TouchScan", 2560,
                            (void *)g_xEventGroup, 6, &touch_task_hdl, app_core);
    
    vTaskDelay(pdMS_TO_TICKS(100));
#endif

    xTaskCreatePinnedToCore(&task_MouseScroll, "task_MouseScroll", 3584,
                            NULL, 5, &scroll_task_hdl, app_core);

#if CONFIG_USE_BUTTON_FEATURE
    TaskHandle_t button_task_hdl;
    xTaskCreatePinnedToCore(&task_Button, "task_Button", 3072,
                            NULL, 5, &button_task_hdl, app_core);
#endif

    vTaskDelay(pdMS_TO_TICKS(1000 * 5)); // wait 5 seconds for everything to be ready

    /** MAIN also control sleep function */
    EventBits_t _bits;
    for (;;)
    {
        _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_IDLE, pdFALSE, pdFALSE, portMAX_DELAY);

        // if (xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 60 * 1)) & DEVICE_ACTIVE) // for testing
        _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BLE_DISCONNECTED, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 60 * CONFIG_IDLE_TIME_BEFORE_SLEEP));
        
        if (_bits & DEVICE_ACTIVE)
            continue;

        if (_bits & BLE_DISCONNECTED)
        {
            ESP_LOGW(M_TAG, "BLE not connected, wait 30 seconds until go to sleep.");
            _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BLE_CONNECTED, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 30));

            if (_bits & BLE_CONNECTED || _bits & DEVICE_ACTIVE)
                continue;
        }

        ESP_LOGW(M_TAG, "Entering sleep mode.....");
        esp_pre_sleep_config(M_TAG);
        
        /* Some log for testing */
        ESP_LOGW(M_TAG, "high watermark %d",  uxTaskGetStackHighWaterMark(NULL));
        ESP_LOGW("SCROLL", "high watermark %d",  uxTaskGetStackHighWaterMark(scroll_task_hdl));
#if !CONFIG_USE_TOUCH_BUILTIN
        ESP_LOGW("TOUCH", "high watermark %d",  uxTaskGetStackHighWaterMark(touch_task_hdl));
#endif
#if CONFIG_USE_BUTTON_FEATURE
        ESP_LOGW("BUTTON", "high watermark %d",  uxTaskGetStackHighWaterMark(button_task_hdl));
#endif

        uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);
        esp_deep_sleep_start();
    }
}
