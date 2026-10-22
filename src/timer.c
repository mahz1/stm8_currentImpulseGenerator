#include "timer.h"
#include "stm8s.h"
#include "stm8s_tim1.h"
#include "stm8s_clk.h"

// Предделитель 64 -> 250 кГц -> 4 мкс на тик
// 1 единица = 0.1 мс = 100 мкс = 25 тиков
#define TIMER_PRESCALER     64
#define TICKS_PER_UNIT      25 

void timer_init(void) {
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);

    // Начальный период - 3 мс
    TIM1_TimeBaseInit(TIMER_PRESCALER - 1, TIM1_COUNTERMODE_UP, 750, 0);
    TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
    
    enableInterrupts();
    TIM1_Cmd(DISABLE);
}

void timer_start(DWORD period_units) {
    DWORD ticks = period_units * TICKS_PER_UNIT;
    if (ticks > 65535) ticks = 65535;
    if (ticks < 1) ticks = 1;

    TIM1_Cmd(DISABLE);
    TIM1_SetAutoreload((uint16_t)ticks);
    TIM1_SetCounter(0);
    TIM1_ClearFlag(TIM1_FLAG_UPDATE);
    TIM1_Cmd(ENABLE);
}

void timer_stop(void) {
    TIM1_Cmd(DISABLE);
    TIM1_SetCounter(0);
    TIM1_ClearFlag(TIM1_FLAG_UPDATE);
}