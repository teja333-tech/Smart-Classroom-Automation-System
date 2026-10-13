#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Handles */
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);

/* ================= LCD ================= */

#define LCD_ADDR        (0x27 << 1)
// If your LCD does not work, try:
// #define LCD_ADDR     (0x3F << 1)

#define LCD_BACKLIGHT   0x08
#define LCD_ENABLE      0x04
#define LCD_RS          0x01

/* ================= DHT11 ================= */

#define DHT_PORT GPIOB
#define DHT_PIN  GPIO_PIN_0

/* ================= SETTINGS ================= */

#define LIGHT_THRESHOLD 60
#define TEMP_THRESHOLD  35

/* =========================================================
   DWT MICROSECOND DELAY
   ========================================================= */

static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0;

    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;

    uint32_t cycles =
        us * (HAL_RCC_GetHCLKFreq() / 1000000);

    while ((DWT->CYCCNT - start) < cycles)
    {
    }
}

/* =========================================================
   LCD
   ========================================================= */

static void LCD_Write(uint8_t data)
{
    HAL_I2C_Master_Transmit(
        &hi2c1,
        LCD_ADDR,
        &data,
        1,
        100
    );
}

static void LCD_Pulse(uint8_t data)
{
    LCD_Write(data | LCD_ENABLE | LCD_BACKLIGHT);

    delay_us(1);

    LCD_Write((data & ~LCD_ENABLE) | LCD_BACKLIGHT);

    delay_us(50);
}

static void LCD_Send4Bits(uint8_t data)
{
    LCD_Pulse(data);
}

static void LCD_SendCommand(uint8_t cmd)
{
    uint8_t high;
    uint8_t low;

    high = cmd & 0xF0;
    low  = (cmd << 4) & 0xF0;

    LCD_Send4Bits(high);
    LCD_Send4Bits(low);
}

static void LCD_SendData(uint8_t data)
{
    uint8_t high;
    uint8_t low;

    high = (data & 0xF0) | LCD_RS;
    low  = ((data << 4) & 0xF0) | LCD_RS;

    LCD_Send4Bits(high);
    LCD_Send4Bits(low);
}

static void LCD_Init(void)
{
    HAL_Delay(50);

    LCD_Send4Bits(0x30);
    HAL_Delay(5);

    LCD_Send4Bits(0x30);
    delay_us(150);

    LCD_Send4Bits(0x30);
    delay_us(150);

    LCD_Send4Bits(0x20);
    delay_us(150);

    LCD_SendCommand(0x28);
    LCD_SendCommand(0x08);
    LCD_SendCommand(0x01);

    HAL_Delay(2);

    LCD_SendCommand(0x06);
    LCD_SendCommand(0x0C);
}

static void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

static void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
        address = 0x80 + col;
    else
        address = 0xC0 + col;

    LCD_SendCommand(address);
}

static void LCD_Print(char *str)
{
    while (*str)
    {
        LCD_SendData((uint8_t)*str);
        str++;
    }
}

/* =========================================================
   DHT11
   ========================================================= */

static void DHT_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(DHT_PORT, &GPIO_InitStruct);
}

static void DHT_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(DHT_PORT, &GPIO_InitStruct);
}

static uint8_t DHT_WaitForState(
    GPIO_PinState state,
    uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;

    uint32_t cycles =
        timeout_us * (HAL_RCC_GetHCLKFreq() / 1000000);

    while (HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) != state)
    {
        if ((DWT->CYCCNT - start) > cycles)
            return 0;
    }

    return 1;
}

