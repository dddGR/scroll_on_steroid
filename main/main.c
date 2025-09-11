#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_check.h>

#include "sdkconfig.h"
#include "user_Types.h"
#include "user_Config.h"
#include "HID_bluedroid.h"
#include "esp_as5600.h"
#include "touch_Button.h"


#if CONFIG_FREERTOS_UNICORE
const BaseType_t app_core = 0;
#else
const BaseType_t app_core = 1;
#endif

RTC_DATA_ATTR static volatile scrollSpeedDiv g_scroll_div = SPEED_LOW;
RTC_DATA_ATTR static volatile scrollDirection g_scroll_dir = SCROLL_VERTICAL;

/** AS5600 */
static as5600_handle_t hdl_encoder;

/** BLE MOUSE */
extern esp_hid_raw_report_map_t ble_report_maps[];
static esp_hidd_dev_t *p_hidDevice;

/** MAIN */
static TaskHandle_t hdl_scroll_task;
static EventGroupHandle_t g_xEventGroup;


static esp_err_t init_MouseScroll(const char *_TAG)
{
    ESP_LOGI(_TAG, "Initializing BLE.....");
    xEventGroupSetBits(g_xEventGroup, BLE_DISCONNECTED);
    
    const esp_hid_device_config_t ble_hid_config = {
        .vendor_id          = 0x16C0,
        .product_id         = 0x05DF,
        .version            = 0x0100,
        .device_name        = DEVICE_NAME,
        .manufacturer_name  = DEVICE_MANUFACTURER,
        .serial_number      = DEVICE_SERIAL,
        .report_maps        = ble_report_maps,
        .report_maps_len    = 1
    };

    ESP_ERROR_CHECK_WITHOUT_ABORT(hidDevInit(&p_hidDevice, &ble_hid_config));

    ESP_LOGI(_TAG, "Initializing Magnetic Sensor.....");
    gpio_config_t pin_config = {
        .pin_bit_mask = BIT(ss_power_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&pin_config), _TAG, "failed to configure power pin..");
    ESP_RETURN_ON_ERROR(gpio_set_level(ss_power_pin, 1), _TAG, "failed to turn on power..");

    vTaskDelay(pdMS_TO_TICKS(50));

    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_sda,
        .scl_io_num = i2c_scl,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000,
    };

    hdl_encoder.i2c_num = I2C_NUM_0;
    hdl_encoder.i2c_config = i2c_config;
    hdl_encoder.device_address = AS5600_DEFAULT_ADDRESS;

    ESP_RETURN_ON_ERROR(as5600_init(&hdl_encoder), _TAG, "failed to init as5600");

    return ESP_OK;
}


#if !CONFIG_USE_TOUCH_BUILTIN
void task_TouchScan(void *pvParameter)
{
    const char *_TAG = "TOUCH";
    ESP_LOGI(_TAG, "Initializing Touch...");
    ESP_ERROR_CHECK(init_TouchExternal(_TAG, pin_touch_sensor));

    EventGroupHandle_t xEventGroup = (EventGroupHandle_t)pvParameter;
    EventBits_t last_bits = xEventGroupGetBits(xEventGroup);
    EventBits_t curr_bits;
    TickType_t _xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        curr_bits = (gpio_get_level(pin_touch_sensor)) ? BUTTON_PRESSED : BUTTON_RELEASED;

        if (curr_bits & last_bits)
            goto end_loop;

        xEventGroupClearBits(xEventGroup, BUTTON_PRESSED | BUTTON_RELEASED);
        last_bits = xEventGroupSetBits(xEventGroup, curr_bits);

    end_loop:
        /* ~100hz */
        xTaskDelayUntil(&_xLastWakeTime, pdMS_TO_TICKS(10));
    }

    /* safe guard */
    vTaskDelete(NULL);
}
#endif

#if CONFIG_USE_TOUCH_BUTTON
static void task_Button(void *pvParameter)
{
    const char *_TAG = "BUTTON";

    for (;;)
    {
        xEventGroupWaitBits(g_xEventGroup, DEVICE_IDLE | BLE_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);

        switch (getButtonPressState(g_xEventGroup, hdl_scroll_task))
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
            xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE, pdFALSE, pdTRUE, portMAX_DELAY);
        }
    }
    /* safe guard */
    vTaskDelete(NULL);
}
#endif // CONFIG_USE_TOUCH_BUTTON

