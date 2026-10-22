#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_clk.h"
#include "stm8s_i2c.h"
#include "common.h"
#include "show.h"
#include "measure.h"
#include "charge.h"
#include "pulse.h"
#include "timer.h"
#include "file.h"
#include "keyboard.h"
#include "wd.h"

MODE_TYPE mode = WORK;

int main(void) {
    // 1. Тактирование 16 МГц
    CLK_HSICmd(ENABLE);
    CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);

    // 2. Светодиод (индикатор)
    GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_SLOW);

    // 3. I2C
    GPIO_Init(GPIOE, GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_OUT_OD_HIZ_FAST);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
    I2C_Init(100000, 0x00, I2C_DUTYCYCLE_2, I2C_ACK_CURR, I2C_ADDMODE_7BIT, 16);
    I2C_Cmd(ENABLE);

    // 4. Инициализация модулей (сначала выключаем силовую часть!)
    pulse_init();
    charge_init();
    show_init(); // Долгая инициализация LCD
    
    // NOTE: Watchdog отключён на время отладки.
    // wd_start();

    measure_init();
    timer_init();
    file_init();
    keyboard_init();

    // Переменные состояний
    WORD amperage = 500;
    WORD pulse_length = 3 * PULSE_DURATION_SCALE;
    WORD measured_amperage = 0;

    enum STATE_TYPE {
        STATE_GET_MODE, STATE_GET_VALUE, STATE_CHARGE, 
        STATE_READY, STATE_PULSE, STATE_SHOW_RESULT, 
        STATE_DISCHARGE, STATE_SETKI, STATE_EMERGENCY
    };

    enum STATE_TYPE state = STATE_GET_MODE;
    ANSWER_TYPE answer;

    while (1) {
        switch (state) {
            case STATE_GET_MODE:
                mode = show_get_mode();
                if (mode == WORK)           state = STATE_GET_VALUE;
                else if (mode == DISCHARGE) state = STATE_DISCHARGE;
                else if (mode == SET_KI)    state = STATE_SETKI;
                break;

            case STATE_GET_VALUE:
                answer = show_input_amperage(&amperage);
                if (answer == ANSWER_ESC) { state = STATE_GET_MODE; break; }
                
                answer = show_input_length(&pulse_length);
                if (answer == ANSWER_ESC) { state = STATE_GET_MODE; break; }
                
                state = STATE_CHARGE;
                break;

            case STATE_CHARGE:
                answer = show_charging(amperage);
                state = (answer == ANSWER_ESC) ? STATE_GET_VALUE : STATE_READY;
                break;

            case STATE_READY:
                answer = show_wait_command();
                state = (answer == ANSWER_ESC) ? STATE_GET_VALUE : STATE_PULSE;
                break;

            case STATE_PULSE:
                // Запуск с учётом длины фронта
                pulse_start((BYTE)(pulse_length + measure_get_front_len()));
                measured_amperage = measure_measure_amperage((BYTE)pulse_length);
                state = STATE_SHOW_RESULT;
                break;

            case STATE_SHOW_RESULT:
                show_result(measured_amperage);
                measure_calc_impedance(amperage, measured_amperage);
                state = STATE_GET_VALUE;
                break;

            case STATE_DISCHARGE:
                show_discharge();
                state = STATE_GET_MODE;
                break;

            case STATE_SETKI:
                show_set_ki();
                state = STATE_GET_MODE;
                break;

            case STATE_EMERGENCY:
                show_emergency();
                state = STATE_GET_MODE;
                break;

            default:
                state = STATE_GET_MODE;
                break;
        }
    }
}

// Заглушка SPL
void assert_failed(uint8_t* file, uint32_t line) {
    while (1) { }
}