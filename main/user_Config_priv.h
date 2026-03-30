#ifndef _USER_CONFIG_PRIV_H_
#define _USER_CONFIG_PRIV_H_

/** THIS FILE CONTAINS PRIVATE USER CONFIGURATIONS RELATED TO THE PROJECT
 *  THAT SHARED ACROSS THE TRANSLATION UNITS, BUT NOT MEAN FOR USER TO MODIFY */
#include "sdkconfig.h"   // IWYU pragma: export
#include "user_Types.h"  // IWYU pragma: export

#if CONFIG_FREERTOS_UNICORE
    #define APP_CORE_ID (0)
#else
    #define APP_CORE_ID (1)
#endif

/* For button touch state */
typedef enum ButtonPressState {
    B_PRESS_NONE,
    B_PRESS_1,
    B_PRESS_2,
    B_PRESS_3,
    B_PRESS_HOLD = UINT8_MAX
} ButtonPressState_t;

/* For scroll speed division */
typedef enum ScrollSpeedDiv {
    SPEED_HIGH = 1,
    // SPEED_MEDIUM, /* not much different */
    SPEED_LOW  = 4
} ScrollSpeedDiv_t;

/* For scroll direction */
typedef enum ScrollDirection {
    SCROLL_VERTICAL,
    SCROLL_HORIZONTAL
} ScrollDirection_t;

/* Bit for event group */
#define DEVICE_ACTIVE    BIT(0)
#define DEVICE_IDLE      BIT(1)
#define BUTTON_PRESSED   BIT(2)
#define BUTTON_RELEASED  BIT(3)
#define BLE_CONNECTED    BIT(8)
#define BLE_DISCONNECTED BIT(9)

#endif
