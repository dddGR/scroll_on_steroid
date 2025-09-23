#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_check.h"
#include "sdkconfig.h"

#include "user_Config.h"
#include "user_Config_priv.h"

#include "HID_bluedroid.h"
#include "esp_as5600.h"
#include "touch_Button.h"
#include "user_HID_Descriptor.h"

/* This two variables are store in RTC memory to reuse when device is go to sleep and reboot */
RTC_DATA_ATTR static volatile ScrollSpeedDiv_t g_scroll_div = SPEED_LOW;
RTC_DATA_ATTR static volatile ScrollDirection_t g_scroll_dir = SCROLL_VERTICAL;

/** AS5600 */
// static as5600_handle_t hdl_encoder;

/** BLE MOUSE */
// extern esp_hid_raw_report_map_t ble_report_maps[];
// static esp_hidd_dev_t *p_hidDevice;

/** MAIN */
static TaskHandle_t hdl_scroll_task;
/* this is mainly for printing log, can be removed if not needed */
/* ------------------------------- */
static TaskHandle_t hdl_button_task;
#if !CONFIG_USE_TOUCH_BUILTIN
static TaskHandle_t hdl_touch_task;
#endif
/* ------------------------------- */

static EventGroupHandle_t g_xEventGroup;

#if !CONFIG_USE_TOUCH_BUILTIN
void task_TouchScan(void *pvParameter)
{
    const char *TAG = "[TOUCH]";
    /**
     * @brief The external touch is connected to a GPIO pin and functions as a button. 
     * This task is responsible for listening to the input in polling mode and 
     * updating the event group bit accordingly. The "lastbits" variable is used 
     * to ensure that the bit changes only when necessary. 
     */
    ESP_LOGI(TAG, "Initializing Touch...");
    ESP_ERROR_CHECK(init_TouchExternal(TAG, CONFIG_PIN_TOUCH_SENSOR));

    EventGroupHandle_t xEventGroup = (EventGroupHandle_t)pvParameter;
    EventBits_t lastbits = xEventGroupGetBits(xEventGroup);
    EventBits_t currbits;
    TickType_t _xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        currbits = (gpio_get_level(CONFIG_PIN_TOUCH_SENSOR)) ? BUTTON_PRESSED : BUTTON_RELEASED;

        if (currbits & lastbits)
            goto end_loop;

        xEventGroupClearBits(xEventGroup, BUTTON_PRESSED | BUTTON_RELEASED);
        lastbits = xEventGroupSetBits(xEventGroup, currbits);

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
    const char *TAG = "[BUTTON]";

    for (;;)
    {
        /**
         * @brief This task listens for button presses and handles them only when 
         * the device is idle and Bluetooth is connected. It has two main functions: 
         * 
         * 1. If the user double presses the button, it changes the scroll direction 
         *    (Horizontal/Vertical).
         * 2. If the user triple presses the button, it changes the scroll speed 
         *    (High/Low).
         * 
         * These actions modify a global variable that will be utilized in the 
         * {task_MouseScroll}. Single presses are not recognized to prevent 
         * false button triggers. 
         */
        xEventGroupWaitBits(g_xEventGroup, DEVICE_IDLE | BLE_CONNECTED, pdFALSE, pdTRUE, portMAX_DELAY);

        switch (getButtonPressState(g_xEventGroup, hdl_scroll_task))
        {
        default: break;
        case B_PRESS_2:
            g_scroll_dir = (g_scroll_dir == SCROLL_HORIZONTAL) ? SCROLL_VERTICAL : SCROLL_HORIZONTAL;
            ESP_LOGI(TAG, "[double pressed] - Change scroll direction.");
            break;
        case B_PRESS_3:
            g_scroll_div = (g_scroll_div == SPEED_HIGH) ? SPEED_LOW : SPEED_HIGH;
            ESP_LOGI(TAG, "[triple pressed] - Change scroll speed to %s", (g_scroll_div == 1) ? "HIGH" : "LOW");
            break;
        case B_PRESS_NONE:
            xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE, pdFALSE, pdTRUE, portMAX_DELAY);
        }
    }
    /* safe guard */
    vTaskDelete(NULL);
}
#endif // CONFIG_USE_TOUCH_BUTTON

