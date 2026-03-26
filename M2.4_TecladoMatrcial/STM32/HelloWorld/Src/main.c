#include <stdint.h> 
#include "main.h"
#include "user_keypad.h"

void USER_GPIO_Init(void);
void USER_Delay_1sec(void);

int main(void)
{
	USER_GPIO_Init( ); 
	USER_Keypad_Init( ); 
	uint8_t tecla_presionada = 0xFFU;
    for(;;){
		tecla_presionada = USER_Key();
    	if(tecla_presionada != 0xFFU){
            GPIOA->ODR = (GPIOA->ODR & ~0xFUL) | (tecla_presionada & 0xFUL);
        }
    }
}

void USER_GPIO_Init( void ){
	RCC->APB2ENR |= ( 0x1UL << 2U );
	GPIOA->ODR &= ~( 0x1UL << 0U ); 
    GPIOA->ODR &= ~( 0x1UL << 1U ); 
    GPIOA->ODR &= ~( 0x1UL << 2U ); 
    GPIOA->ODR &= ~( 0x1UL << 3U );
	GPIOA->CRL &= ~( 0x3UL << 2U ) & ~( 0x2UL << 0U ); 
    GPIOA->CRL |= ( 0x1UL << 0U );                     
    GPIOA->CRL &= ~( 0x3UL << 6U ) & ~( 0x2UL << 4U );
    GPIOA->CRL |= ( 0x1UL << 4U );
    GPIOA->CRL &= ~( 0x3UL << 10U ) & ~( 0x2UL << 8U );
    GPIOA->CRL |= ( 0x1UL << 8U );
    GPIOA->CRL &= ~( 0x3UL << 14U ) & ~( 0x2UL << 12U );
    GPIOA->CRL |= ( 0x1UL << 12U );
}
void USER_Delay_1sec( void ){
	__asm(" 		ldr r0, =888888	");
	__asm(" again:	sub r0, r0, #1		");
	__asm("			cmp r0, #0			");
	__asm("			bne again			");
	__asm("			nop					");
}


