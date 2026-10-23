#include "show.h"
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_i2c.h"
#include "file.h"
#include "keyboard.h"    
#include "measure.h"     
#include "charge.h"
#include "wd.h" // Для wd_reset в emergency

// Настройки LCD
#define LCD_ADDR 0x38
#define LCD_BL   0x00
#define LCD_EN   0x04
#define LCD_RS   0x01

// --- Внутренние функции LCD ---
static void lcd_send_nibble(unsigned char nibble, unsigned char rs) {
    unsigned char data = (nibble << 4) | LCD_BL | (rs ? 0x01 : 0); // BL=1

    I2C_GenerateSTART(ENABLE);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(LCD_ADDR << 1, I2C_DIRECTION_TX);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(data | 0x04); // EN=1
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // Задержка для строба (как в тесте)
    for (volatile unsigned int i = 0; i < 500; i++);

    I2C_SendData(data & ~0x04); // EN=0
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(ENABLE);
}

static void lcd_send_byte(unsigned char data, unsigned char rs) {
    lcd_send_nibble(data >> 4, rs);
    lcd_send_nibble(data & 0x0F, rs);
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
    unsigned char addr = (row == 0) ? 0x80 : 0xC0;
    lcd_send_byte(addr + col, 0);
    
    // Задержка для применения команды
    volatile unsigned int i; for (i = 0; i < 2000U; i++); 
}

// Перекодировка UTF-8 в знакогенератор (страница 0x2A)
static uint8_t ru_lcd_utf8(const char** p) {
    uint8_t b1 = (uint8_t)**p;

    if (b1 < 0x80) { (*p)++; return b1; } // ASCII

    if ((b1 == 0xD0 || b1 == 0xD1) && ((uint8_t)(*p)[1] & 0xC0) == 0x80) {
        uint8_t b2 = (uint8_t)(*p)[1];
        uint16_t code = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
        *p += 2;

        if (code == 0x0401 || code == 0x0451) return 0xC5; // Ё/ё -> Е
        if (code >= 0x0410 && code <= 0x042F) return 0xC0 + (code - 0x0410); // А..Я
        if (code >= 0x0430 && code <= 0x044F) return 0xC0 + (code - 0x0430); // а..я -> А..Я
        return '?';
    }

    (*p)++; return '?';
}

