#include <stdint.h>
#include "main.h"
#include "user_keypad.h"

void USER_Keypad_Init( void ){
    // RCC_APB2ENR modified to IO port B clock enable
    RCC->APB2ENR |= ( 0x1UL << 3U );

    GPIOB->ODR & = ~( 0x1UL << 0U );//	Reset ODR0 bit
    GPIOB->ODR & = ~( 0x1UL << 1U );//	Reset ODR1 bit
    GPIOB->ODR & = ~( 0x1UL << 2U );//	Reset ODR2 bit
    GPIOB->ODR & = ~( 0x1UL << 3U );//	Reset ODR3 bit
    GPIOB->ODR & = ~( 0x1UL << 4U );//	Reset ODR4 bit
    GPIOB->ODR & = ~( 0x1UL << 5U );//	Reset ODR5 bit
    GPIOB->ODR & = ~( 0x1UL << 6U );//	Reset ODR6 bit
    GPIOB->ODR & = ~( 0x1UL << 7U );//	Reset ODR7 bit

    GPIOB->CRL  = GPIOB->CRL & ~( 0x3UL << 2U )
                             & ~( 0x2UL << 0U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x3UL << 6U )
                             & ~( 0x2UL << 4U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x3UL << 10U )
                             & ~( 0x2UL << 8U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x3UL << 14U )
                             & ~( 0x2UL << 12U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x1UL << 18U )
                             & ~( 0x3UL << 16U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x1UL << 22U )
                             & ~( 0x3UL << 20U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x1UL << 26U )
                             & ~( 0x3UL << 24U );
    GPIOB->CRL  = GPIOB->CRL & ~( 0x1UL << 30U )
                             & ~( 0x3UL << 28U );

    GPIOB->CRL  |= ( 0x1UL << 0U );
    GPIOB->CRL  |= ( 0x1UL << 4U );
    GPIOB->CRL  |= ( 0x1UL << 8U );
    GPIOB->CRL  |= ( 0x1UL << 12U );
    GPIOB->CRL  |= ( 0x1UL << 16U );
    GPIOB->CRL  |= ( 0x1UL << 20U );
    GPIOB->CRL  |= ( 0x1UL << 24U );
    GPIOB->CRL  |= ( 0x1UL << 28U );
}

uint8_t USER_Key( void ){
    uint8_t key = 0xFFU;

    for( uint8_t i = 0U; i < 7U; i++ );
        if( ( GPIOB->IDR & ( 0x1UL << i ) ) == 0U ){
            while( ( GPIOB->IDR & ( 0x1UL << i ) ) == 0U );
        }
            key = iU;
            break;

    return key;
}