esp_err_t init_Hardware(const char *TAG, as5600_handle_t *hdl_encoder, esp_hidd_dev_t **p_hid_dev)
{
    ESP_LOGI(TAG, "Initializing BLE.....");
    xEventGroupSetBits(g_xEventGroup, BLE_DISCONNECTED);

    esp_hid_raw_report_map_t ble_report_maps[] = {
        {
            .data = scrollDescriptor_hiRes,
            .len = sizeof(scrollDescriptor_hiRes)
        }
    };
    
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

    ESP_ERROR_CHECK_WITHOUT_ABORT(hidDevInit(p_hid_dev, &ble_hid_config));

    ESP_LOGI(TAG, "Initializing Magnetic Sensor.....");
    gpio_config_t pin_config = {
        .pin_bit_mask = BIT(CONFIG_PIN_SENSOR_POWER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&pin_config), TAG, "failed to configure power pin..");
    ESP_RETURN_ON_ERROR(gpio_set_level(CONFIG_PIN_SENSOR_POWER, 1), TAG, "failed to turn on power..");

    vTaskDelay(pdMS_TO_TICKS(50));

    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_PIN_SENSOR_SDA,
        .scl_io_num = CONFIG_PIN_SENSOR_SCL,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000UL,
    };

    hdl_encoder->i2c_num = I2C_NUM_0;
    hdl_encoder->i2c_config = i2c_config;
    hdl_encoder->device_address = AS5600_DEFAULT_ADDRESS;

    ESP_RETURN_ON_ERROR(as5600_init(hdl_encoder), TAG, "failed to init as5600");

    /**
     * @brief If the SoC has a built-in touch sensor, it will be initialized. 
     * Otherwise, a task will be created to listen for input from the external 
     * touch sensor. The button task will utilize data from the touch sensor 
     * to execute the corresponding button actions.  
     */
#if CONFIG_USE_TOUCH_BUILTIN
    ESP_LOGI(TAG, "Initializing Internal Touch...");
    ESP_ERROR_CHECK(init_TouchBuitin(TAG, CONFIG_TOUCH_PAD_ID, g_xEventGroup));
#else
    xTaskCreatePinnedToCore(&task_TouchScan, "task_TouchScan", 2560,
                            (void *)g_xEventGroup, 6, &hdl_touch_task, (BaseType_t)APP_CORE_ID);
    vTaskDelay(pdMS_TO_TICKS(100));
#endif
#if CONFIG_USE_TOUCH_BUTTON
    xTaskCreatePinnedToCore(&task_Button, "task_Button", 3072,
                            NULL, 5, &hdl_button_task, (BaseType_t)APP_CORE_ID);
#endif // CONFIG_USE_TOUCH_BUTTON

    return ESP_OK;
}

