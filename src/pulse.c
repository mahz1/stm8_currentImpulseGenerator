#include "pulse.h"
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "timer.h"

// Пины импульса
#define PULSE_PIN           GPIO_PIN_5
#define PULSE_PORT          GPIOD
#define SAFETY_PIN          GPIO_PIN_6
#define SAFETY_PORT         GPIOD

static volatile bool pulse_active = false;

void pulse_init(void) {
    GPIO_Init(PULSE_PORT,  PULSE_PIN,  GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(SAFETY_PORT, SAFETY_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    pulse_stop();
}

void pulse_start(BYTE length) {
    GPIO_WriteHigh(PULSE_PORT, PULSE_PIN);
    GPIO_WriteLow(SAFETY_PORT, SAFETY_PIN); // Снимаем предохранитель
    timer_start(length);
    pulse_active = true;
}

void pulse_stop(void) {
    pulse_active = false;
    GPIO_WriteLow(PULSE_PORT, PULSE_PIN);
    GPIO_WriteHigh(SAFETY_PORT, SAFETY_PIN); // Ставим на предохранитель
}

bool pulse_is_active(void) { return pulse_active; }