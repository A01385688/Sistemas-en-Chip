/* ============================================================
 * main.c  —  STM32F103xB  @  64 MHz
 *
 * Mapa de pines
 * ─────────────────────────────────────────────────────────────
 *  LCD  : PB5(D6) PB6(D7) PB8(RS) PB10(EN) PB12(D4) PB13(D5)
 *  POT  : PA0  → ADC1_CH0   (aceleración 0–100 %)
 *  BRAKE: PA1  → entrada pull-up  (botón a GND)
 *  TIM2 : interrupción cada 40 ms → ejecuta modelo
 *
 *  PWM LEDs — TIM3 @ 1 kHz
 *    LED1 → PB0  (TIM3_CH3)
 *    LED2 → PB1  (TIM3_CH4)
 *    LED3 → PA6  (TIM3_CH1)
 *    LED4 → PA7  (TIM3_CH2)
 * ============================================================ */

#include "main.h"
#include "lcd.h"
#include "EngTrModel.h"
#include "rt_nonfinite.h"

/* ============================================================
 * FLAG compartida entre ISR y main
 * ============================================================ */
volatile uint8_t model_tick = 0;

/* ============================================================
 * DELAYS  (~10 iter/µs @ 64 MHz)
 * ============================================================ */
void delay_us(uint16_t us)
{
    volatile uint32_t n = (uint32_t)us * 10UL;
    while (n--);
}
void delay_ms(uint16_t ms)
{
    for (uint16_t i = 0; i < ms; i++) delay_us(1000U);
}

/* ============================================================
 * RCC — 64 MHz
 *   HSI (8 MHz) → PLL src = HSI/2 (4 MHz) → ×16 = 64 MHz
 *   APB1 ÷2 → 32 MHz   APB2 ÷1 → 64 MHz
 *   Flash: 2 wait-states
 * ============================================================ */
void rcc_init_64MHz(void)
{
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY)
               | FLASH_ACR_LATENCY_2
               | FLASH_ACR_PRFTBE;

    RCC->CFGR = (RCC->CFGR
                 & ~(RCC_CFGR_PLLSRC  | RCC_CFGR_PLLMULL |
                     RCC_CFGR_HPRE    | RCC_CFGR_PPRE1   |
                     RCC_CFGR_PPRE2))
               | RCC_CFGR_PLLMULL16
               | RCC_CFGR_PPRE1_DIV2;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/* ============================================================
 * TIM2 — interrupción cada 40 ms
 *   CK_INT = 64 MHz (APB1×2)
 *   PSC=6399 → 100 µs/tick   ARR=399 → 40 ms
 * ============================================================ */
void timer3_init(void) {}   /* requerido por main.h */

static void timer2_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->CR1  = 0;
    TIM2->PSC  = 6399U;
    TIM2->ARR  = 399U;
    TIM2->CNT  = 0;
    TIM2->SR   = 0;
    TIM2->DIER = TIM_DIER_UIE;
    TIM2->CR1  = TIM_CR1_CEN;
    NVIC_SetPriority(TIM2_IRQn, 1);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;
        model_tick = 1;
    }
}

/* ============================================================
 * TIM3 — PWM 1 kHz para 4 LEDs
 *
 *   CK_INT = 64 MHz (APB1×2)
 *   PSC = 63   → tick = 64/64 MHz = 1 µs
 *   ARR = 999  → periodo = 1000 µs = 1 kHz
 *   CCR = 0..999 → duty 0..100 %
 *
 *   CH1 → PA6   CH2 → PA7   CH3 → PB0   CH4 → PB1
 *   Pines AF push-pull (CNF=10, MODE=10 → 0xA)
 * ============================================================ */