static uint8_t DHT11_Read(
    uint8_t *temperature,
    uint8_t *humidity)
{
    uint8_t data[5] = {0};

    uint8_t i;

    /* Start signal */

    DHT_Pin_Output();

    HAL_GPIO_WritePin(
        DHT_PORT,
        DHT_PIN,
        GPIO_PIN_RESET
    );

    HAL_Delay(20);

    HAL_GPIO_WritePin(
        DHT_PORT,
        DHT_PIN,
        GPIO_PIN_SET
    );

    delay_us(30);

    DHT_Pin_Input();

    /* Sensor response */

    if (!DHT_WaitForState(GPIO_PIN_RESET, 100))
        return 0;

    if (!DHT_WaitForState(GPIO_PIN_SET, 100))
        return 0;

    if (!DHT_WaitForState(GPIO_PIN_RESET, 100))
        return 0;

    /* Read 40 bits */

    for (i = 0; i < 40; i++)
    {
        if (!DHT_WaitForState(GPIO_PIN_SET, 100))
            return 0;

        delay_us(40);

        if (HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN)
            == GPIO_PIN_SET)
        {
            data[i / 8] |=
                (1 << (7 - (i % 8)));
        }

        if (!DHT_WaitForState(GPIO_PIN_RESET, 100))
            return 0;
    }

    /* Checksum */

    if ((uint8_t)(
        data[0] +
        data[1] +
        data[2] +
        data[3])
        != data[4])
    {
        return 0;
    }

    *humidity = data[0];
    *temperature = data[2];

    return 1;
}

/* =========================================================
   READ LDR
   ========================================================= */

static uint32_t LDR_ReadADC(void)
{
    uint32_t adc;

    HAL_ADC_Start(&hadc1);

    HAL_ADC_PollForConversion(
        &hadc1,
        100
    );

    adc = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return adc;
}

/* =========================================================
   SEND DATA TO ESP32
   ========================================================= */

static void Send_To_ESP32(
    uint32_t brightness,
    uint8_t temperature,
    uint8_t humidity,
    uint8_t light,
    uint8_t relay)
{
    char buffer[120];

    sprintf(
        buffer,
        "TEMP=%d,HUM=%d,LIGHT=%lu,LED=%d,RELAY=%d\r\n",
        temperature,
        humidity,
        brightness,
        light,
        relay
    );

    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)buffer,
        strlen(buffer),
        100
    );
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    DWT_Init();

    LCD_Init();

    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_Print("SMART CLASSROOM");

    LCD_SetCursor(1, 0);
    LCD_Print("Starting...");

    HAL_Delay(1500);

    uint8_t temperature = 0;
    uint8_t humidity = 0;

    uint8_t dht_ok = 0;

    uint32_t adc_value;
    uint32_t brightness;

    uint8_t light_state = 0;
    uint8_t relay_state = 0;

    while (1)
    {
        /* LDR */

        adc_value = LDR_ReadADC();

        /*
         * Assumption:
         * More light = higher ADC value.
         */

        brightness =
            (adc_value * 100UL) / 4095UL;

        if (brightness < LIGHT_THRESHOLD)
        {
            light_state = 1;

            HAL_GPIO_WritePin(
                GPIOA,
                GPIO_PIN_2,
                GPIO_PIN_SET
            );
        }
        else
        {
            light_state = 0;

            HAL_GPIO_WritePin(
                GPIOA,
                GPIO_PIN_2,
                GPIO_PIN_RESET
            );
        }

        /* DHT11 */

        dht_ok = DHT11_Read(
            &temperature,
            &humidity
        );

        if (dht_ok)
        {
            if (temperature > TEMP_THRESHOLD)
            {
                relay_state = 1;

                HAL_GPIO_WritePin(
                    GPIOA,
                    GPIO_PIN_1,
                    GPIO_PIN_SET
                );
            }
            else
            {
                relay_state = 0;

                HAL_GPIO_WritePin(
                    GPIOA,
                    GPIO_PIN_1,
                    GPIO_PIN_RESET
                );
            }
        }
        else
        {
            /* Sensor error */
            relay_state = 0;

            HAL_GPIO_WritePin(
                GPIOA,
                GPIO_PIN_1,
                GPIO_PIN_RESET
            );
        }

        /* LCD */

        LCD_Clear();

        LCD_SetCursor(0, 0);

        if (dht_ok)
        {
            char line1[17];

            sprintf(
                line1,
                "T:%dC H:%d%%",
                temperature,
                humidity
            );

            LCD_Print(line1);
        }
        else
        {
            LCD_Print("DHT11 ERROR");
        }

        LCD_SetCursor(1, 0);

        char line2[17];

        sprintf(
            line2,
            "L:%lu%% %s",
            brightness,
            light_state ? "ON" : "OFF"
        );

        LCD_Print(line2);

        /* SEND TO ESP32 */

        Send_To_ESP32(
            brightness,
            temperature,
            humidity,
            light_state,
            relay_state
        );

        HAL_Delay(2000);
    }
}

