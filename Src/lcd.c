#include <stdint.h>
#include "main.h"
#include "lcd.h"

/* ================================================================
 * LCD_Out_Data4 — envía el nibble bajo de val a D4-D7
 *
 *   D4 = PB12   D5 = PB13   D6 = PB5   D7 = PB6
 * ================================================================ */
void LCD_Out_Data4(uint8_t val)
{
    GPIOB->BSRR = (val & 0x01U) ? LCD_D4_PIN_HIGH : LCD_D4_PIN_LOW;
    GPIOB->BSRR = (val & 0x02U) ? LCD_D5_PIN_HIGH : LCD_D5_PIN_LOW;
    GPIOB->BSRR = (val & 0x04U) ? LCD_D6_PIN_HIGH : LCD_D6_PIN_LOW;
    GPIOB->BSRR = (val & 0x08U) ? LCD_D7_PIN_HIGH : LCD_D7_PIN_LOW;
}

/* ================================================================
 * LCD_Pulse_EN — genera un pulso en EN
 * ================================================================ */
void LCD_Pulse_EN(void)
{
    GPIOB->BSRR = LCD_EN_PIN_HIGH;
    delay_us(5);
    GPIOB->BSRR = LCD_EN_PIN_LOW;
    delay_us(5);
}

/* ================================================================
 * LCD_Write_Byte — envía un byte completo en modo 4-bit
 * ================================================================ */
void LCD_Write_Byte(uint8_t val)
{
    LCD_Out_Data4((val >> 4) & 0x0FU);
    LCD_Pulse_EN();
    LCD_Out_Data4(val & 0x0FU);
    LCD_Pulse_EN();
    delay_us(50);
}

/* ================================================================
 * LCD_Write_Cmd — envía un comando (RS=0)
 * ================================================================ */
void LCD_Write_Cmd(uint8_t val)
{
    GPIOB->BSRR = LCD_RS_PIN_LOW;
    delay_us(1);
    LCD_Write_Byte(val);
    if (val == 0x01U || val == 0x02U)
        delay_ms(2);
}

/* ================================================================
 * LCD_Put_Char — envía un dato (RS=1)
 * ================================================================ */
void LCD_Put_Char(uint8_t c)
{
    GPIOB->BSRR = LCD_RS_PIN_HIGH;
    delay_us(1);
    LCD_Write_Byte(c);
}

/* ================================================================
 * LCD_Init
 *
 * Pines y sus registros de configuración:
 *
 * CRL (PB0-PB7):
 *   PB5 (D6) → bits [23:20]   MODE=10 CNF=00 → 0x2
 *   PB6 (D7) → bits [27:24]   MODE=10 CNF=00 → 0x2
 *
 * CRH (PB8-PB15):
 *   PB8  (RS) → bits [3:0]    MODE=10 CNF=00 → 0x2
 *   PB10 (EN) → bits [11:8]   MODE=10 CNF=00 → 0x2
 *   PB12 (D4) → bits [19:16]  MODE=10 CNF=00 → 0x2
 *   PB13 (D5) → bits [23:20]  MODE=10 CNF=00 → 0x2
 * ================================================================ */
