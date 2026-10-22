#include "measure.h"
#include "stm8s.h"
#include "stm8s_adc1.h"
#include "stm8s_gpio.h"
#include "stm8s_clk.h"
#include "file.h"

const float VREF = 5.0f; // VDDA = 5V
#define ADC_MAX_CODE 1023
#define MAX_SAMPLE_QNTY 200

WORD measure_bias_code = 0;

// Параметры цепи
const float MIN_CIRCUIT_IMPEDANCE = 0.0013f;
const float MAX_CIRCUIT_IMPEDANCE = 0.0160f;
float measure_circuit_impedance = 0.0013f;

// Параметры измерения тока
const float SHUNT_IMPEDANCE = 0.000167f;
const float SHUNT_KGAIN = 90.9f / 49.9f;
const float CHARGE_KGAIN = 10.0f / 100.0f / 0.9953f;
const float MAX_CHARGE_VOLTAGE = 8.01f;

const WORD MIN_AMPERAGE = 500;
const WORD MAX_AMPERAGE = 3000;
const WORD SAFE_ACCEPTABLE_STANDBY_CURRENT = ADC_MAX_CODE / 10;

// Границы длительности
#define MAX_PULSE_LEN         (MAX_SAMPLE_QNTY / 10 * PULSE_DURATION_SCALE)
#define MIN_PULSE_LEN         (1 * PULSE_DURATION_SCALE)
#define RISE_FRONT_MIN_LEN    1

BYTE measure_fronts_len = RISE_FRONT_MIN_LEN * PULSE_DURATION_SCALE;
BYTE measure_measured_len = MIN_PULSE_LEN;

const BYTE NOISE_LEN = 5;
WORD samples[MAX_SAMPLE_QNTY + 5];
WORD* rawSamples = &samples[1];
WORD measure_sample_qnty = 0;

// Переменные для отладки
WORD measure_max_code = 0;
WORD measure_max_pos = 0;
WORD measure_threshold_code = 0;
BYTE measure_impulse_total_len = 0;

void measure_init(void) {
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
    GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_IN_FL_NO_IT);

    ADC1_Init(ADC1_CONVERSIONMODE_SINGLE, ADC1_CHANNEL_0, ADC1_PRESSEL_FCPU_D2,
              ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT,
              ADC1_SCHMITTTRIG_CHANNEL0, DISABLE);
    ADC1_Cmd(ENABLE);
}

WORD measure_get_adc_value(BYTE analog_channel) {
    WORD acc = 0, value = 0, min = ADC_MAX_CODE, max = 0;
    const BYTE AVERAGE_RATIO = 5;

    ADC1_ConversionConfig(ADC1_CONVERSIONMODE_SINGLE, (ADC1_Channel_TypeDef)analog_channel, ADC1_ALIGN_RIGHT);
    
    ADC1_StartConversion();
    while (!ADC1_GetFlagStatus(ADC1_FLAG_EOC));
    ADC1_ClearFlag(ADC1_FLAG_EOC);
    value = ADC1_GetConversionValue();
    
    { volatile WORD i; for (i = 0; i < 800; i++); } // Стабилизация канала

    for (BYTE i = 0; i < AVERAGE_RATIO; ++i) {
        ADC1_StartConversion();
        while (!ADC1_GetFlagStatus(ADC1_FLAG_EOC));
        ADC1_ClearFlag(ADC1_FLAG_EOC);
        value = ADC1_GetConversionValue();

        if (value > 1000) { // Защита от выбросов
            volatile WORD j; for (j = 0; j < 800; j++);
            ADC1_StartConversion();
            while (!ADC1_GetFlagStatus(ADC1_FLAG_EOC));
            ADC1_ClearFlag(ADC1_FLAG_EOC);
            value = ADC1_GetConversionValue();
        }

        acc += value;
        if (value < min) min = value;
        if (value > max) max = value;
    }

    acc -= max + min;
    acc /= AVERAGE_RATIO - 2;
    return acc;
}

float measure_adc_code_to_voltage(WORD adc_code) {
    const float scale = VREF / (float)ADC_MAX_CODE;
    return (float)adc_code * scale;
}

float measure_shunt_voltage_to_amperage(float voltage) {
    return voltage / SHUNT_IMPEDANCE / SHUNT_KGAIN * file_read_ki();
}

float measure_get_capacitor_voltage(void) {
    WORD adc = measure_get_adc_value(MEASURE_ADC_CH_CHARGE);
    return measure_adc_code_to_voltage(adc) / CHARGE_KGAIN;
}

float measure_get_max_amperage_for_impedance(void) {
    return MAX_CHARGE_VOLTAGE / measure_circuit_impedance;
}

WORD measure_get_charge_percent(WORD need_amperage, float* charge) {
    float v = measure_get_capacitor_voltage();
    *charge = v;

    float have_amperage = v / measure_circuit_impedance;
    WORD percent = (WORD)(have_amperage * 1000.0f / (float)need_amperage);

    if (v > MAX_CHARGE_VOLTAGE && percent < 1000) percent = 1010;
    return percent;
}

bool measure_check_amperage(WORD value) {
    return (MIN_AMPERAGE > value) || (value > MAX_AMPERAGE);
}

