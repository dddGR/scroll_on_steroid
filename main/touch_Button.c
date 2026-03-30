#include "touch_Button.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: export
#include "freertos/task.h"

static const char* TAG = "[TOUCH]";

#define TIME_WAIT           (200UL)  /* in milliseconds */
#define TIME_WAIT_FOR_INPUT (2000UL) /* in milliseconds */

ButtonPressState_t getButtonPressState(EventGroupHandle_t xEventGroup,
                                       TaskHandle_t xTask) {
    ButtonPressState_t button_state = B_PRESS_NONE;
    EventBits_t cur_bits;

    ESP_LOGI(TAG, "%s() waiting for input...", __func__);

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_PRESSED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT_FOR_INPUT));
    if ( ~cur_bits & BUTTON_PRESSED ) {
        ESP_LOGI(TAG, "%s() no input, exit", __func__);
        goto end;
    }

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_RELEASED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT));
    if ( ~cur_bits & BUTTON_RELEASED ) {  // not released, pressed and hold
        goto end;
    }

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_PRESSED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT));
    if ( ~cur_bits & BUTTON_PRESSED ) {  // not pressed, -> not button input
        goto end;
    }

    vTaskSuspend(xTask);

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_RELEASED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT));
    if ( cur_bits & BUTTON_RELEASED ) {  // released, double press
        button_state = B_PRESS_2;
    } else {
        goto end;
    }

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_PRESSED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT));
    if ( ~cur_bits
         & BUTTON_PRESSED ) {  // not pressed again -> just double press
        goto end;
    }

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_RELEASED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT));
    if ( cur_bits & BUTTON_RELEASED ) {  // released, triple press
        button_state = B_PRESS_3;
    } else  // pressed again, but not released -> false press -> ignore
    {
        button_state = B_PRESS_HOLD;
        goto end;
    }

    cur_bits = xEventGroupWaitBits(xEventGroup,
                                   BUTTON_PRESSED,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(TIME_WAIT));
    if ( cur_bits & BUTTON_PRESSED ) {
        ESP_LOGI(TAG, "%s() too many press, ignore", __func__);
        button_state = B_PRESS_HOLD;
    }

end:
    vTaskResume(xTask);
    return button_state;
}

#if SOC_TOUCH_SENSOR_SUPPORTED && CONFIG_USE_TOUCH_BUILTIN
    #include <driver/touch_sens.h>
    #include <inttypes.h>
    #include <soc/touch_sensor_channel.h>

    #define TOUCH_SAMPLE_NUM           (TOUCH_SAMPLE_CFG_NUM)
    #define TOUCH_CHAN_INIT_SCAN_TIMES (10)

    #define TOUCH_CHAN_CFG_DEFAULT()                            \
        {                                                       \
            .active_thresh    = {30000},                        \
            .charge_speed     = TOUCH_CHARGE_SPEED_7,           \
            .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT, \
        }

/** Handles of touch sensor */
static touch_sensor_handle_t sens_handle  = NULL;
static touch_channel_handle_t chan_handle = NULL;
static float s_thresh2bm_ratio            = 0.015f;

bool touch_on_active_callback(touch_sensor_handle_t sens_handle,
                              const touch_active_event_data_t* event,
                              void* user_ctx) {
    EventGroupHandle_t xEventGroup = (EventGroupHandle_t)user_ctx;

    xEventGroupClearBits(xEventGroup, BUTTON_PRESSED | BUTTON_RELEASED);
    xEventGroupSetBits(xEventGroup, BUTTON_PRESSED);

    return ESP_OK;
}

bool touch_on_inactive_callback(touch_sensor_handle_t sens_handle,
                                const touch_inactive_event_data_t* event,
                                void* user_ctx) {
    EventGroupHandle_t xEventGroup = (EventGroupHandle_t)user_ctx;

    xEventGroupClearBits(xEventGroup, BUTTON_PRESSED | BUTTON_RELEASED);
    xEventGroupSetBits(xEventGroup, BUTTON_RELEASED);

    return ESP_OK;
}

