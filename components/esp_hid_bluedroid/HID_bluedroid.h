#ifndef _HID_BLUEDROID_H_
#define _HID_BLUEDROID_H_

#include "esp_hidd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REPORT_ID_MOUSE         0x01
#define REPORT_ID_MULTIPLIER    0x02

/**
 * @brief Initialize the HID device
 * 
 * This function initializes the HID device by setting up the NVS flash, 
 * configuring the HID GAP and BLE GAP advertising, and registering the GATTS callback.
 * 
 * @param[out] hid_device Pointer to the HID device
 * @param[in] ble_hid_cfg Pointer to the BLE HID device configuration
 * 
 * @return 
 *     - ESP_OK: Success
 *     - Others: Failure
 */
esp_err_t hidDevInit(esp_hidd_dev_t **hid_device, const esp_hid_device_config_t *ble_hid_cfg);

extern void do_when_ble_hid_connect();       // use this to do things when a device is connected
extern void do_when_ble_hid_disconnect();    // use this to do things when a device is disconnected
extern void do_when_ble_hid_stop();          // use this to do things when a device is stopped

/**
 * @brief Send a scroll event over the hid device
 * @param[in] hid_device: The hid device to send the event to
 * @param[in] wheel: The vertical scroll amount
 * @param[in] hWheel: The horizontal scroll amount
 */
void sendScroll(esp_hidd_dev_t *hid_device, const char wheel, const char hWheel);

#ifdef __cplusplus
}
#endif

#endif