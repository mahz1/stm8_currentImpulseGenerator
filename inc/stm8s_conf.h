#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

#include "stm8s.h"

/* Определяем assert_param, чтобы библиотека не искала функцию assert_failed */
#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif

/* Раскомментируй те модули, которые используешь */
#define _I2C  (1)
#define _GPIO (1)
#define _CLK  (1)
#define _ADC1 (1)
#define _TIM1 (1)
#define _IWDG (1)
#define _FLASH (1)
//#define _TIM2 (1)
//#define _TIM4 (1)
//#define _UART2 (1)

#endif /* __STM8S_CONF_H */