/******************************************************************************/
/**
 * @file    main.c
 * @brief   Ponto de entrada do Lab 4 - Potenciometro Digital Optico (ADC, DAC, DMA).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "main.h"
#include "Bsp.h"
#include "DigipotVactrol.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * DEFINES LOCAIS
 ******************************************************************************/
#define dTEST_POINTS_COUNT               10
#define dDMA_BUFFER_SIZE                 (dDIGIPOT_LUT_SIZE * 2)

/*******************************************************************************
 * CONSTANTES
 ******************************************************************************/
static const u8 cTestWords[dTEST_POINTS_COUNT] =
{
    0, 28, 56, 85, 113, 142, 170, 198, 227, 255
};

// Resistencias esperadas aproximadas para os 10 pontos de teste (em Ohms)
static const float cExpectedResistance[dTEST_POINTS_COUNT] =
{
    100000.0f, 45000.0f, 22000.0f, 12000.0f, 7500.0f,
    4800.0f,   3100.0f,  2000.0f,  1200.0f,  500.0f
};

/*******************************************************************************
 * ESTRUTURAS DE DADOS LOCAIS
 ******************************************************************************/
static struct
{
    u8    dmaRxBuffer[dDMA_BUFFER_SIZE];
    float measuredResistances[dTEST_POINTS_COUNT];
    float lastRmse;
    u32   lastTestTick;
} mainApp;

/*******************************************************************************
 * PROTOTIPOS LOCAIS
 ******************************************************************************/
static void Main_RunCalibrationTest(void);
static void Main_PrintResults(float rmse);
static void Main_RunApplicationExample(void);

/*******************************************************************************
 * FUNCOES PUBLICAS
 ******************************************************************************/

int main(void)
{
    Bsp_Init();
    DigipotVactrol_Init();

    mainApp.lastTestTick = 0;
    mainApp.lastRmse = 0.0f;

    Bsp_UartTransmit("\r\n==================================================\r\n");
    Bsp_UartTransmit("  Lab 4 - Potenciometro Digital Optico (Vactrol)\r\n");
    Bsp_UartTransmit("  Assert Coding Standard - Virtus-CC / Rodrigues\r\n");
    Bsp_UartTransmit("==================================================\r\n");

    // Inicia escuta UART DMA para recepcao de nova tabela LUT
    Bsp_UartStartDmaReceive(mainApp.dmaRxBuffer, dDMA_BUFFER_SIZE);

    // Executa primeira rodada de testes
    Main_RunCalibrationTest();
    Main_RunApplicationExample();

    while(1)
    {
        u32 currentTick = HAL_GetTick();

        // Repete o ciclo de testes a cada 5 segundos
        if((currentTick - mainApp.lastTestTick) >= 5000)
        {
            mainApp.lastTestTick = currentTick;
            Bsp_LedToggle();
            Main_RunCalibrationTest();
            Main_RunApplicationExample();
        }

        Bsp_DelayMs(10);
    }
}

/*******************************************************************************
 * FUNCOES LOCAIS
 ******************************************************************************/

static void Main_RunCalibrationTest(void)
{
    char msgBuffer[128];

    Bsp_UartTransmit("\r\n--- Executando Sequencia de 10 Pontos de Teste ---\r\n");

    for(u8 i = 0; i < dTEST_POINTS_COUNT; i++)
    {
        u8 testWord = cTestWords[i];

        // Aplica o degrau no Digipot
        DigipotVactrol_SetWord(testWord);

        // Tempo de acomodacao para estabilizacao fotoquimica do LDR (50 ms)
        Bsp_DelayMs(50);

        // Mede resistencia via ADC com media movel
        float rMeasured = DigipotVactrol_MeasureResistanceOhm();
        mainApp.measuredResistances[i] = rMeasured;

        snprintf(msgBuffer, sizeof(msgBuffer),
                 "Ponto %02u | Word: %3u | R_med: %7.1f Ohm | R_exp: %7.1f Ohm\r\n",
                 (unsigned int)(i + 1),
                 (unsigned int)testWord,
                 rMeasured,
                 cExpectedResistance[i]);
        Bsp_UartTransmit(msgBuffer);
    }

    // Calcula RMSE
    float rmse = DigipotVactrol_CalculateRmse(mainApp.measuredResistances,
                                            cExpectedResistance,
                                            dTEST_POINTS_COUNT);
    mainApp.lastRmse = rmse;
    Main_PrintResults(rmse);
}

static void Main_PrintResults(float rmse)
{
    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer),
             "\r\n>>> Erro Medio Quadratico (RMSE): %.2f Ohm <<<\r\n", rmse);
    Bsp_UartTransmit(msgBuffer);
    Bsp_UartTransmit("--------------------------------------------------\r\n");
}

static void Main_RunApplicationExample(void)
{
    char msg[128];
    const float targetResistances[] = {10000.0f, 5000.0f, 2200.0f};

    Bsp_UartTransmit("\r\n>>> APLICACAO EXEMPLO (REQUISITO AVANCADO): Resistor Calibrado / Controle de Ganho <<<\r\n");

    for(u8 i = 0; i < 3; i++)
    {
        float target = targetResistances[i];
        DigipotVactrol_SetTargetResistanceOhm(target);
        float actual = DigipotVactrol_MeasureResistanceOhm();

        snprintf(msg, sizeof(msg),
                 "[AGC/Calibrado] Alvo: %7.1f Ohm | Ajustado: %7.1f Ohm | Erro: %+.2f%%\r\n",
                 target,
                 actual,
                 ((actual - target) / target) * 100.0f);
        Bsp_UartTransmit(msg);
    }
    Bsp_UartTransmit("==================================================\r\n");
}

/** @} DOXYGEN GROUP TAG END OF FILE */