static void task_MouseScroll(void *pvParameter)
{
    const char *TAG = "[SCROLL]";

    uint8_t counter_to_idle = 0;
    uint16_t prev_angle = 0;
    uint16_t curr_angle = 0;
    EventBits_t bits;
    as5600_handle_t hdl_encoder;
    esp_hidd_dev_t *p_hid_device;
    ESP_ERROR_CHECK(init_Hardware(TAG, &hdl_encoder, &p_hid_device));

    TickType_t xLastWakeTime = xTaskGetTickCount();

    xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE
    /* if no touch interface is used, then device is set to always active */
#if !CONFIG_USE_TOUCH_BUTTON
        | BUTTON_PRESSED
#endif
    );
    xEventGroupWaitBits(g_xEventGroup, BLE_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);

    for (;;)
    {
        /** @brief: This is the checkpoint to wait for the device to be active. 
          * If the device is in an active state (or button is pressed) but the
          * "device idle" bit is set, it indicates a transition from idle to active.
          * The next if statement will clear the idle bit while simultaneously 
          * retrieving the current angle of the encoder. This step is crucial 
          * to prevent any changes in angle caused by the magnet spinning 
          * while the device is idle. 
          * 
          * The remainder of the loop involves reading the angle, calculating 
          * the delta, and sending data to the HID device.
          */
        bits = xEventGroupWaitBits(g_xEventGroup, DEVICE_ACTIVE | BUTTON_PRESSED, pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & DEVICE_IDLE)
        {
            ESP_LOGI(TAG, "Active state...");
            as5600_get_raw_angle(&hdl_encoder, &curr_angle);
            prev_angle = curr_angle;
            xEventGroupClearBits(g_xEventGroup, DEVICE_IDLE);
            xEventGroupSetBits(g_xEventGroup, DEVICE_ACTIVE);
        }

        as5600_get_raw_angle(&hdl_encoder, &curr_angle);
        const int16_t delta_angle = (curr_angle - prev_angle/*  - curr_angle */);

        ESP_LOGD(TAG, "Current angle: %d", curr_angle);
        ESP_LOGD(TAG, "Angle delta: %d", delta_angle);

        /**
         * @brief When the delta is smaller than the set threshold, it is assumed that 
         * the device is not active. This helps to filter out noise from the encoder. 
         * The "counter_to_idle" variable counts the number of loops in which the 
         * device remains in this state. 
         * 
         * The "prev_angle" is updated at the end of the loop to account for cases 
         * where the user is scrolling at a very slow speed. Over multiple loops, 
         * the accumulated delta angle may eventually trigger scrolling and clear
         * the "counter_to_idle".
         * 
         * However, if the "counter_to_idle" reaches a certain threshold and the 
         * user is not interacting with the device, it will be set to idle state. 
         */
        if (abs(delta_angle) < CONFIG_SENSOR_NOISE_THRESHOLD)
        {
            bits = xEventGroupWaitBits(g_xEventGroup, BUTTON_PRESSED, pdFALSE, pdTRUE, 0);

            if (~bits & BUTTON_PRESSED && ++counter_to_idle > 50 /* about 500ms with 100hz polling rate */)
            {
                ESP_LOGI(TAG, "Entering idle state...");
                xEventGroupClearBits(g_xEventGroup, DEVICE_ACTIVE);
                xEventGroupSetBits(g_xEventGroup, DEVICE_IDLE);
                counter_to_idle = 0;
            }
            goto end_loop;
        }
        else if (counter_to_idle > 0) counter_to_idle = 0;


        if (abs(delta_angle) < 2048) /* ignore when angle loop back from 4095 to 0 (12-bit) */
        {
            int8_t scroll_val = MIN(MAX((delta_angle/(int8_t)g_scroll_div), -127), 127);
            switch (g_scroll_dir)
            {
            default: break;
            case SCROLL_VERTICAL:
                sendScroll(p_hid_device, -scroll_val, 0);
                break;
            case SCROLL_HORIZONTAL:
                sendScroll(p_hid_device, 0, scroll_val);
                break;
            }
        }

        prev_angle = curr_angle;

    end_loop:
        /* cap at about ~100hz polling rate */
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

/* BLE HID CALLBACK */
/* -------------------------------------------------------------------- */
void do_when_ble_hid_connect(void)
{
    while (!hdl_scroll_task)
        vTaskDelay(pdMS_TO_TICKS(100));

    xEventGroupClearBits(g_xEventGroup, BLE_CONNECTED | BLE_DISCONNECTED);
    xEventGroupSetBits(g_xEventGroup, BLE_CONNECTED);
    vTaskResume(hdl_scroll_task);
}

void do_when_ble_hid_disconnect(void)
{
    if (!hdl_scroll_task)
        goto end;

    vTaskSuspend(hdl_scroll_task);
end:
    xEventGroupClearBits(g_xEventGroup, BLE_CONNECTED | BLE_DISCONNECTED);
    xEventGroupSetBits(g_xEventGroup, BLE_DISCONNECTED);
}

void do_when_ble_hid_stop(void)
{
    /* nothing for now */
}
/* -------------------------------------------------------------------- */

static void esp_pre_sleep_config(const char *TAG)
{
#if CONFIG_TOUCH_TO_WAKEUP
    ESP_ERROR_CHECK_WITHOUT_ABORT(regTouchWakeup(TAG));
#endif

#if CONFIG_SHAKE_TO_WAKEUP
    const gpio_config_t config = {
        .pin_bit_mask = BIT(CONFIG_PIN_WAKEUP),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&config));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_deep_sleep_enable_gpio_wakeup(BIT64(CONFIG_PIN_WAKEUP), ESP_GPIO_WAKEUP_GPIO_HIGH));
#endif

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(CONFIG_PIN_SENSOR_POWER, 0));

    vTaskDelay(pdMS_TO_TICKS(100));
}

