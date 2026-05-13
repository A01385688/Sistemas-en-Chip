#ifndef INC_MAIN_H_
#define INC_MAIN_H_

#define STM32F103xB
#include <stdint.h>
#include "stm32f1xx.h"

/* ---------------------------------------------------------------
 * Clock & Timer
 * --------------------------------------------------------------- */
void rcc_init_64MHz(void);
void timer3_init(void);

/* ---------------------------------------------------------------
 * Blocking delays (driven by TIM3 at 64 MHz, PSC=64-1 → 1 µs/tick)
 * --------------------------------------------------------------- */
void delay_us(uint16_t us);
void delay_ms(uint16_t ms);

#endif /* INC_MAIN_H_ */