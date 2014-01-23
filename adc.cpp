#include "arm_math.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "constants.h"

#define ADC_DMA_CHANNEL_NUMBER 0

/* ADC channel 0 input; mbed pin 15 */
static const PINSEL_CFG_Type ADC_Pin = {
  PINSEL_PORT_0,
  PINSEL_PIN_23,
  PINSEL_FUNC_1,
  PINSEL_PINMODE_TRISTATE,
  PINSEL_PINMODE_NORMAL
};

static GPDMA_Channel_CFG_Type GPDMACfg;

void adc_init(void)
{
  PINSEL_ConfigPin((PINSEL_CFG_Type *)&ADC_Pin);
  ADC_Init(LPC_ADC, ADC_SAMPLE_RATE_HZ);
  GPDMA_Init();
  ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
}

bool adc_acquire(volatile uint32_t *pBuffer_adc, uint32_t numberOfSamples)
{
  if (numberOfSamples > 0xfff)
  {
    /* Exceeds max transfer size for a single DMA descriptor */
    return false;
  }

  /* Setup DMA transfer from ADC to RAM */
  GPDMACfg.ChannelNum = ADC_DMA_CHANNEL_NUMBER;
  GPDMACfg.SrcMemAddr = 0;
  GPDMACfg.DstMemAddr = (uint32_t)pBuffer_adc;
  GPDMACfg.TransferSize = numberOfSamples;
  GPDMACfg.TransferWidth = 0;
  GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
  GPDMACfg.SrcConn = GPDMA_CONN_ADC;
  GPDMACfg.DstConn = 0;
  GPDMACfg.DMALLI = 0;

  if (GPDMA_Setup(&GPDMACfg) != SUCCESS)
  {
    return false;
  }

  GPDMA_ChannelCmd(ADC_DMA_CHANNEL_NUMBER, ENABLE);

  /* Start aquisition */
  ADC_BurstCmd(LPC_ADC, ENABLE);

  /* Wait for completion */
  while (GPDMA_IntGetStatus(GPDMA_STAT_INTTC, ADC_DMA_CHANNEL_NUMBER) != SET);

  /* Completed */
  ADC_BurstCmd(LPC_ADC, DISABLE);
  return true;
}

void adc_to_q15(volatile uint32_t *pBuffer_adc, q15_t *pBuffer_q15, uint32_t numberOfSamples)
{
  /* The 12-bit ADC reading is implicitly shifted left by four bits and treated */
  /* as a 16-bit unsigned value. This is then converted to a signed q15 value. */
  while (numberOfSamples-- > 0)
  {
    *pBuffer_q15++ = (q15_t)((int32_t)(*pBuffer_adc++ & 0xfff0) - 0x8000);
  }
}