static void task_MouseScroll(void *pvParameter)
{
    const char *_TAG = "SCROLL";

    uint8_t counter_to_idle = 0;
    uint16_t prev_angle = 0;
    uint16_t curr_angle = 0;
    EventBits_t _bits;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE
#if !CONFIG_USE_TOUCH_BUTTON /* If no touch use, device is always active */
        | BUTTON_PRESSED
#endif
    );
    xEventGroupWaitBits(g_xEventGroup, BLE_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);

    for (;;)
    {
        _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BUTTON_PRESSED, pdFALSE, pdFALSE, portMAX_DELAY);
        if (_bits & DEVICE_IDLE)
        {
            ESP_LOGI(_TAG, "Active state...");
            as5600_get_raw_angle(&hdl_encoder, &curr_angle);
            prev_angle = curr_angle;
            xEventGroupClearBits(g_xEventGroup, DEVICE_IDLE);
            xEventGroupSetBits(g_xEventGroup, DEVICE_ACTIVE);
        }

        as5600_get_raw_angle(&hdl_encoder, &curr_angle);
        const int16_t delta_angle = (curr_angle - prev_angle/*  - curr_angle */);

        ESP_LOGD(_TAG, "Current angle: %d", curr_angle);
        ESP_LOGD(_TAG, "Angle delta: %d", delta_angle);

        /** 
         *  @note: when delta is too small, assuming not spin and begin check for
         *  idle state. Incase of not spining but still touching, skip check.
         *  Only here can set idle state to true, this is to avoid false idle
         *  when scroll is still spinning.
        */
        if (abs(delta_angle) < sensor_cuttoff_threshold)
        {
            #if 0
            _bits = xEventGroupWaitBits(g_xEventGroup, BUTTON_PRESSED, pdFALSE, pdFALSE, pdMS_TO_TICKS(200));

            if (~_bits & BUTTON_PRESSED)
            {
                ESP_LOGI(_TAG, "Entering idle state...");
                xEventGroupClearBits(g_xEventGroup, DEVICE_ACTIVE);
                xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE);
            }
            #endif

            _bits = xEventGroupWaitBits(g_xEventGroup, BUTTON_PRESSED, pdFALSE, pdTRUE, 0);

            if (~_bits & BUTTON_PRESSED && ++counter_to_idle > 50 /* about 500ms with 100hz polling rate */)
            {
                ESP_LOGI(_TAG, "Entering idle state...");
                xEventGroupClearBits(g_xEventGroup, DEVICE_ACTIVE);
                xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE);
                counter_to_idle = 0;
            }
            goto end_loop;
        }
        else if (counter_to_idle > 0) counter_to_idle = 0;


        if (abs(delta_angle) < 2048) // ignore when angle loop back from 4095 to 0 (12-bit)
        {
            int8_t scroll_val = MIN(MAX((delta_angle/(int8_t)g_scroll_div), -127), 127);
            switch (g_scroll_dir)
            {
            default: break;
            case SCROLL_VERTICAL:
                sendScroll(p_hidDevice, -scroll_val, 0);
                break;
            case SCROLL_HORIZONTAL:
                sendScroll(p_hidDevice, 0, scroll_val);
                break;
            }
        }

        prev_angle = curr_angle;

    end_loop:
        // cap at about ~100hz polling rate
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}


/* BLE HID CALLBACK */
void do_when_ble_hid_connect()
{
    while (!hdl_scroll_task)
        vTaskDelay(pdMS_TO_TICKS(100));

    xEventGroupClearBits(g_xEventGroup, BLE_CONNECTED | BLE_DISCONNECTED);
    xEventGroupSetBits(g_xEventGroup, BLE_CONNECTED);
    vTaskResume(hdl_scroll_task);
}

void do_when_ble_hid_disconnect()
{
    if (!hdl_scroll_task)
        goto end;

    vTaskSuspend(hdl_scroll_task);
end:
    xEventGroupClearBits(g_xEventGroup, BLE_CONNECTED | BLE_DISCONNECTED);
    xEventGroupSetBits(g_xEventGroup, BLE_DISCONNECTED);
}

void do_when_ble_hid_stop()
{
    // nothing for now
}

