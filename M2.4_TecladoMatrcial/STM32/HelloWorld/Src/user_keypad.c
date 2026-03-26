#include <stdint.h>
#include "main.h"
#include "user_keypad.h"

void USER_Keypad_Init( void ){
    RCC->APB2ENR |= ( 0x1UL << 2U );

    GPIOA->ODR & = ~( 0x1UL << 0U );//	Reset ODR0 bit
    GPIOA->ODR & = ~( 0x1UL << 1U );//	Reset ODR1 bit
    GPIOA->ODR & = ~( 0x1UL << 2U );//	Reset ODR2 bit
    GPIOA->ODR & = ~( 0x1UL << 3U );//	Reset ODR3 bit
    GPIOA->ODR & = ~( 0x1UL << 4U );//	Reset ODR4 bit
    GPIOA->ODR & = ~( 0x1UL << 5U );//	Reset ODR5 bit
    GPIOA->ODR & = ~( 0x1UL << 6U );//	Reset ODR6 bit
    GPIOA->ODR & = ~( 0x1UL << 7U );//	Reset ODR7 bit

    GPIOA->CRL  = GPIOA->CRL & ~( 0x3UL << 2U )
                             & ~( 0x2UL << 0U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x3UL << 6U )
                             & ~( 0x2UL << 4U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x3UL << 10U )
                             & ~( 0x2UL << 8U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x3UL << 14U )
                             & ~( 0x2UL << 12U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x1UL << 18U )
                             & ~( 0x3UL << 16U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x1UL << 22U )
                             & ~( 0x3UL << 20U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x1UL << 26U )
                             & ~( 0x3UL << 24U );
    GPIOA->CRL  = GPIOA->CRL & ~( 0x1UL << 30U )
                             & ~( 0x3UL << 28U );

    GPIOA->CRL  |= ( 0x1UL << 0U );
    GPIOA->CRL  |= ( 0x1UL << 4U );
    GPIOA->CRL  |= ( 0x1UL << 8U );
    GPIOA->CRL  |= ( 0x1UL << 12U );
    GPIOA->CRL  |= ( 0x1UL << 16U );
    GPIOA->CRL  |= ( 0x1UL << 20U );
    GPIOA->CRL  |= ( 0x1UL << 24U );
    GPIOA->CRL  |= ( 0x1UL << 28U );
}

uint8_t USER_Key( void ){
    uint8_t key = 0xFFU;

    for( uint8_t i = 0U; i < 7U; i++ )
        if( ( GPIOA->IDR & ( 0x1UL << i ) ) == 0U ){
            while( ( GPIOA->IDR & ( 0x1UL << i ) ) == 0U );
            key = iU;
            break;
        }
    return key;
}
            