void show_init(void) {
    // 1. Ждём питания
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 2. Три раза 0x03
    lcd_send_nibble(0x03, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    lcd_send_nibble(0x03, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    lcd_send_nibble(0x03, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 3. Переход в 4-битный режим
    lcd_send_nibble(0x02, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 4. Function Set (0x28)
    lcd_send_byte(0x28, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 5. Display ON (0x0C)
    lcd_send_byte(0x0C, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 6. Clear (0x01)
    lcd_send_byte(0x01, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 7. Entry mode (0x06)
    lcd_send_byte(0x06, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);

    // 8. Страница 0x2A (русский)
    lcd_send_byte(0x2A, 0);
    for (volatile unsigned long i = 0; i < 100000UL; i++);
}

void show_send_string(BYTE col, BYTE row, const char* str) {
    unsigned char addr = (row == 0) ? 0x80 : 0xC0;
    lcd_send_byte(addr + col, 0);
    { volatile unsigned int i; for (i = 0; i < 5000U; i++); } // LCD должен переварить

    const char* p = str;
    while (*p) {
        lcd_send_byte(ru_lcd_utf8(&p), 1);
    }
}

void show_clear(void) {
    lcd_send_byte(0x01, 0);
    { volatile unsigned int i; for (i = 0; i < 10000U; i++); }
}

ANSWER_TYPE show_wait_command(void) {
    show_clear();
    show_send_string(0, 0, "Для старта жми #");

    while (1) {
        char key = keyboard_get_key();
        if (key == KEY_ESC) return ANSWER_ESC;
        if (key == KEY_ENTER) return ANSWER_ENTER;
    }
}

MODE_TYPE show_get_mode(void) {
    show_clear();
    show_send_string(0, 0, "ГЕНЕРАЦИЯ - 1");
    show_send_string(0, 1, "РАЗРЯД - 0");

    while (1) {
        char key = keyboard_get_key();
        if (key == '1') return WORK;
        if (key == '0') return DISCHARGE;
        if (key == '3') return SET_KI;
    }
}

static ANSWER_TYPE show_input_value(BYTE digits, WORD* value, BYTE x, BYTE y, BYTE fraction) {
    if (digits == 0 || digits > 5) return ANSWER_ESC;

    DWORD divider = 1;
    for (BYTE i = 0; i < digits; ++i) divider *= 10;

    WORD fraction_divider = 1;
    for (BYTE i = 0; i < fraction; ++i) fraction_divider *= 10;

    char buf[8];

    while (1) {
        WORD whole = *value / fraction_divider;
        WORD frac = *value % fraction_divider;
        BYTE pos = 0;
        
        BYTE whole_digits = digits - fraction;
        DWORD tmp = whole;
        for (int i = whole_digits - 1; i >= 0; --i) { buf[i] = '0' + (tmp % 10); tmp /= 10; }
        pos = whole_digits;

        if (fraction > 0) {
            buf[pos++] = '.';
            DWORD f_tmp = frac;
            for (int i = fraction - 1; i >= 0; --i) { buf[pos + i] = '0' + (f_tmp % 10); f_tmp /= 10; }
            pos += fraction;
        }
        buf[pos] = 0;

        show_send_string(x, y, buf);

        char key = keyboard_get_key();
        if (key == KEY_ENTER) return ANSWER_ENTER;
        if (key == KEY_ESC) return ANSWER_ESC;
        if (key < '0' || key > '9') continue;

        *value %= (WORD)divider; 
        *value *= 10;
        *value += (key - '0');
    }
}

ANSWER_TYPE show_input_amperage(WORD* amperage) {
    show_clear();
    show_send_string(0, 0, "ВВОД ТОКА:  ");
    show_send_string(10, 1, " А");
    
    float max_amp = measure_get_max_amperage_for_impedance();
    if (*amperage > max_amp) *amperage = (WORD)max_amp;

    return show_input_value(4, amperage, 5, 1, 0);
}

ANSWER_TYPE show_input_length(WORD* len) {
    show_clear();
    show_send_string(0, 0, "ДЛИТЕЛЬНОСТЬ:  ");
    show_send_string(12, 1, " МС");
    return show_input_value(3, len, 7, 1, 1);
}

ANSWER_TYPE show_get_prec_amperage(WORD* value) {
    show_clear();
    show_send_string(0, 0, "ВВЕДИТ. ИЗМЕРЕН.");
    show_send_string(0, 1, "ТОК:     А");
    return show_input_value(4, value, 10, 1, 0);
}

ANSWER_TYPE show_set_ki(void) {
    show_clear();
    show_send_string(0, 0, "ВВОД KI:  ");

    const WORD SCALE = 1000;
    WORD ki = (WORD)(file_read_ki() * SCALE);
    ANSWER_TYPE answer = show_input_value(4, &ki, 5, 1, 3);
    file_write_ki((float)ki / SCALE);
    return answer;
}

void show_result(WORD amperage) {
    show_clear();
    show_send_string(0, 0, "ИМПУЛЬС:");
    show_send_string(5, 1, "А");

    char buf[6];
    WORD amp = amperage;
    buf[3] = '0' + (amp % 10); amp /= 10;
    buf[2] = '0' + (amp % 10); amp /= 10;
    buf[1] = '0' + (amp % 10); amp /= 10;
    buf[0] = '0' + (amp % 10);
    buf[4] = 0;
    show_send_string(0, 1, buf);

    BYTE len = measure_get_impulse_len_measured();
    BYTE whole = len / PULSE_DURATION_SCALE;
    BYTE frac = len % PULSE_DURATION_SCALE;

    buf[0] = '0' + (whole / 10) % 10;
    buf[1] = '0' + whole % 10;
    buf[2] = '.';
    buf[3] = '0' + frac;
    buf[4] = 0;
    show_send_string(8, 1, buf);
    show_send_string(13, 1, "МС");

    while (1) {
        if (keyboard_get_key() != KEY_NONE) return;
    }
}

ANSWER_TYPE show_charging(WORD amperage) {
    show_clear();
    show_send_string(0, 0, "ФОРМИРОВАТЬ:    ");
    { volatile unsigned int i; for (i = 0; i < 5000U; i++); }

    WORD percent = 0, old_percent = 0;
    float charge = 0;
    BYTE charge_cntr = 0, discharge_cntr = 0;
    const BYTE TRY_LIMIT = 3;
    BYTE state = 0; // 0 = заряд, 1 = разряд

    while (1) {
        wd_reset();
        percent = measure_get_charge_percent(amperage, &charge);

        if (percent != old_percent) {
            char buf[14];
            WORD p = percent / 10;
            
            buf[0] = '0' + (p / 100) % 10;
            buf[1] = '0' + (p / 10) % 10;
            buf[2] = '0' + p % 10;
            buf[3] = '%'; buf[4] = ' '; buf[5] = ' '; buf[6] = ' ';

            int v_int = (int)charge;
            int v_frac = (int)((charge - (float)v_int) * 100.0f);
            if (v_frac < 0) v_frac = -v_frac;

            buf[7] = '0' + (v_int / 10) % 10;
            buf[8] = '0' + v_int % 10;
            buf[9] = '.';
            buf[10] = '0' + (v_frac / 10) % 10;
            buf[11] = '0' + v_frac % 10;
            buf[12] = 'V'; buf[13] = 0;

            show_send_string(0, 1, buf);
            { volatile unsigned int i; for (i = 0; i < 5000U; i++); }
        }
        old_percent = percent;

        if (percent > 1000) {
            if (state != 1) {
                ++discharge_cntr;
                show_send_string(0, 0, "РАЗРЯДИТЬ:      ");
                { volatile unsigned int i; for (i = 0; i < 5000U; i++); }
            }
            if (percent > 1010) charge_discharge();
            else                charge_discharge_slow();

            if (discharge_cntr > TRY_LIMIT) return ANSWER_ENTER;
            state = 1;
        } else if (percent < 999) {
            if (state != 0) {
                ++charge_cntr;
                show_send_string(0, 0, "ФОРМИРОВАТЬ:    ");
                { volatile unsigned int i; for (i = 0; i < 5000U; i++); }
            }
            charge_charge();
            if (charge_cntr > TRY_LIMIT) return ANSWER_ENTER;
            state = 0;
        } else {
            charge_stop();
            return ANSWER_ENTER;
        }

        if (keyboard_get_key() == KEY_ESC) {
            charge_stop();
            return ANSWER_ESC;
        }
    }
}

void show_discharge(void) {
    show_clear();
    show_send_string(0, 0, "РАЗРЯД...");
    { volatile unsigned int i; for (i = 0; i < 5000U; i++); }
    
    charge_discharge();

    while (1) {
        wd_reset();
        float voltage = measure_get_capacitor_voltage();
        char buf[8];
        int v_int = (int)voltage;
        int v_frac = (int)((voltage - (float)v_int) * 100.0f);
        if (v_frac < 0) v_frac = -v_frac;
        if (v_int > 99) v_int = 99;

        buf[0] = '0' + (v_int / 10) % 10; buf[1] = '0' + v_int % 10;
        buf[2] = '.';
        buf[3] = '0' + (v_frac / 10) % 10; buf[4] = '0' + v_frac % 10;
        buf[5] = 'V'; buf[6] = ' '; buf[7] = 0;

        show_send_string(0, 1, buf);
        { volatile unsigned int i; for (i = 0; i < 20000U; i++); }

        if (keyboard_get_key() == KEY_ESC) {
            charge_stop();
            return;
        }
    }
}

void show_emergency(void) {
    charge_discharge();

    while (1) {
        show_clear();
        show_send_string(0, 0, "ТРЕВОГА! РАЗРЯД");
        show_send_string(0, 1, "СНЯТЬ НАГР.!");
        
        // ~1 сек задержка
        { volatile unsigned int i; for (i = 0; i < 60000U; i++); wd_reset(); }

        show_clear();
        { volatile unsigned int i; for (i = 0; i < 60000U; i++); wd_reset(); }
    }
}