static void pwm_init(void)
{
    /* Relojes */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    /* PA6 (CH1) y PA7 (CH2): AF push-pull 10 MHz → 0xA
     * CRL bits [27:24]=PA6, [31:28]=PA7 */
    GPIOA->CRL &= ~((0xFUL << 24) | (0xFUL << 28));
    GPIOA->CRL |=  ((0xAUL << 24) | (0xAUL << 28));

    /* PB0 (CH3) y PB1 (CH4): AF push-pull 10 MHz → 0xA
     * CRL bits [3:0]=PB0, [7:4]=PB1 */
    GPIOB->CRL &= ~((0xFUL <<  0) | (0xFUL <<  4));
    GPIOB->CRL |=  ((0xAUL <<  0) | (0xAUL <<  4));

    /* Timer base */
    TIM3->CR1  = 0;
    TIM3->PSC  = 63U;    /* 64 MHz / 64 = 1 MHz */
    TIM3->ARR  = 999U;   /* 1 MHz / 1000 = 1 kHz */
    TIM3->CNT  = 0;

    /* CH1–CH4: PWM mode 1 (OCxM=110), preload enable */
    TIM3->CCMR1 = (0x6UL << 4)  | TIM_CCMR1_OC1PE   /* CH1 */
                | (0x6UL << 12) | TIM_CCMR1_OC2PE;   /* CH2 */
    TIM3->CCMR2 = (0x6UL << 4)  | TIM_CCMR2_OC3PE   /* CH3 */
                | (0x6UL << 12) | TIM_CCMR2_OC4PE;   /* CH4 */

    /* Polaridad normal, salidas habilitadas */
    TIM3->CCER  = TIM_CCER_CC1E | TIM_CCER_CC2E
                | TIM_CCER_CC3E | TIM_CCER_CC4E;

    /* Arrancar con duty 0 */
    TIM3->CCR1 = 0;
    TIM3->CCR2 = 0;
    TIM3->CCR3 = 0;
    TIM3->CCR4 = 0;

    TIM3->EGR  = TIM_EGR_UG;   /* forzar update para cargar PSC/ARR */
    TIM3->CR1  = TIM_CR1_CEN;
}

/* Actualiza los 4 canales PWM según velocidad del vehículo
 *
 * Lógica progresiva (velocidad máx asumida ~120 km/h):
 *   v < 25%  → solo LED1 brilla (proporcional)
 *   v < 50%  → LED1 full + LED2 proporcional
 *   v < 75%  → LED1&2 full + LED3 proporcional
 *   v ≥ 75%  → LED1-3 full + LED4 proporcional
 */
static void pwm_update(uint16_t vspeed)
{
    /* Normalizar a 0–1000 (escala del ARR) */
    if (vspeed > 120U) vspeed = 120U;
    uint32_t v = ((uint32_t)vspeed * 1000UL) / 120UL;  /* 0–1000 */

    uint32_t q = 250UL;  /* cada cuarto = 250 unidades */

    uint32_t d1 = (v >= q)     ? 1000UL : (v * 1000UL / q);
    uint32_t d2 = (v >= 2*q)   ? 1000UL : (v > q   ? (v - q)   * 1000UL / q : 0UL);
    uint32_t d3 = (v >= 3*q)   ? 1000UL : (v > 2*q ? (v - 2*q) * 1000UL / q : 0UL);
    uint32_t d4 =                          (v > 3*q ? (v - 3*q) * 1000UL / q : 0UL);
    if (d4 > 1000UL) d4 = 1000UL;

    /* LED1=PB0=CH3  LED2=PB1=CH4  LED3=PA6=CH1  LED4=PA7=CH2 */
    TIM3->CCR3 = d1;
    TIM3->CCR4 = d2;
    TIM3->CCR1 = d3;
    TIM3->CCR2 = d4;
}

/* ============================================================
 * GPIO — PA0 analógico (ADC), PA1 entrada pull-up (freno)
 * ============================================================ */
static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* PA0 → analógico */
    GPIOA->CRL &= ~(0xFUL << 0);

    /* PA1 → entrada pull-up */
    GPIOA->CRL &= ~(0xFUL << 4);
    GPIOA->CRL |=  (0x8UL << 4);
    GPIOA->ODR |=  (1UL   << 1);
}

/* ============================================================
 * ADC1 — canal 0 (PA0)
 * ============================================================ */
static void adc_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV8;

    ADC1->CR2  &= ~ADC_CR2_ADON;
    ADC1->SMPR2 = (0x7UL << 0);
    ADC1->SQR3  = 0;
    ADC1->SQR1  = 0;
    ADC1->CR2  |=  ADC_CR2_ADON;
    delay_us(10);
    ADC1->CR2  |=  ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL);
}