static void esp_pre_sleep_config(const char *_TAG)
{
#if CONFIG_TOUCH_TO_WAKEUP
    ESP_ERROR_CHECK_WITHOUT_ABORT(regTouchWakeup(_TAG));
#endif

#if CONFIG_SHAKE_TO_WAKEUP
    const gpio_config_t config = {
        .pin_bit_mask = BIT(pin_to_wakeup),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&config));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_deep_sleep_enable_gpio_wakeup(BIT(pin_to_wakeup), ESP_GPIO_WAKEUP_GPIO_HIGH));
#endif

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(ss_power_pin, 0));

    vTaskDelay(pdMS_TO_TICKS(100));
}

//===================================================================================//
//===================================================================================//

void app_main()
{
    const char *M_TAG = "MAIN";

#if CONFIG_SHAKE_TO_WAKEUP
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_reset_pin(pin_to_wakeup));
#endif

    g_xEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK(init_MouseScroll(M_TAG));

#if CONFIG_USE_TOUCH_BUTTON

#if CONFIG_USE_TOUCH_BUILTIN
    ESP_LOGI(M_TAG, "Initializing Internal Touch...");
    ESP_ERROR_CHECK(init_TouchBuitin(M_TAG, touch_channel_id, g_xEventGroup));
#else
    TaskHandle_t hdl_touch_task;
    xTaskCreatePinnedToCore(&task_TouchScan, "task_TouchScan", 2560,
                            (void *)g_xEventGroup, 6, &hdl_touch_task, app_core);
    
    vTaskDelay(pdMS_TO_TICKS(100));
#endif

    TaskHandle_t hdl_button_task;
    xTaskCreatePinnedToCore(&task_Button, "task_Button", 3072,
                            NULL, 5, &hdl_button_task, app_core);
#endif // CONFIG_USE_TOUCH_BUTTON

    xTaskCreatePinnedToCore(&task_MouseScroll, "task_MouseScroll", 3584,
                            NULL, 5, &hdl_scroll_task, app_core);

    vTaskDelay(pdMS_TO_TICKS(1000 * 5)); // wait 5 seconds for everything to be ready

    /** MAIN also control sleep function */
    EventBits_t _bits;
    for (;;)
    {
#if CONFIG_USE_TOUCH_BUTTON
        _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_IDLE, pdFALSE, pdTRUE, portMAX_DELAY);

        _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BLE_DISCONNECTED, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 60 * idle_time_before_sleep));
        
        if (_bits & DEVICE_ACTIVE)
            continue;

        if (_bits & BLE_DISCONNECTED)
        {
            ESP_LOGW(M_TAG, "BLE not connected, wait 30 seconds until go to sleep.");
            _bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BLE_CONNECTED, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 30));

            if (_bits & BLE_CONNECTED || _bits & DEVICE_ACTIVE)
                continue;
        }
#else /* Device is always active when touch is not used, so only time is going to sleep is when BLE is disconnected */
        _bits = xEventGroupWaitBits(g_xEventGroup, BLE_DISCONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);
        
        ESP_LOGW(M_TAG, "BLE not connected, wait 30 seconds until go to sleep.");
        _bits = xEventGroupWaitBits(g_xEventGroup, BLE_CONNECTED, pdFALSE, pdTRUE, pdMS_TO_TICKS(1000 * 30));

        if (_bits & BLE_CONNECTED) continue;
#endif

        ESP_LOGW(M_TAG, "Entering sleep mode.....");
        esp_pre_sleep_config(M_TAG);
        
        /* Some log for testing */
        ESP_LOGW(M_TAG, "high watermark %d",  uxTaskGetStackHighWaterMark(NULL));
        ESP_LOGW("SCROLL", "high watermark %d",  uxTaskGetStackHighWaterMark(hdl_scroll_task));
#if CONFIG_USE_TOUCH_BUTTON
#if !CONFIG_USE_TOUCH_BUILTIN
        ESP_LOGW("TOUCH", "high watermark %d",  uxTaskGetStackHighWaterMark(hdl_touch_task));
#endif
        ESP_LOGW("BUTTON", "high watermark %d",  uxTaskGetStackHighWaterMark(hdl_button_task));
#endif

        uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);
        esp_deep_sleep_start();
    }
}