void touch_do_initial_scanning(const char* _TAG,
                               touch_sensor_handle_t sens_handle,
                               touch_channel_handle_t chan_handle) {
    /* Enable the touch sensor to do the initial scanning, so that to initialize
     * the channel data */
    ESP_ERROR_CHECK(touch_sensor_enable(sens_handle));

    /* Scan the enabled touch channels for several times, to make sure the initial
     * channel data is stable */
    for ( int i = 0; i < TOUCH_CHAN_INIT_SCAN_TIMES; i++ ) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            touch_sensor_trigger_oneshot_scanning(sens_handle, 2000));
    }

    /* Disable the touch channel to rollback the state */
    ESP_ERROR_CHECK(touch_sensor_disable(sens_handle));

    /* (Optional) Read the initial channel benchmark and reconfig the channel
     * active threshold accordingly */
    ESP_LOGI(_TAG, "Initial benchmark and new threshold are:");

    /* Read the initial benchmark of the touch channel */
    uint32_t benchmark[TOUCH_SAMPLE_NUM] = {};
    #if SOC_TOUCH_SUPPORT_BENCHMARK
    ESP_LOGI(_TAG, "Benchmark Type...");
    ESP_ERROR_CHECK(touch_channel_read_data(chan_handle,
                                            TOUCH_CHAN_DATA_TYPE_BENCHMARK,
                                            benchmark));
    #else
    /* Read smooth data instead if the touch V1 hardware does not support
     * benchmark */
    ESP_LOGI(TAG, "Smooth Type...");
    ESP_ERROR_CHECK(touch_channel_read_data(chan_handle,
                                            TOUCH_CHAN_DATA_TYPE_SMOOTH,
                                            benchmark));
    #endif

    touch_channel_config_t new_chan_cfg = TOUCH_CHAN_CFG_DEFAULT();
    for ( int j = 0; j < TOUCH_SAMPLE_NUM; j++ ) {
        uint32_t new_active_thresh = (uint32_t)(benchmark[j]
                                                * s_thresh2bm_ratio);
        ESP_LOGI(_TAG,
                 " Touch sample [%d]: %" PRIu32 ", %" PRIu32 "\t",
                 j,
                 benchmark[j],
                 new_active_thresh);

        new_chan_cfg.active_thresh[j] = new_active_thresh;
    }

    /* Update the channel configuration */
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        touch_sensor_reconfig_channel(chan_handle, &new_chan_cfg));
}