static uint16_t adc_read(void)
{
    ADC1->CR2 |= ADC_CR2_ADON;
    while (!(ADC1->SR & ADC_SR_EOC));
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

/* ============================================================
 * LCD helpers
 * ============================================================ */
static void lcd_uint_w(uint16_t val, uint8_t width)
{
    char    buf[6];
    uint8_t i = 0;
    if (val == 0) { buf[i++] = '0'; }
    else { uint16_t v = val; while (v) { buf[i++] = (char)('0' + v % 10); v /= 10; } }
    for (uint8_t s = i; s < width; s++) LCD_Put_Char(' ');
    for (int8_t  j = (int8_t)(i - 1); j >= 0; j--) LCD_Put_Char((uint8_t)buf[j]);
}

static void lcd_update(uint16_t accel, uint8_t brake,
                       uint16_t rpm,   uint16_t vspeed, uint8_t gear)
{
    /* Línea 1: "A:XXX% R:XXXXX " */
    LCD_Set_Cursor(1, 1);
    LCD_Put_Str("A:");
    lcd_uint_w(accel, 3);
    LCD_Put_Char('%');
    LCD_Put_Str(" R:");
    lcd_uint_w(rpm, 5);
    LCD_Put_Char(' ');

    /* Línea 2 */
    LCD_Set_Cursor(2, 1);
    if (brake)
    {
        LCD_Put_Str("***FRENO*** M:");
        LCD_Put_Char((uint8_t)('0' + gear));
        LCD_Put_Char(' ');
    }
    else
    {
        LCD_Put_Str("V:");
        lcd_uint_w(vspeed, 3);
        LCD_Put_Str("km/h M:");
        LCD_Put_Char((uint8_t)('0' + gear));
        LCD_Put_Str("  ");
    }
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void)
{
    rcc_init_64MHz();
    gpio_init();
    adc_init();
    pwm_init();
    LCD_Init();
    timer2_init();

    rt_InitInfAndNaN(sizeof(real_T));
    EngTrModel_initialize();

    LCD_Set_Cursor(1, 1); LCD_Put_Str(" TRACTOR  AUTO  ");
    LCD_Set_Cursor(2, 1); LCD_Put_Str("  Iniciando...  ");
    delay_ms(1500);
    LCD_Clear();
    delay_ms(5);

    uint16_t rpm    = 0;
    uint16_t vspeed = 0;
    uint8_t  gear   = 1;

    /* Contador de ticks con aceleración = 0 para detectar idle */
    uint8_t  zero_ticks = 0;

    while (1)
    {
        if (!model_tick) continue;
        model_tick = 0;

        /* 1. Sensores */
        uint16_t raw   = adc_read();
        uint16_t accel = (raw * 100U) / 4095U;
        uint8_t  brake = (GPIOA->IDR & (1UL << 1)) ? 0U : 1U;

        /* ── Protección: forzar mínimo de aceleración
         *   El modelo Simulink se inestabiliza con Throttle=0
         *   porque el integrador de RPM se dispara al bajar
         *   el torque a valores negativos sin límite inferior.
         *   Mantenemos un mínimo de 2% (ralentí) y si llevamos
         *   más de 25 ticks (1 s) en cero, reiniciamos el modelo.
         * ────────────────────────────────────────────────────── */
        if (accel < 2U)
        {
            accel = 2U;
            zero_ticks++;
        }
        else
        {
            zero_ticks = 0;
        }

        /* Reinicio suave tras 1 segundo en idle (25 × 40 ms) */
        if (zero_ticks >= 25U)
        {
            zero_ticks = 0;
            EngTrModel_terminate();
            EngTrModel_initialize();
            rpm    = 800U;   /* RPM de ralentí */
            vspeed = 0U;
            gear   = 1U;
            pwm_update(0U);
            lcd_update(0U, brake, rpm, vspeed, gear);
            continue;
        }

        /* Sanity check: si el modelo devuelve valores imposibles
         * (NaN/Inf se convierten a valores enormes al castear),
         * también reiniciamos */
        if (EngTrModel_Y.EngineSpeed > 8000.0 ||
            EngTrModel_Y.EngineSpeed < 0.0    ||
            EngTrModel_Y.VehicleSpeed < 0.0   ||
            EngTrModel_Y.VehicleSpeed > 300.0)
        {
            EngTrModel_terminate();
            EngTrModel_initialize();
            rpm    = 800U;
            vspeed = 0U;
            gear   = 1U;
            zero_ticks = 0;
            pwm_update(0U);
            lcd_update(accel, brake, rpm, vspeed, gear);
            continue;
        }

        /* 2. Entradas del modelo */
        EngTrModel_U.Throttle    = (real_T)accel;
        EngTrModel_U.BrakeTorque = brake ? 200.0 : 0.0;

        /* 3. Paso del modelo */
        EngTrModel_step();

        /* 4. Salidas */
        rpm    = (uint16_t)EngTrModel_Y.EngineSpeed;
        vspeed = (uint16_t)EngTrModel_Y.VehicleSpeed;
        gear   = (uint8_t) EngTrModel_Y.Gear;
        if (gear < 1U) gear = 1U;
        if (gear > 4U) gear = 4U;

        /* 5. LCD */
        lcd_update(accel, brake, rpm, vspeed, gear);

        /* 6. PWM LEDs */
        pwm_update(vspeed);
    }
}