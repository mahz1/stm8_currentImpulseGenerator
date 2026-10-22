#ifndef PULSE_HEADER_GUARD
#define PULSE_HEADER_GUARD

#include "common.h"

void pulse_init(void);
void pulse_start(BYTE length);   // length в единицах 0.1 мс
void pulse_stop(void);
bool pulse_is_active(void);

#endif  // PULSE_HEADER_GUARD