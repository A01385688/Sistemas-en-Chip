#include <stdint.h>
#include "main.h"
#include "user_keypad.h"
// Configuración Inicial de los Pines
void USER_Keypad_Init( void ){
    RCC->APB2ENR |= ( 0x1UL << 3U );

    GPIOB->ODR & = ~( 0x1UL << 0U );//	Reset ODR0 bit
    GPIOB->ODR & = ~( 0x1UL << 1U );//	Reset ODR1 bit
    GPIOB->ODR & = ~( 0x1UL << 2U );//	Reset ODR2 bit
    GPIOB->ODR & = ~( 0x1UL << 3U );//	Reset ODR3 bit
    GPIOB->ODR & = ~( 0x1UL << 4U );//	Reset ODR4 bit
    GPIOB->ODR & = ~( 0x1UL << 5U );//	Reset ODR5 bit
    GPIOB->ODR & = ~( 0x1UL << 6U );//	Reset ODR6 bit
    GPIOB->ODR & = ~( 0x1UL << 7U );//	Reset ODR7 bit

    //Limpiamos el registro CRL
    GPIOB->CRL  = GPIOA->CRL & ~( 0x3UL << 2U )//       Filas (Outputs)
                             & ~( 0x2UL << 0U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x3UL << 6U )
                             & ~( 0x2UL << 4U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x3UL << 10U )
                             & ~( 0x2UL << 8U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x3UL << 14U )
                             & ~( 0x2UL << 12U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x1UL << 18U )//      Columnas (Inputs)
                             & ~( 0x3UL << 16U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x1UL << 22U )
                             & ~( 0x3UL << 20U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x1UL << 26U )
                             & ~( 0x3UL << 24U );
    GPIOB->CRL  = GPIOA->CRL & ~( 0x1UL << 30U )
                             & ~( 0x3UL << 28U );

    //Configuramos el registro CRL para Inputs y Ouputs
    GPIOB->CRL  |= ( 0x1UL << 0U );//       Filas (Outputs) - PB0
    GPIOB->CRL  |= ( 0x1UL << 4U );//       PB1
    GPIOB->CRL  |= ( 0x1UL << 8U );//       PB2
    GPIOB->CRL  |= ( 0x1UL << 12U );//      PB3
    GPIOB->CRL  |= ( 0x1UL << 19U );//      Columnas (Inputs) - PB4
    GPIOB->CRL  |= ( 0x1UL << 23U );//      PB5
    GPIOB->CRL  |= ( 0x1UL << 27U );//      PB6
    GPIOB->CRL  |= ( 0x1UL << 31U );//      PB7
}
// Barrido de Filas y Columnas
uint8_t USER_Key( void ){
    uint8_t Bkey = 0;//             Bandera inicializada en 0
    uint8_t key = 0xFFU;//          Inicialización de key con valor nulo
    uint8_t keymap[4][4]{//         Keymap: Traducción del barrido
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };
    
    for(uint8_t f = 0U; f < 4U; f++){//     Barrido para Filas
        GPIOB->BSRR = ( 0xFU );//           Pines 0 a 3 en HIGH
        GPIOB->BBR = (0x1UL << f);//        Fila actual LOW

        for(uint8_t c = 0U; c < 4U; c++){//                 Barrido para Columnas
            if(GPIOB->IDR & (0x1UL << (c + 4))){//          Condición de busqueda: Circuito Cerrado = 0
                
                key = keymap[f][c];//                       Asignación al key del valor del keymap
                break;//                                    Terminamos la ejecución de la busqueda
            }
        }
    if (key != 0xFFU) break;//                              Terminamos la busqueda general
    }

    if (key != 0xFFU) {//               Inicio de la lógica de la Bandera - Detección de una tecla presionada
        if (!Bkey){//                   Bandera = 0 -> Detectamos un boton activo
            key = key; 
            Bkey = 1;
        } else {//                      El boton continua activo
            key = 0xFFU;//              No se devuelve nada
        }
    } 
    else {//                            No hay boton presionada
        Bkey = 0;//                     Reinicio de la bandera
        key = 0xFFU;//
    }

    return key; 
}
            


