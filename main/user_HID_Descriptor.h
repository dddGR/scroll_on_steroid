#ifndef _HID_DESCRIPTOR_H_
#define _HID_DESCRIPTOR_H_

#include <stdint.h>

/** This is from USBCLASS_HID_TYPES, to make life easier */
// HIDTypes.h ==================================================================
/* HID Class Report Descriptor */
/* Short items: size is 0, 1, 2 or 3 specifying 0, 1, 2 or 4 (four) bytes */
/* of data as per HID Class standard */

/* Main items */
#define INPUT(size)  (0x80 | size)
#define OUTPUT(size) (0x90 | size)
#define FEATURE(size)        (0xb0 | size)
#define COLLECTION(size)     (0xa0 | size)
#define END_COLLECTION(size) (0xc0 | size)

/* Global items */
#define USAGE_PAGE(size)       (0x04 | size)
#define LOGICAL_MINIMUM(size)  (0x14 | size)
#define LOGICAL_MAXIMUM(size)  (0x24 | size)
#define PHYSICAL_MINIMUM(size) (0x34 | size)
#define PHYSICAL_MAXIMUM(size) (0x44 | size)
#define UNIT_EXPONENT(size)    (0x54 | size)
#define UNIT(size)             (0x64 | size)
#define REPORT_SIZE(size)      (0x74 | size)  //bits
#define REPORT_ID(size)        (0x84 | size)
#define REPORT_COUNT(size)     (0x94 | size)  //bytes
#define PUSH(size)             (0xa4 | size)
#define POP(size)              (0xb4 | size)

/* Local items */
#define USAGE(size)              (0x08 | size)
#define USAGE_MINIMUM(size)      (0x18 | size)
#define USAGE_MAXIMUM(size)      (0x28 | size)
#define DESIGNATOR_INDEX(size)   (0x38 | size)
#define DESIGNATOR_MINIMUM(size) (0x48 | size)
#define DESIGNATOR_MAXIMUM(size) (0x58 | size)
#define STRING_INDEX(size)       (0x78 | size)
#define STRING_MINIMUM(size)     (0x88 | size)
#define STRING_MAXIMUM(size)     (0x98 | size)
#define DELIMITER(size)          (0xa8 | size)
//==============================================================================


/** This is for scroll only */
const uint8_t scrollDescriptor_hiRes[] = {
    USAGE_PAGE(1),              0x01,   //  USAGE_PAGE (Generic Desktop)
    USAGE(1),                   0x02,   //  USAGE (Mouse)
    COLLECTION(1),              0x01,   //  COLLECTION (Application)
    USAGE_PAGE(1),              0x01,   //      USAGE_PAGE (Generic Desktop)
    USAGE(1),                   0x02,   //      USAGE (Mouse)
    COLLECTION(1),              0x02,   //      COLLECTION (Logical)
    REPORT_ID(1),    REPORT_ID_MOUSE,   //          REPORT_ID (REPORT_ID_MOUSE)
    USAGE(1),                   0x01,   //          USAGE (Pointer)
    COLLECTION(1),              0x00,   //          COLLECTION (Physical)
    //==================================== Mouse movement (only for compatibility)
    USAGE_PAGE(1),              0x01,   //              USAGE_PAGE (Generic Desktop)
    USAGE(1),                   0x30,   //              USAGE (X)
    USAGE(1),                   0x31,   //              USAGE (Y)
    LOGICAL_MINIMUM(1),         0x00,   //              LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1),         0x00,   //              LOGICAL_MAXIMUM (0)
    REPORT_SIZE(1),             0x08,   //              REPORT SIZE (8)
    REPORT_COUNT(1),            0x02,   //              REPORT_COUNT (2)
    INPUT(1),                   0x06,   //              INPUT (Data, Variable, Relative)
    COLLECTION(1),              0x02,   //              COLLECTION (Logical)
    //==================================== Vertical Scroll Wheel
    // REPORT_ID(1),    REPORT_ID_MOUSE,   //                  REPORT_ID (REPORT_ID_MOUSE)
    USAGE(1),                   0x38,   //                  USAGE (Wheel)
    PHYSICAL_MINIMUM(1),        0x00,   //                  PHYSICAL_MINIMUM (0)
    PHYSICAL_MAXIMUM(1),        0x00,   //                  PHYSICAL_MAXIMUM (0)
    LOGICAL_MINIMUM(1),         0x81,   //                  LOGICAL_MINIMUM (-127)
    LOGICAL_MAXIMUM(1),         0x7F,   //                  LOGICAL_MAXIMUM (127)
    REPORT_COUNT(1),            0x01,   //                  REPORT_COUNT (1)
    REPORT_SIZE(1),             0x08,   //                  REPORT_SIZE (8)
    INPUT(1),                   0x06,   //                  INPUT (Data, Variable, Relative)
    //==================================== Horizontal Scroll
    USAGE_PAGE(1),              0x0C,   //                  USAGE_PAGE (Consumer)
    USAGE(2),             0x38, 0x02,   //                  USAGE (AC Pan)
    REPORT_COUNT(1),            0x01,   //                  REPORT_COUNT (1)
    REPORT_SIZE(1),             0x08,   //                  REPORT_SIZE (8)
    LOGICAL_MINIMUM(1),         0x81,   //                  LOGICAL_MINIMUM (-127)
    LOGICAL_MAXIMUM(1),         0x7F,   //                  LOGICAL_MAXIMUM (127)
    INPUT(1),                   0x06,   //                  INPUT (Data, Variable, Relative)
    //==================================== Resolution Multiplier
    REPORT_ID(1), REPORT_ID_MULTIPLIER, //                  REPORT_ID (REPORT_ID_MULTIPLIER)
    USAGE_PAGE(1),              0x01,   //                  USAGE_PAGE (Generic Desktop)
    USAGE(1),                   0x48,   //                  USAGE (Resolution Multiplier)
    LOGICAL_MINIMUM(1),         0x00,   //                  LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1),         0x01,   //                  LOGICAL_MAXIMUM (1)
    PHYSICAL_MINIMUM(1),        0x01,   //                  PHYSICAL_MINIMUM (1)
    PHYSICAL_MAXIMUM(1),        0x78,   //                  PHYSICAL_MAXIMUM (120)
    REPORT_SIZE(1),             0x08,   //                  REPORT SIZE (8)
    REPORT_COUNT(1),            0x01,   //                  REPORT_COUNT (1)
    FEATURE(1),                 0x02,   //                  FEATURE (Data, Variable, Absolute)
    END_COLLECTION(0),                  //              END_COLLECTION
    //====================================
    END_COLLECTION(0),                  //          END_COLLECTION
    END_COLLECTION(0),                  //      END_COLLECTION
    END_COLLECTION(0)                   //  END_COLLECTION
};


#endif