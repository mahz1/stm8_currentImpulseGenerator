#ifndef KEYBOARD_HEADER_GUARD
#define KEYBOARD_HEADER_GUARD

#include "common.h"

// Специальные символы клавиш
#define KEY_ENTER  '#'
#define KEY_ESC    '*'
#define KEY_NONE   0

// Инициализация пинов
void keyboard_init(void);

// Опрос клавиатуры. Возвращает:
//   символ нажатой клавиши ('0'..'9', '*', '#') или KEY_NONE
char keyboard_get_key(void);

// Устаревшее: оставлено для совместимости с логикой Show::* и Safe::*
char keyboard_refresh_key(void);
char keyboard_last_key(void);

#endif  // KEYBOARD_HEADER_GUARD