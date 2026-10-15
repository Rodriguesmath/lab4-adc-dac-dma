/******************************************************************************/
/**
 * @file    Bsp.c
 * @brief   Implementacao do BSP para Lab 4 ADC, DAC e DMA STM32 (Digipot).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Bsp.h"
#include <string.h>

/*******************************************************************************
 * DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
 ******************************************************************************/
#define dMOVING_AVG_SAMPLES               8
#define dADC_TIMEOUT_MS                   10
#define dUART_TIMEOUT_MS                  100

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    ADC_HandleTypeDef  hadc1;
#if defined(STM32H723xx) || defined(STM32F767xx)
    DAC_HandleTypeDef  hdac;
#else
    TIM_HandleTypeDef  htimPwmDac;
#endif
    UART_HandleTypeDef huart1;
    DMA_HandleTypeDef  hdmaUartRx;
    bool isInitialized;
} bsp;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/
static void Bsp_SystemClockConfig(void);
static void Bsp_GpioInit(void);
static void Bsp_AdcInit(void);
static void Bsp_DacInit(void);
static void Bsp_UartDmaInit(void);

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

void Bsp_Init(void)
{
    HAL_Init();
    Bsp_SystemClockConfig();
    Bsp_GpioInit();
    Bsp_AdcInit();
    Bsp_DacInit();
    Bsp_UartDmaInit();

    bsp.isInitialized = true;
}

void Bsp_LedToggle(void)
{
    HAL_GPIO_TogglePin(dBSP_LED_PORT, dBSP_LED_PIN);
}

void Bsp_DelayMs(u32 ms)
{
    HAL_Delay(ms);
}

void Bsp_DacSetValue(u16 dacVal)
{
    if(dacVal > 4095)
    {
        dacVal = 4095;
    }

#if defined(STM32H723xx) || defined(STM32F767xx)
    HAL_DAC_SetValue(&bsp.hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dacVal);
    HAL_DAC_Start(&bsp.hdac, DAC_CHANNEL_1);
#else
    // Em F411: emulado via PWM escalado (ARR = 4095)
    __HAL_TIM_SET_COMPARE(&bsp.htimPwmDac, TIM_CHANNEL_1, dacVal);
#endif
}

u16 Bsp_AdcReadFiltered(void)
{
    u32 sum = 0;

    // Regra ASSERT 5.1: Filtragem por Media Movel de todo sinal analogico medido
    for(u8 i = 0; i < dMOVING_AVG_SAMPLES; i++)
    {
        HAL_ADC_Start(&bsp.hadc1);
        if(HAL_ADC_PollForConversion(&bsp.hadc1, dADC_TIMEOUT_MS) == HAL_OK)
        {
            sum += HAL_ADC_GetValue(&bsp.hadc1);
        }
        HAL_ADC_Stop(&bsp.hadc1);
    }

    return (u16)(sum / dMOVING_AVG_SAMPLES);
}

float Bsp_AdcReadVoltage(void)
{
    u16 rawAdc = Bsp_AdcReadFiltered();
    return ((float)rawAdc / dBSP_ADC_MAX_COUNT) * dBSP_ADC_VREF_VOLTS;
}

void Bsp_UartTransmit(const char *pStr)
{
    if(pStr != NULL)
    {
        HAL_UART_Transmit(&bsp.huart1, (uint8_t *)pStr, (uint16_t)strlen(pStr), dUART_TIMEOUT_MS);
    }
}

void Bsp_UartStartDmaReceive(u8 *pBuffer, u16 length)
{
    HAL_UART_Receive_DMA(&bsp.huart1, pBuffer, length);
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

static void Bsp_SystemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#if defined(STM32F411xE)
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
#endif
}

static void Bsp_GpioInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* LED On-Board */
    HAL_GPIO_WritePin(dBSP_LED_PORT, dBSP_LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = dBSP_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dBSP_LED_PORT, &GPIO_InitStruct);
}

static void Bsp_AdcInit(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    __HAL_RCC_ADC1_CLK_ENABLE();

    bsp.hadc1.Instance = ADC1;
    bsp.hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    bsp.hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    bsp.hadc1.Init.ScanConvMode = DISABLE;
    bsp.hadc1.Init.ContinuousConvMode = DISABLE;
    bsp.hadc1.Init.DiscontinuousConvMode = DISABLE;
    bsp.hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    bsp.hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    bsp.hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    bsp.hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&bsp.hadc1);

    /* Canal 0 (PA0) */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES; // Tempo de amostragem alto para estabilidade
    HAL_ADC_ConfigChannel(&bsp.hadc1, &sConfig);
}

static void Bsp_DacInit(void)
{
#if defined(STM32H723xx) || defined(STM32F767xx)
    DAC_ChannelConfTypeDef sConfig = {0};
    __HAL_RCC_DAC1_CLK_ENABLE();
    bsp.hdac.Instance = DAC1;
    HAL_DAC_Init(&bsp.hdac);
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&bsp.hdac, &sConfig, DAC_CHANNEL_1);
#else
    /* F411: Emulado por PWM de 12 bits */
    TIM_OC_InitTypeDef sConfigOC = {0};
    __HAL_RCC_TIM2_CLK_ENABLE();
    bsp.htimPwmDac.Instance = TIM2;
    bsp.htimPwmDac.Init.Prescaler = 0;
    bsp.htimPwmDac.Init.CounterMode = TIM_COUNTERMODE_UP;
    bsp.htimPwmDac.Init.Period = 4095;
    bsp.htimPwmDac.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    bsp.htimPwmDac.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&bsp.htimPwmDac);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&bsp.htimPwmDac, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&bsp.htimPwmDac, TIM_CHANNEL_1);
#endif
}

static void Bsp_UartDmaInit(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    bsp.huart1.Instance = USART1;
    bsp.huart1.Init.BaudRate = 115200;
    bsp.huart1.Init.WordLength = UART_WORDLENGTH_8B;
    bsp.huart1.Init.StopBits = UART_STOPBITS_1;
    bsp.huart1.Init.Parity = UART_PARITY_NONE;
    bsp.huart1.Init.Mode = UART_MODE_TX_RX;
    bsp.huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    bsp.huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&bsp.huart1);

    /* DMA USART1_RX */
    bsp.hdmaUartRx.Instance = DMA2_Stream2;
    bsp.hdmaUartRx.Init.Channel = DMA_CHANNEL_4;
    bsp.hdmaUartRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    bsp.hdmaUartRx.Init.PeriphInc = DMA_PINC_DISABLE;
    bsp.hdmaUartRx.Init.MemInc = DMA_MINC_ENABLE;
    bsp.hdmaUartRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    bsp.hdmaUartRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    bsp.hdmaUartRx.Init.Mode = DMA_NORMAL;
    bsp.hdmaUartRx.Init.Priority = DMA_PRIORITY_MEDIUM;
    bsp.hdmaUartRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&bsp.hdmaUartRx);

    __HAL_LINKDMA(&bsp.huart1, hdmarx, bsp.hdmaUartRx);

    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/** @} DOXYGEN GROUP TAG END OF FILE */
