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
    uint8_t Bkey = 0;
    uint8_t key = 0xFFU;
    uint8_t keymap[4][4]{
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };

    for(uint8_t f = 0U; f < 4U; f++){//                     Barrido para Filas
        GPIOB->BSRR = ( 0xFU );
        GPIOB->BBR = (0x1UL << f);

        for(uint8_t c = 0U; c < 4U; c++){//                 Barrido para Columnas
            if(GPIOB->IDR & (0x1UL << (c + 4))){
                while(!(GPIOB->IDR & (0x1UL << (c + 4))));
                
                key = keymap[f][c];
                break;
            }
        }
    if (key != 0xFFU) break;
    }

    if (key != 0xFFU) {
        if (!Bkey){
            key = key; 
            Bkey = 1;
        } else {
            key = 0xFFU; 
        }
    } 
    else {
        Bkey = 0;
        key = 0xFFU;
    }

    return key; 
}
            


