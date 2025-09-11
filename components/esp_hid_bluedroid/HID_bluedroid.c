#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"


#include "esp_hid_gap.h"
#include "sdkconfig.h"

#include "HID_bluedroid.h"
#include "hid_Descriptor.h"

static const char *TAG = "HID_DEV";

#if CONFIG_BT_BLUEDROID_ENABLED && CONFIG_BT_BLE_42_FEATURES_SUPPORTED

esp_hid_raw_report_map_t ble_report_maps[] = {
    {
        .data = scrollDescriptor_hiRes,
        .len = sizeof(scrollDescriptor_hiRes)
    }
};

static void ble_hidd_event_cb(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDD_START_EVENT: {
        ESP_LOGI(TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT: {
        ESP_LOGI(TAG, "CONNECT");
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT: {
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT: {
        ESP_LOGI(TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        if (param->control.control)
        {
            // exit suspend
            do_when_ble_hid_connect();
        } else {
            // suspend
            do_when_ble_hid_disconnect();
        }
    break;
    }
    case ESP_HIDD_OUTPUT_EVENT: {
        ESP_LOGI(TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        break;
    }
    case ESP_HIDD_FEATURE_EVENT: {
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT: {
        ESP_LOGI(TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        do_when_ble_hid_disconnect();
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT: {
        ESP_LOGI(TAG, "STOP");
        do_when_ble_hid_stop();
        break;
    }
    default:
        break;
    }
    return;
}

esp_err_t hidDevInit(esp_hidd_dev_t **hid_device, const esp_hid_device_config_t *ble_hid_cfg)
{
    esp_err_t _res;

    _res = nvs_flash_init();
    if (_res == ESP_ERR_NVS_NO_FREE_PAGES || _res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "erase NVS failed");
        _res = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(_res, TAG, "failed to initialize NVS");

    ESP_LOGI(TAG, "setting hid gap, mode:%d", HID_DEV_MODE);
    ESP_RETURN_ON_ERROR(esp_hid_gap_init(HID_DEV_MODE), TAG, "failed to init hid gap");
    ESP_RETURN_ON_ERROR(esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_MOUSE, ble_hid_cfg->device_name), TAG, "failed to init ble gap adv");
    ESP_RETURN_ON_ERROR(esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler), TAG, "GATTS register callback failed");

    ESP_LOGI(TAG, "setting ble device");
    ESP_RETURN_ON_ERROR(esp_hidd_dev_init(ble_hid_cfg, ESP_HID_TRANSPORT_BLE, ble_hidd_event_cb, hid_device), TAG, "failed to init hid device");
    
    return ESP_OK;
}

void sendScroll(esp_hidd_dev_t *hid_device, const char wheel, const char hWheel)
{
    static uint8_t buffer[4] = {0, 0, 0, 0};
    // buffer[0] = dx;
    // buffer[1] = dy;
    buffer[2] = wheel;
    buffer[3] = hWheel;
    
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_hidd_dev_input_set(hid_device, 0, REPORT_ID_MOUSE, buffer, sizeof(buffer)));
}



#endif // CONFIG_BT_BLUEDROID_ENABLED && CONFIG_BT_BLE_42_FEATURES_SUPPORTED