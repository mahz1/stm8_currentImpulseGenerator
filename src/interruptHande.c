#include "interruptHande.h"
#include "stm8s.h"
#include "timer.h"
#include "pulse.h"

// IRQ-номер TIM1 Update/Overflow = 11
INTERRUPT_HANDLER(TIM1_UPD_OVF_IRQHandler, 11)
{
    timer_stop();
    pulse_stop();
}