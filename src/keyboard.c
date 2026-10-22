#include "keyboard.h"
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "wd.h"
#include "safe.h"

// Матрица 4x3
#define KBD_PORT  GPIOC
#define ROW1  GPIO_PIN_1
#define ROW2  GPIO_PIN_2
#define ROW3  GPIO_PIN_3
#define ROW4  GPIO_PIN_4
#define COL1  GPIO_PIN_5
#define COL2  GPIO_PIN_6
#define COL3  GPIO_PIN_7

static const char keymap[4][3] = {
    { '1', '2', '3' },
    { '4', '5', '6' },
    { '7', '8', '9' },
    { '*', '0', '#' }
};

static const GPIO_Pin_TypeDef rows[4] = { ROW1, ROW2, ROW3, ROW4 };
static const GPIO_Pin_TypeDef cols[3] = { COL1, COL2, COL3 };

static char last_key = KEY_NONE;

void keyboard_init(void) {
    // Строки - выходы
    GPIO_Init(KBD_PORT, ROW1, GPIO_MODE_OUT_PP_HIGH_SLOW);
    GPIO_Init(KBD_PORT, ROW2, GPIO_MODE_OUT_PP_HIGH_SLOW);
    GPIO_Init(KBD_PORT, ROW3, GPIO_MODE_OUT_PP_HIGH_SLOW);
    GPIO_Init(KBD_PORT, ROW4, GPIO_MODE_OUT_PP_HIGH_SLOW);

    // Столбцы - входы с подтяжкой
    GPIO_Init(KBD_PORT, COL1, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(KBD_PORT, COL2, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(KBD_PORT, COL3, GPIO_MODE_IN_PU_NO_IT);
}

static void row_set(GPIO_Pin_TypeDef row, unsigned char level) {
    if (level) GPIO_WriteHigh(KBD_PORT, row);
    else       GPIO_WriteLow(KBD_PORT, row);
}

static unsigned char col_read(GPIO_Pin_TypeDef col) {
    return (GPIO_ReadInputPin(KBD_PORT, col) == RESET) ? 1 : 0;
}

char keyboard_get_key(void) {
    safe_check();
    last_key = KEY_NONE;

    for (unsigned char r = 0; r < 4; r++) {
        row_set(ROW1, 1);
        row_set(ROW2, 1);
        row_set(ROW3, 1);
        row_set(ROW4, 1);
        row_set(rows[r], 0); // Активируем текущую строку

        for (unsigned char c = 0; c < 3; c++) {
            if (col_read(cols[c])) {
                // Антидребезг
                volatile unsigned int i;
                for (i = 0; i < 2000; i++);

                if (col_read(cols[c])) {
                    last_key = keymap[r][c];
                    while (col_read(cols[c])) wd_reset(); // Ждём отпускания

                    row_set(ROW1, 1); row_set(ROW2, 1); row_set(ROW3, 1); row_set(ROW4, 1);
                    return last_key;
                }
            }
        }
    }

    row_set(ROW1, 1); row_set(ROW2, 1); row_set(ROW3, 1); row_set(ROW4, 1);
    return KEY_NONE;
}

char keyboard_refresh_key(void) { return keyboard_get_key(); }
char keyboard_last_key(void) { return last_key; }