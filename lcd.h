#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdint.h>

/* ----------------------------------------------------------------
 * Mapeo de pines
 *   RS  = PB8    EN  = PB10
 *   D4  = PB12   D5  = PB13   D6  = PB5   D7  = PB6
 *
 * BSRR: bits [15:0] = SET, bits [31:16] = RESET
 *   SET  pin n  →  (1UL << n)
 *   RESET pin n →  (1UL << (n+16))
 * ---------------------------------------------------------------- */

/* RS — PB8 */
#define LCD_RS_PIN_HIGH     (1UL <<  8)
#define LCD_RS_PIN_LOW      (1UL << 24)

/* EN — PB10 */
#define LCD_EN_PIN_HIGH     (1UL << 10)
#define LCD_EN_PIN_LOW      (1UL << 26)

/* D4 — PB12 */
#define LCD_D4_PIN_HIGH     (1UL << 12)
#define LCD_D4_PIN_LOW      (1UL << 28)

/* D5 — PB13 */
#define LCD_D5_PIN_HIGH     (1UL << 13)
#define LCD_D5_PIN_LOW      (1UL << 29)

/* D6 — PB5 */
#define LCD_D6_PIN_HIGH     (1UL <<  5)
#define LCD_D6_PIN_LOW      (1UL << 21)

/* D7 — PB6 */
#define LCD_D7_PIN_HIGH     (1UL <<  6)
#define LCD_D7_PIN_LOW      (1UL << 22)

/* ----------------------------------------------------------------
 * Comandos
 * ---------------------------------------------------------------- */
#define LCD_Clear()         LCD_Write_Cmd(0x01U)
#define LCD_Display_ON()    LCD_Write_Cmd(0x0CU)
#define LCD_Display_OFF()   LCD_Write_Cmd(0x08U)
#define LCD_Cursor_Home()   LCD_Write_Cmd(0x02U)
#define LCD_Cursor_Blink()  LCD_Write_Cmd(0x0FU)
#define LCD_Cursor_ON()     LCD_Write_Cmd(0x0EU)
#define LCD_Cursor_OFF()    LCD_Write_Cmd(0x0CU)
#define LCD_Cursor_Left()   LCD_Write_Cmd(0x10U)
#define LCD_Cursor_Right()  LCD_Write_Cmd(0x14U)
#define LCD_Cursor_SLeft()  LCD_Write_Cmd(0x18U)
#define LCD_Cursor_SRight() LCD_Write_Cmd(0x1CU)

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */
void LCD_Init(void);
void LCD_Out_Data4(uint8_t val);
void LCD_Pulse_EN(void);
void LCD_Write_Byte(uint8_t val);
void LCD_Write_Cmd(uint8_t val);
void LCD_Put_Char(uint8_t c);
void LCD_Set_Cursor(uint8_t line, uint8_t column);
void LCD_Put_Str(char *str);
void LCD_Put_Num(int16_t num);
char LCD_Busy(void);
void LCD_BarGraphic(int16_t value, int16_t size);
void LCD_BarGraphicXY(int16_t pos_x, int16_t pos_y, int16_t value);

#endif /* INC_LCD_H_ */