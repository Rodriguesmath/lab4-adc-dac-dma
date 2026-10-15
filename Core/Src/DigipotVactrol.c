/******************************************************************************/
/**
 * @file    DigipotVactrol.c
 * @brief   Implementacao do Digipot optico com Look-Up Table e calculo de RMSE.
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "DigipotVactrol.h"
#include "Bsp.h"
#include <math.h>
#include <string.h>

/*******************************************************************************
 * DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
 ******************************************************************************/
#define dLDR_MAX_DARK_RESISTANCE_OHM      1000000.0f // 1 MOhm no escuro
#define dLDR_MIN_LIGHT_RESISTANCE_OHM     200.0f     // 200 Ohm com luz maxima

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    u8    currentWord;
    u16   lutDac[dDIGIPOT_LUT_SIZE];
    float lastMeasuredResistanceOhm;
    bool  isCalibrated;
} digipotVactrol;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/
static void DigipotVactrol_GenerateDefaultLut(void);

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

void DigipotVactrol_Init(void)
{
    DigipotVactrol_GenerateDefaultLut();
    DigipotVactrol_SetWord(0);
    digipotVactrol.isCalibrated = true;
}

void DigipotVactrol_SetWord(u8 resWord)
{
    digipotVactrol.currentWord = resWord;

    // Obtem o valor do DAC/PWM mapeado na tabela de calibracao LUT
    u16 dacVal = digipotVactrol.lutDac[resWord];
    Bsp_DacSetValue(dacVal);
}

float DigipotVactrol_MeasureResistanceOhm(void)
{
    // Le a tensao do divisor de tensao com media movel
    float vAdc = Bsp_AdcReadVoltage();

    // Evita divisao por zero
    if(vAdc >= (dBSP_ADC_VREF_VOLTS - 0.05f))
    {
        digipotVactrol.lastMeasuredResistanceOhm = dLDR_MAX_DARK_RESISTANCE_OHM;
        return dLDR_MAX_DARK_RESISTANCE_OHM;
    }
    if(vAdc <= 0.05f)
    {
        digipotVactrol.lastMeasuredResistanceOhm = dLDR_MIN_LIGHT_RESISTANCE_OHM;
        return dLDR_MIN_LIGHT_RESISTANCE_OHM;
    }

    // Formula do divisor de tensao: V_adc = V_ref * (R_ldr / (R_fixo + R_ldr))
    // Logo: R_ldr = R_fixo * (V_adc / (V_ref - V_adc))
    float rLdr = dBSP_ADC_DIVIDER_RESISTOR_OHM * (vAdc / (dBSP_ADC_VREF_VOLTS - vAdc));

    digipotVactrol.lastMeasuredResistanceOhm = rLdr;
    return rLdr;
}

void DigipotVactrol_UpdateLutFromDma(const u16 *pNewLut, u16 length)
{
    if((pNewLut != NULL) && (length <= dDIGIPOT_LUT_SIZE))
    {
        memcpy(digipotVactrol.lutDac, pNewLut, length * sizeof(u16));
        digipotVactrol.isCalibrated = true;
    }
}

float DigipotVactrol_CalculateRmse(const float *pMeasured, const float *pExpected, u8 count)
{
    if((pMeasured == NULL) || (pExpected == NULL) || (count == 0))
    {
        return 0.0f;
    }

    float sumSquares = 0.0f;

    for(u8 i = 0; i < count; i++)
    {
        float error = pMeasured[i] - pExpected[i];
        sumSquares += (error * error);
    }

    return sqrtf(sumSquares / (float)count);
}

bool DigipotVactrol_SetTargetResistanceOhm(float targetOhm)
{
    if((targetOhm < dLDR_MIN_LIGHT_RESISTANCE_OHM) || (targetOhm > dLDR_MAX_DARK_RESISTANCE_OHM))
    {
        return false;
    }

    // Busca de convergencia em malha fechada (Closed-Loop Calibration)
    // Se a resistencia medida for maior que a desejada, aumenta a luz (aumenta word)
    u8 word = digipotVactrol.currentWord;

    for(u8 iter = 0; iter < 16; iter++)
    {
        DigipotVactrol_SetWord(word);
        Bsp_DelayMs(20);
        float currentR = DigipotVactrol_MeasureResistanceOhm();

        float errorRatio = fabsf(currentR - targetOhm) / targetOhm;
        if(errorRatio < 0.05f) // Convergiu com erro inferior a 5%
        {
            return true;
        }

        if(currentR > targetOhm)
        {
            if(word < 255)
            {
                word++;
            }
        }
        else
        {
            if(word > 0)
            {
                word--;
            }
        }
    }

    return true;
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

static void DigipotVactrol_GenerateDefaultLut(void)
{
    // Modelo logaritmico inicial de aproximacao do Vactrol
    // Quanto maior resWord (0..255), maior a luz do LED, menor a resistencia R_LDR
    for(u16 i = 0; i < dDIGIPOT_LUT_SIZE; i++)
    {
        float ratio = (float)i / (float)(dDIGIPOT_LUT_SIZE - 1);
        // Curva exponencial para compensar a resposta do LED + LDR
        float dacScaled = powf(ratio, 1.8f) * 4095.0f;
        digipotVactrol.lutDac[i] = (u16)dacScaled;
    }
}

/** @} DOXYGEN GROUP TAG END OF FILE */
