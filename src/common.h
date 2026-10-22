#ifndef COMMON_HEADER_GUARD
#define COMMON_HEADER_GUARD

#include "stm8s.h"
#include <stdbool.h>

// Замена стандартных типов Arduino
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef int16_t  SWORD;
typedef uint32_t DWORD;

// Заглушки отладки (Serial на STM8 нет)
#define PRINT_NAMED(str, value)   {}
#define PRINT(value)              {}
#define PRINT_LN()                {}

typedef enum {
    WORK,
    CALIBRATION,
    DISCHARGE,
    SET_KI
} MODE_TYPE;

// Масштаб длительности импульса (1 единица = 0.1 мс)
#define PULSE_DURATION_SCALE 10

#endif  // COMMON_HEADER_GUARD