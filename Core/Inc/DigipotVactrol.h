/******************************************************************************/
/**
 * @file    DigipotVactrol.h
 * @brief   Emulador de Potenciometro Digital via Acoplamento Optico (Vactrol).
 * @author  Rodrigues - Formacao em Sistemas Embarcados Virtus-CC
 * @note    Estilo de codificacao inspirado nas normas do Laboratorio ASSERT (IFPB).
 * @{
 ******************************************************************************/
#ifndef _DIGIPOT_VACTROL_H_
#define _DIGIPOT_VACTROL_H_

/*******************************************************************************
 * INCLUDES NECESSARIOS
 ******************************************************************************/
#include "AssertTypes.h"

/*******************************************************************************
 * CONFIGURACOES
 ******************************************************************************/
#define dDIGIPOT_LUT_SIZE                 256
#define dDIGIPOT_CALIBRATION_POINTS       10

/*******************************************************************************
 * PROTOTIPOS PUBLICOS
 ******************************************************************************/
void  DigipotVactrol_Init(void);
void  DigipotVactrol_SetWord(u8 resWord);
float DigipotVactrol_MeasureResistanceOhm(void);
void  DigipotVactrol_UpdateLutFromDma(const u16 *pNewLut, u16 length);
float DigipotVactrol_CalculateRmse(const float *pMeasured, const float *pExpected, u8 count);

#endif /* _DIGIPOT_VACTROL_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
