#ifndef _USER_TYPES_H_
#define _USER_TYPES_H_

#include <stdint.h>

/**  SOME BASIC MACROS TO MAKE LIFE EASIER */

#ifndef MIN
    #define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
    #define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef ABS
    #define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

#ifndef SQRT
    #define SQRT(x) ((x) * (x))
#endif

/* Map value x from range [a] to range [b] */
#define MAP(x, a_min, a_max, b_min, b_max)\
    (((x - a_min) * (b_max - b_min)) / (a_max - a_min) + b_min)

#ifndef POW2
    #define POW2(x) ((uint16_t)1u << (x))
#endif

#ifndef POW2_U32
    #define POW2_U32(x) ((uint32_t)1ul << (x))
#endif

#ifndef POW2_U64
    #define POW2_U64(x) ((uint64_t)1ull << (x))
#endif

#ifndef HIGH
    #define HIGH    (1)
#endif
#ifndef LOW
    #define LOW     (0)
#endif


#endif