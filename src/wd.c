#include "wd.h"
#include "stm8s.h"
#include "stm8s_iwdg.h"
#include "stm8s_clk.h"

// Настройка IWDG на ~500 мс
// Prescaler = 256, Reload = 249
// Timeout = (249 + 1) * 256 / 128000 = 0.5 s

void wd_start(void) {
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_256);
    IWDG_SetReload(249);
    IWDG_ReloadCounter();
    IWDG_Enable();
}

void wd_reset(void) {
    IWDG_ReloadCounter();
}