#ifndef MEASURE_HEADER_GUARD
#define MEASURE_HEADER_GUARD

#include "common.h"

// ============================================================
//  Инициализация
// ============================================================
void measure_init(void);

// ============================================================
//  Главные функции измерений
// ============================================================

// Измерение тока во время импульса.
// Возвращает амплитуду тока (в Амперах, умноженных на ... — см. код).
WORD measure_amperage(BYTE setted_len);

// Процент заряда конденсаторов (0..1010) и текущее напряжение.
// Возвращает процент * 10, в charge — само напряжение (В).
WORD measure_get_charge_percent(WORD need_amperage, float* charge);

// Максимально возможный ток для текущего импеданса
float measure_get_max_amperage_for_impedance(void);

// Напряжение на конденсаторах (В)
float measure_get_capacitor_voltage(void);

// Проверка на утечку тока
bool measure_check_safe(void);

// Проверки на допустимость
WORD measure_calc_amperage(BYTE setted_len);
bool measure_check_pulse_length(BYTE value);

// Калибровки
void measure_calc_impedance(WORD specified_current, WORD measured_current);
void measure_calc_ki(WORD measured_current, WORD real_current);

// Ожидание окончания фронта (пока пустая, как в оригинале)
void measure_awaiting_front(void);

// Длительность переднего фронта в единицах 0.1 мс
BYTE measure_get_front_len(void);

// Поиск длительности импульса на заданном уровне
BYTE measure_get_impulse_len(WORD level);

// Измеренная длительность импульса (после calc_amperage)
BYTE measure_get_impulse_len_measured(void);

// === Отладочные (для show_debug) ===
WORD measure_get_max_code(void);
WORD measure_get_threshold_code(void);
WORD measure_get_max_pos(void);
WORD measure_get_samples_qnty(void);
BYTE measure_get_total_len(void);
WORD measure_get_sample(WORD index);

// ============================================================
//  КАНАЛЫ АЦП
// ============================================================
// AIN0 = PB0 — ток через шунт
// AIN1 = PB1 — напряжение на конденсаторах
#define MEASURE_ADC_CH_CURRENT   0
#define MEASURE_ADC_CH_CHARGE    1

#endif  // MEASURE_HEADER_GUARD