esp_err_t init_TouchBuitin(const char* _TAG,
                           int touch_channel_id,
                           EventGroupHandle_t xEventGroup) {
    /* Initialize touch sensor handle */
    touch_sensor_sample_config_t sample_cfg = {
        .charge_times      = 500,
        .charge_volt_lim_h = TOUCH_VOLT_LIM_H_2V2,
        .charge_volt_lim_l = TOUCH_VOLT_LIM_L_0V5,
        .idle_conn         = TOUCH_IDLE_CONN_GND,
        .bias_type         = TOUCH_BIAS_TYPE_SELF,
    };
    touch_sensor_config_t sens_cfg = {
        .power_on_wait_us = 256,
        .meas_interval_us = 32.0,
        .max_meas_time_us = 0,
        .sample_cfg_num   = TOUCH_SAMPLE_NUM,
        .sample_cfg       = &sample_cfg,
    };
    ESP_RETURN_ON_ERROR(touch_sensor_new_controller(&sens_cfg, &sens_handle),
                        _TAG,
                        "Failed to init touch controller.");

    touch_channel_config_t chan_cfg = TOUCH_CHAN_CFG_DEFAULT();
    ESP_RETURN_ON_ERROR(touch_sensor_new_channel(sens_handle,
                                                 touch_channel_id,
                                                 &chan_cfg,
                                                 &chan_handle),
                        _TAG,
                        "Failed to init touch channel handle.");

    touch_sensor_filter_config_t filter_cfg =
        TOUCH_SENSOR_DEFAULT_FILTER_CONFIG();
    ESP_RETURN_ON_ERROR(touch_sensor_config_filter(sens_handle, &filter_cfg),
                        _TAG,
                        "Failed to config filter.");

    /** Do initial scanning */
    touch_do_initial_scanning(_TAG, sens_handle, chan_handle);

    touch_event_callbacks_t callbacks = {
        .on_active   = touch_on_active_callback,
        .on_inactive = touch_on_inactive_callback,
    };
    // ESP_RETURN_ON_ERROR(touch_sensor_register_callbacks(sens_handle,
    // &callbacks, NULL), _TAG, "Failed to register callbacks.");
    ESP_RETURN_ON_ERROR(touch_sensor_register_callbacks(sens_handle,
                                                        &callbacks,
                                                        (void*)xEventGroup),
                        _TAG,
                        "Failed to register callbacks.");
    ESP_RETURN_ON_ERROR(touch_sensor_enable(sens_handle),
                        _TAG,
                        "Failed to enable touch.");
    ESP_RETURN_ON_ERROR(touch_sensor_start_continuous_scanning(sens_handle),
                        _TAG,
                        "Failed to start scanning.");

    return ESP_OK;
}

esp_err_t regTouchWakeup(const char* _TAG) {
    ESP_RETURN_ON_ERROR(touch_sensor_stop_continuous_scanning(sens_handle),
                        _TAG,
                        "Failed to stop scanning.");
    ESP_RETURN_ON_ERROR(touch_sensor_disable(sens_handle),
                        _TAG,
                        "Failed to disable touch.");

    #if CONFIG_EXAMPLE_TOUCH_DEEP_SLEEP_PD /* Not tested, maybe later */
    /* Get the channel information to use same active threshold for the sleep
   * channel */
    touch_chan_info_t chan_info = {};
    ESP_ERROR_CHECK(touch_sensor_get_channel_info(chan_handle[0], &chan_info));

    touch_sleep_config_t slp_cfg = TOUCH_SENSOR_DEFAULT_DSLP_PD_CONFIG(
        chan_handle[0],
        chan_info.active_thresh[0]);
    printf(
        "Touch channel %d (GPIO%d) is selected as deep sleep wakeup channel\n",
        chan_info.chan_id,
        chan_info.chan_gpio);
    #else
    touch_sleep_config_t slp_cfg = TOUCH_SENSOR_DEFAULT_DSLP_CONFIG();
    #endif

    ESP_RETURN_ON_ERROR(touch_sensor_config_sleep_wakeup(sens_handle, &slp_cfg),
                        _TAG,
                        "Failed to config sleep wakeup.");

    /* Step 5: Enable touch sensor controller and start continuous scanning before
   * entering light sleep */
    ESP_RETURN_ON_ERROR(touch_sensor_enable(sens_handle),
                        _TAG,
                        "Failed to enable touch.");
    ESP_RETURN_ON_ERROR(touch_sensor_start_continuous_scanning(sens_handle),
                        _TAG,
                        "Failed to start scanning.");
    return ESP_OK;
}

#else
esp_err_t init_TouchExternal(const char* _TAG, gpio_num_t pin_touch_sensor) {
    /* Use external touch sensor */
    gpio_config_t io_config = {.pin_bit_mask = BIT(pin_touch_sensor),
                               .mode         = GPIO_MODE_INPUT,
                               .pull_up_en   = GPIO_PULLUP_DISABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type    = GPIO_INTR_DISABLE};
    ESP_RETURN_ON_ERROR(gpio_config(&io_config),
                        _TAG,
                        "Failed to setup touch input pin.");

    return ESP_OK;
}
#endif  // SOC_TOUCH_SENSOR_SUPPORTED