bool measure_check_pulse_length(BYTE value) {
    return (MIN_PULSE_LEN > value) || (value > MAX_PULSE_LEN);
}

bool measure_check_safe(void) {
    extern bool pulse_is_active(void);
    if (!pulse_is_active()) {
        WORD amperage_code = measure_get_adc_value(MEASURE_ADC_CH_CURRENT);
        if (SAFE_ACCEPTABLE_STANDBY_CURRENT <= amperage_code) return false;
    }
    return true;
}

void measure_calc_ki(WORD measured_current, WORD real_current) {
    if (measured_current == 0 || real_current == 0) return;
    float ki = file_read_ki();
    ki /= (float)measured_current / (float)real_current;
    file_write_ki(ki);
}

void measure_calc_impedance(WORD specified_current, WORD measured_current) {
    if (measured_current == 0) return;

    float impedance = measure_circuit_impedance;
    impedance *= (float)specified_current / (float)measured_current;

    if (impedance > MAX_CIRCUIT_IMPEDANCE) impedance = MAX_CIRCUIT_IMPEDANCE;
    else if (impedance < MIN_CIRCUIT_IMPEDANCE) impedance = MIN_CIRCUIT_IMPEDANCE;

    measure_circuit_impedance = impedance;
}

WORD measure_measure_amperage(BYTE setted_len) {
    extern bool pulse_is_active(void);
    measure_sample_qnty = 0;

    while (pulse_is_active() && measure_sample_qnty < MAX_SAMPLE_QNTY) {
        rawSamples[measure_sample_qnty++] = measure_get_adc_value(MEASURE_ADC_CH_CURRENT);
        volatile BYTE i; for (i = 0; i < 100; i++); // Задержка между сэмплами
    }

    if (measure_sample_qnty > 0) --measure_sample_qnty;
    return measure_calc_amperage(setted_len);
}

BYTE measure_get_impulse_len_measured(void) { return measure_measured_len; }

WORD measure_calc_amperage(BYTE setted_len) {
    measure_impulse_total_len = setted_len + measure_fronts_len;
    if (NOISE_LEN + 4 >= measure_sample_qnty) return 0;

    WORD i;
    // Копируем края без фильтрации
    for (i = 0; i < NOISE_LEN + 2; ++i) samples[i] = rawSamples[i];
    samples[measure_sample_qnty - 2] = rawSamples[measure_sample_qnty - 2];
    samples[measure_sample_qnty - 1] = rawSamples[measure_sample_qnty - 1];

    // Взвешенное скользящее среднее
    for (i = NOISE_LEN + 2; i < measure_sample_qnty - 2; ++i) {
        if (rawSamples[i] > 1020) rawSamples[i] = rawSamples[i - 1];
        float acc = (float)rawSamples[i - 2] * 0.05f + (float)rawSamples[i - 1] * 0.25f +
                    (float)rawSamples[i    ] * 0.40f + (float)rawSamples[i + 1] * 0.25f +
                    (float)rawSamples[i + 2] * 0.05f;
        samples[i] = (WORD)(acc + 0.5f);
    }

    measure_max_code = 0; measure_max_pos = 0;
    for (i = NOISE_LEN; i < measure_sample_qnty; ++i) {
        if (measure_max_code <= samples[i]) {
            measure_max_code = samples[i];
            measure_max_pos = i;
        }
    }

    // Бинарный поиск уровня (90% от максимума и выше)
    WORD top = measure_max_code;
    WORD bottom = measure_max_code * 9 / 10;
    WORD amplitude = bottom;
    BYTE result_len = 0;

    const WORD expected_tick_qnty =
        (WORD)(((float)(measure_sample_qnty + 1) * (float)setted_len)
               / (float)(setted_len + measure_fronts_len));

    while (1) {
        result_len = measure_get_impulse_len(amplitude);
        if (result_len > expected_tick_qnty)      bottom = amplitude;
        else if (result_len < expected_tick_qnty) top = amplitude;
        else                                      break;

        amplitude = bottom + (top - bottom) / 2;
        if ((top - bottom) < 2) break;
    }

    measure_threshold_code = amplitude;

    const float tick_len = (float)(setted_len + measure_fronts_len) / (float)(measure_sample_qnty + 1);
    BYTE measured_len_units = (BYTE)((float)result_len * tick_len + 0.5f);
    measure_measured_len = measured_len_units;
    measure_fronts_len = (measure_fronts_len + setted_len) - measured_len_units;

    float voltage = measure_adc_code_to_voltage(measure_threshold_code);
    return (WORD)(measure_shunt_voltage_to_amperage(voltage) + 0.5f);
}

BYTE measure_get_impulse_len(WORD level) {
    WORD tick_cntr = 0;
    WORD i;

    for (i = measure_max_pos; i < measure_sample_qnty - 1; ++i) {
        if (level > samples[i] && level > samples[i + 1]) break;
        ++tick_cntr;
    }

    for (i = measure_max_pos - 1; i >= NOISE_LEN; --i) {
        if (level > samples[i] && level > samples[i - 1]) break;
        ++tick_cntr;
    }

    return (BYTE)tick_cntr;
}

BYTE measure_get_front_len(void) { return measure_fronts_len; }