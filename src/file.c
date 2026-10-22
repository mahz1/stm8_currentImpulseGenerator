#include "file.h"
#include "stm8s.h"
#include "stm8s_flash.h"

// Сигнатура для проверки валидности EEPROM
#define FILE_GUARD  0x14082026UL

typedef struct {
    DWORD guard;
    float Ki;
} SFileRecord;

// Начало EEPROM в STM8S105C6
#define FILE_ADDR  ((uint16_t)0x4000)

static SFileRecord record;

static uint8_t eeprom_read_byte(uint16_t addr) {
    return *(uint8_t*)addr;
}

static void eeprom_write_byte(uint16_t addr, uint8_t value) {
    // Разблокировка Data EEPROM
    if (!(FLASH->IAPSR & FLASH_IAPSR_DUL)) {
        FLASH->DUKR = 0xAE;
        FLASH->DUKR = 0x56;
        while (!(FLASH->IAPSR & FLASH_IAPSR_DUL));
    }

    *(uint8_t*)addr = value;

    // Ожидание завершения записи
    while (!(FLASH->IAPSR & FLASH_IAPSR_EOP));
    FLASH->IAPSR &= ~FLASH_IAPSR_EOP;
}

static void eeprom_read_record(void) {
    uint8_t* p = (uint8_t*)&record;
    for (uint8_t i = 0; i < sizeof(SFileRecord); i++) {
        p[i] = eeprom_read_byte(FILE_ADDR + i);
    }
}

static void eeprom_write_record(void) {
    uint8_t* p = (uint8_t*)&record;
    for (uint8_t i = 0; i < sizeof(SFileRecord); i++) {
        eeprom_write_byte(FILE_ADDR + i, p[i]);
    }
}

void file_init(void) {
    eeprom_read_record();

    if (record.guard == FILE_GUARD) return;

    // Первый запуск или мусор в памяти — инициализация дефолтными значениями
    record.guard = FILE_GUARD;
    record.Ki    = 1.008f;
    eeprom_write_record();
}

float file_read_ki(void) {
    return record.Ki;
}

void file_write_ki(float value) {
    record.Ki = value;
    eeprom_write_record();
}