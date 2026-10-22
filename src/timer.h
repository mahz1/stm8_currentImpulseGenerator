#ifndef TIMER_HEADER_GUARD
#define TIMER_HEADER_GUARD

#include "common.h"

void timer_init(void);
void timer_start(DWORD period_units);   // period_units в единицах 0.1 мс
void timer_stop(void);

#endif  // TIMER_HEADER_GUARD