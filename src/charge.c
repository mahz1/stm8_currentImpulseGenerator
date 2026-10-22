#include "charge.h"
#include "stm8s.h"
#include "stm8s_gpio.h"

// Пины управления силовыми ключами
#define CHARGE_PIN          GPIO_PIN_2
#define CHARGE_PORT         GPIOD

#define DISCHARGE_PIN       GPIO_PIN_3
#define DISCHARGE_PORT      GPIOD

#define DISCHARGE_SLOW_PIN  GPIO_PIN_4
#define DISCHARGE_SLOW_PORT GPIOD

// Активные уровни
#define ON  1
#define OFF 0

// Установка уровня на пине
static void pin_write(GPIO_TypeDef* port, GPIO_Pin_TypeDef pin, unsigned char value) {
    if (value) GPIO_WriteHigh(port, pin);
    else       GPIO_WriteLow(port, pin);
}

void charge_init(void) {
    // Настройка пинов в push-pull
    GPIO_Init(CHARGE_PORT,         CHARGE_PIN,         GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISCHARGE_PORT,      DISCHARGE_PIN,      GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISCHARGE_SLOW_PORT, DISCHARGE_SLOW_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);

    // Двойной стоп для безопасности при старте
    charge_stop();
    charge_stop();
}

void charge_charge(void) {
    pin_write(DISCHARGE_PORT,      DISCHARGE_PIN,      OFF);
    pin_write(DISCHARGE_SLOW_PORT, DISCHARGE_SLOW_PIN, OFF);
    pin_write(CHARGE_PORT,         CHARGE_PIN,         ON);
}

void charge_discharge(void) {
    pin_write(CHARGE_PORT,         CHARGE_PIN,         OFF);
    pin_write(DISCHARGE_PORT,      DISCHARGE_PIN,      ON);
    pin_write(DISCHARGE_SLOW_PORT, DISCHARGE_SLOW_PIN, ON);
}

// NOTE: В оригинальном проекте в этой функции стоит return, 
// поэтому она дублирует charge_discharge(). Оставлено как есть для совместимости.
void charge_discharge_slow(void) {
    charge_discharge();
}

void charge_stop(void) {
    pin_write(CHARGE_PORT,         CHARGE_PIN,         OFF);
    pin_write(DISCHARGE_PORT,      DISCHARGE_PIN,      OFF);
    pin_write(DISCHARGE_SLOW_PORT, DISCHARGE_SLOW_PIN, OFF);
}