void LCD_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* CRL: PB5 y PB6 como salida PP 2 MHz */
    GPIOB->CRL &= ~((0xFUL << 20) | (0xFUL << 24));
    GPIOB->CRL |=  ((0x2UL << 20) | (0x2UL << 24));

    /* CRH: PB8, PB10, PB12, PB13 como salida PP 2 MHz
     *
     * CRH bit positions para PBn (n=8..15): bits [(n-8)*4+3 : (n-8)*4]
     *   PB8  → bits [3:0]
     *   PB9  → bits [7:4]   (no usado)
     *   PB10 → bits [11:8]
     *   PB11 → bits [15:12] (no usado)
     *   PB12 → bits [19:16]
     *   PB13 → bits [23:20]
     */
    GPIOB->CRH &= ~((0xFUL <<  0) | (0xFUL <<  8) |
                    (0xFUL << 16) | (0xFUL << 20));
    GPIOB->CRH |=  ((0x2UL <<  0) | (0x2UL <<  8) |
                    (0x2UL << 16) | (0x2UL << 20));

    /* Todos los pines del LCD a LOW */
    GPIOB->BSRR = LCD_RS_PIN_LOW | LCD_EN_PIN_LOW
                | LCD_D4_PIN_LOW | LCD_D5_PIN_LOW
                | LCD_D6_PIN_LOW | LCD_D7_PIN_LOW;

    /* Espera de encendido ≥ 40 ms */
    delay_ms(50);

    /* --- Secuencia de reset por software (HD44780 Fig. 24) --- */

    /* Paso A: nibble 0x3, espera ≥ 4.1 ms */
    LCD_Out_Data4(0x03U);
    LCD_Pulse_EN();
    delay_ms(5);

    /* Paso B: nibble 0x3, espera ≥ 100 µs */
    LCD_Out_Data4(0x03U);
    LCD_Pulse_EN();
    delay_us(200);

    /* Paso C: nibble 0x3 */
    LCD_Out_Data4(0x03U);
    LCD_Pulse_EN();
    delay_us(200);

    /* Paso D: nibble 0x2 → activa modo 4-bit */
    LCD_Out_Data4(0x02U);
    LCD_Pulse_EN();
    delay_us(200);

    /* --- Configuración estándar --- */
    LCD_Write_Cmd(0x28U);   /* Function Set: 4-bit, 2 líneas, 5×8 */
    LCD_Write_Cmd(0x08U);   /* Display OFF */
    LCD_Write_Cmd(0x01U);   /* Clear Display */
    LCD_Write_Cmd(0x06U);   /* Entry Mode: cursor right, no shift */
    LCD_Write_Cmd(0x0CU);   /* Display ON, cursor OFF */
}

/* ================================================================
 * LCD_Set_Cursor — posiciona el cursor (base 1)
 * ================================================================ */
void LCD_Set_Cursor(uint8_t line, uint8_t column)
{
    uint8_t addr = ((line - 1U) * 0x40U) + (column - 1U);
    LCD_Write_Cmd(0x80U | (addr & 0x7FU));
}

/* ================================================================
 * LCD_Put_Str — imprime string (hasta 16 chars)
 * ================================================================ */
void LCD_Put_Str(char *str)
{
    for (uint8_t i = 0; i < 16U && str[i] != '\0'; i++)
        LCD_Put_Char((uint8_t)str[i]);
}

/* ================================================================
 * LCD_Put_Num — imprime entero sin ceros a la izquierda
 * ================================================================ */
void LCD_Put_Num(int16_t num)
{
    if (num < 0) { LCD_Put_Char('-'); num = -num; }
    const int16_t mag[5] = { 10000, 1000, 100, 10, 1 };
    uint8_t leading = 0;
    for (uint8_t i = 0; i < 5U; i++)
    {
        int8_t d = (int8_t)(num / mag[i]);
        num -= d * mag[i];
        if (d) leading = 1;
        if (leading || i == 4U)
            LCD_Put_Char((uint8_t)('0' + d));
    }
}

char LCD_Busy(void) { return 0; }

/* ================================================================
 * Gráficas de barra
 * ================================================================ */
void LCD_BarGraphic(int16_t value, int16_t size)
{
    value = value * size / 20;
    for (int16_t i = 0; i < size; i++)
    {
        if (value > 5) { LCD_Put_Char(0x05U); value -= 5; }
        else
        {
            LCD_Put_Char((uint8_t)value);
            for (int16_t j = i + 1; j < size; j++) LCD_Put_Char(0x06U);
            break;
        }
    }
}

void LCD_BarGraphicXY(int16_t pos_x, int16_t pos_y, int16_t value)
{
    LCD_Set_Cursor((uint8_t)pos_x, (uint8_t)pos_y);
    for (int16_t i = 0; i < 16; i++)
    {
        if (value > 5) { LCD_Put_Char(0x05U); value -= 5; }
        else
        {
            LCD_Put_Char((uint8_t)value);
            value = 0;
            while (++i < 16) LCD_Put_Char(0x06U);
            break;
        }
    }
}