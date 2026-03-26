#include <stdint.h>
#include "main.h"
#include "user_keypad.h"
// Configuración Inicial de los Pines
void USER_Keypad_Init( void ){
    RCC->APB2ENR |= ( 0x1UL << 3U );

    GPIOB->ODR = 0x0UL;//               Reset (Limpieza) de los 16 bits

    GPIOB->ODR |= ( 0x1UL << 0U );//	Set ODR0 bit
    GPIOB->ODR |= ( 0x1UL << 1U );//	Set ODR1 bit
    GPIOB->ODR |= ( 0x1UL << 2U );//	Set ODR2 bit
    GPIOB->ODR |= ( 0x1UL << 3U );//	Set ODR3 bit
    GPIOB->ODR |= ( 0x1UL << 4U );//	Set ODR4 bit
    GPIOB->ODR |= ( 0x1UL << 5U );//	Set ODR5 bit
    GPIOB->ODR |= ( 0x1UL << 6U );//	Set ODR6 bit
    GPIOB->ODR |= ( 0x1UL << 7U );//	Set ODR7 bit

    //Limpiamos el registro CRL
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 0U );//       Filas (Outputs)
                             
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 4U );
                             
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 8U );
                             
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 12U );
                             
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 16U );//      Columnas (Inputs)
                             
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 20U );
                            
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 24U );
                            
    GPIOB->CRL  = GPIOB->CRL & ~( 0xFUL << 28U );
                             

    //Configuramos el registro CRL para Inputs y Ouputs
    GPIOB->CRL  |= ( 0x3UL << 0U );//       Filas (Outputs) - PB0
    GPIOB->CRL  |= ( 0x3UL << 4U );//       PB1
    GPIOB->CRL  |= ( 0x3UL << 8U );//       PB2
    GPIOB->CRL  |= ( 0x3UL << 12U );//      PB3
    GPIOB->CRL  |= ( 0x8UL << 16U );//      Columnas (Inputs) - PB4
    GPIOB->CRL  |= ( 0x8UL << 20U );//      PB5
    GPIOB->CRL  |= ( 0x8UL << 24U );//      PB6
    GPIOB->CRL  |= ( 0x8UL << 28U );//      PB7
}
// Barrido de Filas y Columnas
uint8_t USER_Key( void ){
    static uint8_t Bkey = 0;//             Bandera inicializada en 0
    uint8_t key = 0xFFU;//          Inicialización de key con valor nulo
    uint8_t keymap[4][4] = {//         Keymap: Traducción del barrido
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };
    
    for(uint8_t f = 0U; f < 4U; f++){//     Barrido para Filas
        GPIOB->BSRR = ( 0xFU );//           Pines 0 a 3 en HIGH
        GPIOB->BRR = (0x1UL << f);//        Fila actual LOW
        
        for(volatile int i = 0; i < 10; i++);//             Pequeño retardo para estabilidad electrica

        for(uint8_t c = 0U; c < 4U; c++){//                 Barrido para Columnas
            if(!(GPIOB->IDR & (0x1UL << (c + 4)))){//       Condición de busqueda: Circuito Cerrado = 0
                
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
            


