#ifndef CHARGE_HEADER_GUARD
#define CHARGE_HEADER_GUARD

#include "common.h"

void charge_init(void);
void charge_charge(void);
void charge_discharge(void);
void charge_discharge_slow(void);
void charge_stop(void);

#endif // CHARGE_HEADER_GUARD