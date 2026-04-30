#include <stdint.h>
#include "main.h"
#include "user_timer.h"

void PWM_Init(){
    // Paso 1, configuracion del Pin PA0
    GPIOA->CRL = GPIOA->CRL & ~(0xFUL << 0U);    // Primero limpiamos la mascara poniendo ceros
    GPIOA->CRL = GPIOA->CRL |  (0xBUL << 0U);    // El pin ahora esta en alternate function output push pull

    // Paso 1. configuramos el counter mode, el comportamiento de auto recarga y el evento
    // de overflow UEV, colocando los ceros y unos en los bits correspondientes
    TIM2->CR1 = TIM2->CR1 & ~(0x3UL << 8U);
    TIM2->CR1 = TIM2->CR1 & ~(0x7UL << 4U);
    TIM2->CR1 = TIM2->CR1 & ~(0xFUL << 0U);
    // Ahora si lo configuramos poniendo un uno en el bit 7 (registro ARPE)
    TIM2->CR1 = TIM2->CR1 | (0x1UL << 7U);

    // Paso 2. Configuramos el modo PWM de 16 bits
    // Ponemos los ceros primero
    TIM2->CCMR1 = TIM2->CCMR1 & ~(0x7UL << 0U);
    TIM2->CCMR1 = TIM2->CCMR1 & ~(0x9UL << 4U);
    TIM2->CCMR1 = TIM2->CCMR1 & ~(0xFUL << 8U);
    TIM2->CCMR1 = TIM2->CCMR1 & ~(0xFUL << 12U);
    // Ahora ponemos los unos
    TIM2->CCMR1 = TIM2->CCMR1 | (0x1UL << 3U);
    TIM2->CCMR1 = TIM2->CCMR1 | (0x1UL << 5U);
    TIM2->CCMR1 = TIM2->CCMR1 | (0x1UL << 6U);

    // Paso 3. Configuramos los valores del prescalador, el auto reload register
    // y el registro CCR1 con los valores indicamos en user_timer.h
    TIM2->PSC =  TIM2->PSC  | (PSC_VALOR << 0U);  
    TIM2->ARR =  TIM2->ARR  | (ARR_VALOR << 0U);
    TIM2->CCR1 = TIM2->CCR1 | (CCR1_VALOR << 0U);

    // Paso 4. Configuramos el registro del evento UEV para que cargue la
    // cuenta maxima (el periodo), el presacalador y el contador reset
    // Colocamos lo ceros
    TIM2->EGR = TIM2->EGR & ~(0xEUL << 0U);
    TIM2->EGR = TIM2->EGR & ~(0x0UL << 4U);
    TIM2->EGR = TIM2->EGR & ~(0x0UL << 6U);
    // Colocamos los unos
    TIM2->EGR = TIM2->EGR & ~(0x1UL << 0U);

    // Paso 5. Inicializamos el Timer 2, clear the timer overflow UEV-event flag
    // Ponemos los ceros
    TIM2->SR = TIM2->SR & ~(0xFUL << 0U);
    TIM2->SR = TIM2->SR & ~(0x1UL << 4U);
    TIM2->SR = TIM2->SR & ~(0x1UL << 6U);
    TIM2->SR = TIM2->SR & ~(0xFUL << 9U);

    // Paso 6. Enable de PWM signal output and send de polarity
    // Colocamos los ceros
    TIM2->CCER = TIM2->CCER & ~(0x1UL << 1U);
    TIM2->CCER = TIM2->CCER & ~(0x3UL << 4U);
    TIM2->CCER = TIM2->CCER & ~(0x3UL << 8U);
    TIM2->CCER = TIM2->CCER & ~(0x3UL << 12U);
    // Colocamos los unos
    TIM2->CCER = TIM2->CCER | (0x1UL << 0U);

    // Paso 7. Habilitamos el contador para empezar a contar
    // Ponemos los unos
    TIM2->CR1 = TIM2->CR1 | (0x1UL << 0U);
};
