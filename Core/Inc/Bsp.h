/******************************************************************************/
/**
 * @file    Bsp.h
 * @brief   Board Support Package para Lab 4 ADC, DAC e DMA STM32 (Digipot).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/
#ifndef _BSP_H_
#define _BSP_H_

/*******************************************************************************
 * INCLUDES NECESSARIOS
 ******************************************************************************/
#include "AssertTypes.h"

#if defined(STM32F411xE) || defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32H723xx)
  #include "stm32h7xx_hal.h"
#elif defined(STM32F767xx)
  #include "stm32f7xx_hal.h"
#else
  #include "stm32f4xx_hal.h"
#endif

/*******************************************************************************
 * CONFIGURACOES
 ******************************************************************************/
#define dBSP_ADC_DIVIDER_RESISTOR_OHM     10000.0f // R2 = 10 kOhm
#define dBSP_ADC_VREF_VOLTS               3.3f
#define dBSP_ADC_MAX_COUNT                4095.0f  // 12 bits ADC

/*******************************************************************************
 * DEFINES PUBLICOS
 ******************************************************************************/
#define dBSP_ADC_PIN                      GPIO_PIN_0
#define dBSP_ADC_PORT                     GPIOA
#define dBSP_DAC_PIN                      GPIO_PIN_4
#define dBSP_DAC_PORT                     GPIOA

#if defined(STM32F411xE)
  #define dBSP_LED_PIN                    GPIO_PIN_13
  #define dBSP_LED_PORT                   GPIOC
#elif defined(STM32F767xx)
  #define dBSP_LED_PIN                    GPIO_PIN_0
  #define dBSP_LED_PORT                   GPIOB
#else
  #define dBSP_LED_PIN                    GPIO_PIN_13
  #define dBSP_LED_PORT                   GPIOC
#endif

/*******************************************************************************
 * PROTOTIPOS PUBLICOS
 ******************************************************************************/
void  Bsp_Init(void);
void  Bsp_LedToggle(void);
void  Bsp_DelayMs(u32 ms);
void  Bsp_DacSetValue(u16 dacVal);
u16   Bsp_AdcReadFiltered(void);
float Bsp_AdcReadVoltage(void);
void  Bsp_UartTransmit(const char *pStr);
void  Bsp_UartStartDmaReceive(u8 *pBuffer, u16 length);

#endif /* _BSP_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
