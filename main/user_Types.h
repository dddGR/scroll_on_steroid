#ifndef _USER_TYPES_H_
#define _USER_TYPES_H_

#include <stdint.h>

#ifndef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

/* For button touch state */
typedef enum {
    B_PRESS_NONE,
    B_PRESS_1,
    B_PRESS_2,
    B_PRESS_3,
    B_PRESS_HOLD = UINT8_MAX
} ButtonPressState_t;

/* For scroll speed division */
typedef enum {
    SPEED_HIGH = 1,
    SPEED_MEDIUM,
    SPEED_LOW = 4
} scrollSpeedDiv;

/* For scroll direction */
typedef enum {
    SCROLL_VERTICAL,
    SCROLL_HORIZONTAL
} scrollDirection;

#define DEVICE_ACTIVE       BIT(0)
#define DEVICE_IDLE         BIT(1)
#define BUTTON_PRESSED      BIT(2)
#define BUTTON_RELEASED     BIT(3)
#define BLE_CONNECTED       BIT(8)
#define BLE_DISCONNECTED    BIT(9)

#endif // _USER_TYPES_H_