/* MAIN */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void app_main(void)
{
    const char *MTAG = "[MAIN]";

#if CONFIG_SHAKE_TO_WAKEUP
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_reset_pin(CONFIG_PIN_WAKEUP));
#endif
    g_xEventGroup = xEventGroupCreate();
    /**
     * @brief The {task_MouseScroll} is responsible for reading the encoder and
     * sending the scroll data to the HID device via the Bluetooth interface. This
     * task also determines the idle and active states of the device. 
     */
    xTaskCreatePinnedToCore(&task_MouseScroll, "task_MouseScroll", 3584,
                            NULL, 5, &hdl_scroll_task, (BaseType_t)APP_CORE_ID);

    /* wait 5 seconds for everything to be ready */
    vTaskDelay(pdMS_TO_TICKS(1000 * 5));

    /** MAIN also control sleep function */
    EventBits_t bits;
    for (;;)
    {
#if CONFIG_USE_TOUCH_BUTTON
        /**
         * @note The main strategy is to wait for the device to enter an idle state, 
         * after which one of two scenarios will occur: 
         * 
         * 1. If BLE is not connected, the device will wait for 30 seconds before 
         *    going to sleep (if BLE remains disconnected).
         * 2. If BLE is connected, the device will wait for 10 minutes (this duration 
         *    can be adjusted in config.h or menuconfig). If the device does not exit 
         *    the idle state during this time, it will go to sleep; otherwise, the 
         *    loop will continue. 
         */
        bits = xEventGroupWaitBits(g_xEventGroup,
                                    DEVICE_IDLE,
                                    pdFALSE,
                                    pdTRUE,
                                    portMAX_DELAY);

        bits = xEventGroupWaitBits(g_xEventGroup,
                                    DEVICE_ACTIVE | BLE_DISCONNECTED,
                                    pdFALSE,
                                    pdFALSE,
                                    pdMS_TO_TICKS(1000 * 60 * CONFIG_IDLE_TIME_BEFORE_SLEEP));
        
        if (bits & DEVICE_ACTIVE)
            continue;

        if (bits & BLE_DISCONNECTED)
        {
            ESP_LOGW(MTAG, "BLE not connected, wait 30 seconds until go to sleep.");
            bits = xEventGroupWaitBits(g_xEventGroup,
                                        DEVICE_ACTIVE | BLE_CONNECTED,
                                        pdFALSE,
                                        pdFALSE,
                                        pdMS_TO_TICKS(1000 * 30));

            if (bits & BLE_CONNECTED || bits & DEVICE_ACTIVE)
                continue;
        }
#else
        /**
         * @note If a touch sensor is not used, the device is set to remain always active. 
         * In this case, the only condition for determining when to go to sleep is 
         * checking for BLE disconnection. This is maybe not the best strategy. 
         */
        bits = xEventGroupWaitBits(g_xEventGroup,
                                    BLE_DISCONNECTED,
                                    pdFALSE,
                                    pdTRUE,
                                    portMAX_DELAY);
        
        ESP_LOGW(MTAG, "BLE not connected, wait 30 seconds until go to sleep.");
        bits = xEventGroupWaitBits(g_xEventGroup,
                                    BLE_CONNECTED,
                                    pdFALSE,
                                    pdTRUE,
                                    pdMS_TO_TICKS(1000 * 30));

        if (bits & BLE_CONNECTED) continue;
#endif
        ESP_LOGW(MTAG, "Entering sleep mode.....");
        esp_pre_sleep_config(MTAG);
        
        /* Some log for testing */
        ESP_LOGW(MTAG, "high watermark %d",  uxTaskGetStackHighWaterMark(NULL));
        ESP_LOGW("[SCROLL]", "high watermark %d",  uxTaskGetStackHighWaterMark(hdl_scroll_task));
#if CONFIG_USE_TOUCH_BUTTON
#if !CONFIG_USE_TOUCH_BUILTIN
        ESP_LOGW("[TOUCH]", "high watermark %d",  uxTaskGetStackHighWaterMark(hdl_touch_task));
#endif
        ESP_LOGW("[BUTTON]", "high watermark %d",  uxTaskGetStackHighWaterMark(hdl_button_task));
#endif

        uart_wait_tx_idle_polling((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM);
        esp_deep_sleep_start();
    }
}

