#include "safe.h"
#include "measure.h"
#include "show.h"

void safe_check(void) {
    if (!measure_check_safe()) {
        show_emergency(); // Аварийный режим
    }
}