#include "arm_math.h"
#include "constants.h"

#ifdef TARGET_NUCLEO_F103RB

/*
 *
 * ST Nucleo F103RB version
 *
 */
 
 #include "stm32f10x.h"
 
 //#define CLOCK_OUT
 
 #define ADC1_DR_Address ((uint32_t)0x4001244C)

ADC_InitTypeDef ADC_InitStructure;
DMA_InitTypeDef DMA_InitStructure;
__IO uint16_t ADCConvertedValue; 
 
void adc_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef   TIM_TimeBaseStructure;
  TIM_OCInitTypeDef         TIM_OCInitStructure;
  uint16_t divider = 0;
  
  if (SystemCoreClock == 64000000)
  {
    divider = 2000;
  }
  
  if (SystemCoreClock == 72000000)
  {
    divider = 2250;
  }
  
  if (divider == 0)
  {
    /* Invalid */
    return;
  }

  /* ADCCLK = PCLK2/2 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div2); 

  /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  /* Enable Timer3 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  
  /* Enable ADC1, GPIOA and GPIOB clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

  /* Configure PA0 (Arduino A0 pin) as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
 
 #ifdef CLOCK_OUT
  /* Configure PB0 (Arduino A3) as clock toggle output */
  /* Note this will be at half the sample frequency */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

  /* Timer3 configuration */
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure); 
  TIM_TimeBaseStructure.TIM_Period = divider;       /* Autoreload value */   
  TIM_TimeBaseStructure.TIM_Prescaler = 0x0;       
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;    
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;  
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  
 #ifdef CLOCK_OUT
  /* Timer3 channel configuration */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle; 
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;                
  TIM_OCInitStructure.TIM_Pulse = 0; 
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;         
  TIM_OC3Init(TIM3, &TIM_OCInitStructure);
#endif

  /* Timer3 trigger output on update */
  TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
  
  /* ADC1 configuration */
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel 0 configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
 
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
  
  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
  
  /* Enable ADC1 external trigger */ 
  ADC_ExternalTrigConvCmd(ADC1, ENABLE);
}
 
bool adc_acquire(volatile uint32_t *pBuffer_adc, uint32_t numberOfSamples)
{
  /* DMA1 channel1 configuration */
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pBuffer_adc;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = numberOfSamples;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* Enable DMA1 channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);     
     
  /* Timer 3 counter enable */
  TIM_Cmd(TIM3, ENABLE);

  /* Wait for completion of DMA transfer */
  while (DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);

  /* Timer 3 counter disable */
  TIM_Cmd(TIM3, DISABLE);
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

#endif // #ifdef TARGET_NUCLEO_F103RB


#ifdef TARGET_LPC1768

/*
 *
 * mbed LPC1768 version
 *
 */

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

#endif // #ifdef TARGET_LPC1768
