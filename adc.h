#ifndef ADC_H
#define ADC_H

void adc_init(void);
bool adc_acquire(volatile uint32_t *pBuffer_adc, uint32_t numberOfSamples);
void adc_to_q15(volatile uint32_t *pBuffer_adc, q15_t *pBuffer_q15, uint32_t numberOfSamples);

#endif
