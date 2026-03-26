#include <stdint.h> 
#include "main.h"
#include "user_keypad.h"

void USER_GPIO_Init(void);
void USER_Delay_1sec(void);

int main(void)
{
	USER_GPIO_Init( );//														Se llama a la funcion GPIO_Init
	USER_Keypad_Init( );//														Se llama a la funcion Keypad_Init
	uint8_t tecla_presionada = 0xFFU;//											Variable para la tecla presionada
    for(;;){//																	Bucle y/o Ciclo (Instrucciones que se repetiran)
		tecla_presionada = USER_Key();//										Asignamos el valor devuelto por la funcion USER_key (Indica el boton presionado)
    	if(tecla_presionada != 0xFFU){//										Si la tecla esta precionada entramos al if
            GPIOA->ODR = (GPIOA->ODR & ~0xFUL) | (tecla_presionada & 0xFUL);//	Muestra el resultado en los primeros pines del puerto A 0 - 3 que corresponden a los LEDS
        }
    }
}

void USER_GPIO_Init( void ){//								Reset AND Set de los PinA y la configuración de los mismos
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
void USER_Delay_1sec( void ){//								Logica de la función Delay: Espera 1 segundo
	__asm(" 		ldr r0, =888888	");
	__asm(" again:	sub r0, r0, #1		");
	__asm("			cmp r0, #0			");
	__asm("			bne again			");
	__asm("			nop					");
}