/* =========================================================
   SYSTEM CLOCK
   ========================================================= */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct)
        != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
        &RCC_ClkInitStruct,
        FLASH_LATENCY_0)
        != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection =
        RCC_PERIPHCLK_ADC;

    PeriphClkInit.AdcClockSelection =
        RCC_ADCPCLK2_DIV2;

    if (HAL_RCCEx_PeriphCLKConfig(
        &PeriphClkInit)
        != HAL_OK)
    {
        Error_Handler();
    }
}

/* =========================================================
   ADC
   ========================================================= */

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;

    hadc1.Init.ScanConvMode =
        ADC_SCAN_DISABLE;

    hadc1.Init.ContinuousConvMode =
        DISABLE;

    hadc1.Init.DiscontinuousConvMode =
        DISABLE;

    hadc1.Init.ExternalTrigConv =
        ADC_SOFTWARE_START;

    hadc1.Init.DataAlign =
        ADC_DATAALIGN_RIGHT;

    hadc1.Init.NbrOfConversion = 1;

    if (HAL_ADC_Init(&hadc1)
        != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel =
        ADC_CHANNEL_0;

    sConfig.Rank =
        ADC_REGULAR_RANK_1;

    sConfig.SamplingTime =
        ADC_SAMPLETIME_239CYCLES_5;

    if (HAL_ADC_ConfigChannel(
        &hadc1,
        &sConfig)
        != HAL_OK)
    {
        Error_Handler();
    }
}

/* =========================================================
   I2C
   ========================================================= */

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;

    hi2c1.Init.ClockSpeed = 100000;

    hi2c1.Init.DutyCycle =
        I2C_DUTYCYCLE_2;

    hi2c1.Init.OwnAddress1 = 0;

    hi2c1.Init.AddressingMode =
        I2C_ADDRESSINGMODE_7BIT;

    hi2c1.Init.DualAddressMode =
        I2C_DUALADDRESS_DISABLE;

    hi2c1.Init.OwnAddress2 = 0;

    hi2c1.Init.GeneralCallMode =
        I2C_GENERALCALL_DISABLE;

    hi2c1.Init.NoStretchMode =
        I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1)
        != HAL_OK)
    {
        Error_Handler();
    }
}

/* =========================================================
   USART1
   ========================================================= */

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;

    huart1.Init.BaudRate = 115200;

    huart1.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart1.Init.StopBits =
        UART_STOPBITS_1;

    huart1.Init.Parity =
        UART_PARITY_NONE;

    huart1.Init.Mode =
        UART_MODE_TX_RX;

    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1)
        != HAL_OK)
    {
        Error_Handler();
    }
}

/* =========================================================
   GPIO
   ========================================================= */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA1 = Relay
       PA2 = Light */

    HAL_GPIO_WritePin(
        GPIOA,
        GPIO_PIN_1 | GPIO_PIN_2,
        GPIO_PIN_RESET
    );

    GPIO_InitStruct.Pin =
        GPIO_PIN_1 | GPIO_PIN_2;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );

    /* PA0 = LDR ADC */

    GPIO_InitStruct.Pin =
        GPIO_PIN_0;

    GPIO_InitStruct.Mode =
        GPIO_MODE_ANALOG;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );

    /* PB0 = DHT11 */

    GPIO_InitStruct.Pin =
        GPIO_PIN_0;

    GPIO_InitStruct.Mode =
        GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );
}

/* =========================================================
   ERROR
   ========================================================= */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
